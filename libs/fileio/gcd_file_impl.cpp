#ifdef __APPLE__

#include <fileio/detail/gcd_file_impl.h>
#include <fileio/detail/gcd_file_service.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_KQUEUE

#include "src/detail/make_err.hpp"

#include <unistd.h>
#include <dispatch/dispatch.h>

namespace nntp::detail {

//------------------------------------------------------------------------------
// gcd_file_impl_internal

gcd_file_impl_internal::gcd_file_impl_internal(gcd_file_service& svc) noexcept
    : svc_(svc)
    , rd_(*this)
    , wr_(*this)
{
}

gcd_file_impl_internal::~gcd_file_impl_internal()
{
    svc_.unregister_impl(*this);
}

void
gcd_file_impl_internal::release_internal()
{
    // Cancel pending I/O before closing
    if (fd_ != -1)
    {
        cancel();
    }
    close_file();
}

void
gcd_file_impl_internal::open_channel()
{
    if (fd_ == -1 || channel_)
        return;

    // Create dispatch_io channel from file descriptor
    // Use DISPATCH_IO_RANDOM for random access (seek support)
    channel_ = dispatch_io_create(
        DISPATCH_IO_RANDOM,
        fd_,
        svc_.io_queue(),
        ^(int error) {
            // Cleanup handler - called when channel closes
            if (error)
            {
                // Log or handle error if needed
            }
        });

    if (channel_)
    {
        // Set low water mark to 1 byte to avoid automatic buffering delays
        dispatch_io_set_low_water(channel_, 1);
    }
}

void
gcd_file_impl_internal::close_channel() noexcept
{
    if (channel_)
    {
        dispatch_io_close(channel_, 0);
        dispatch_release(channel_);
        channel_ = nullptr;
    }
}

std::coroutine_handle<>
gcd_file_impl_internal::read_some(
    std::coroutine_handle<> h,
    boost::capy::executor_ref ex,
    boost::corosio::io_buffer_param buffers,
    std::stop_token token,
    std::error_code* ec,
    std::size_t* bytes_transferred)
{
    // Keep internal alive during I/O
    rd_.internal_ptr = shared_from_this();

    auto& op = rd_;
    op.reset();
    op.h = h;
    op.ex = ex;
    op.ec_out = ec;
    op.bytes_out = bytes_transferred;
    op.start(token);

    // Prepare buffer (files use single buffer)
    boost::capy::mutable_buffer bufs[1];
    auto buf_count = buffers.copy_to(bufs, 1);

    // Handle empty buffer: complete with 0 bytes via post for consistency
    if (buf_count == 0)
    {
        op.bytes_transferred = 0;
        op.empty_buffer = true;
        if (ec)
            *ec = std::error_code();
        if (bytes_transferred)
            *bytes_transferred = 0;
        // Post completion
        svc_.sched_.post(&op);
        return std::noop_coroutine();
    }

    // Store buffer information in operation
    op.buffer_ptr = bufs[0].data();
    op.buffer_size = bufs[0].size();
    op.file_offset = position_;

    // Symmetric transfer to initiator - I/O starts after caller is suspended
    return read_initiator_.start<&gcd_file_impl_internal::do_read_io>(this);
}

void
gcd_file_impl_internal::do_read_io()
{
    auto& op = rd_;

    svc_.work_started();

    // Check if channel is open
    if (!channel_)
    {
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(EBADF);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Capture pointers for completion block
    file_read_op* op_ptr = &op;
    gcd_file_service* svc_ptr = &svc_;

    // Submit read at specified offset via dispatch_io
    dispatch_io_read(
        channel_,
        static_cast<off_t>(op.file_offset),
        op.buffer_size,
        svc_.io_queue(),
        ^(bool done, dispatch_data_t data, int error) {
            if (done)
            {
                // Operation complete
                ssize_t result;

                if (error)
                {
                    // Error occurred
                    result = -error;
                }
                else if (data)
                {
                    // Success - get data size
                    size_t data_size = dispatch_data_get_size(data);

                    // Copy data to user's buffer
                    void* buffer = op_ptr->buffer_ptr;
                    __block size_t offset = 0;

                    dispatch_data_apply(data, ^bool(dispatch_data_t region,
                        size_t region_offset, const void* region_buffer, size_t region_size) {
                        memcpy(static_cast<char*>(buffer) + offset,
                               region_buffer, region_size);
                        offset += region_size;
                        return true;
                    });

                    result = static_cast<ssize_t>(data_size);
                }
                else
                {
                    // No data and no error means EOF
                    result = 0;
                }

                // Post to scheduler with result
                svc_ptr->work_finished();
                svc_ptr->sched_.post(op_ptr, result, 0);
            }
        });
}

std::coroutine_handle<>
gcd_file_impl_internal::write_some(
    std::coroutine_handle<> h,
    boost::capy::executor_ref ex,
    boost::corosio::io_buffer_param buffers,
    std::stop_token token,
    std::error_code* ec,
    std::size_t* bytes_transferred)
{
    // Keep internal alive during I/O
    wr_.internal_ptr = shared_from_this();

    auto& op = wr_;
    op.reset();
    op.h = h;
    op.ex = ex;
    op.ec_out = ec;
    op.bytes_out = bytes_transferred;
    op.start(token);

    // Prepare buffer
    boost::capy::mutable_buffer bufs[1];
    auto buf_count = buffers.copy_to(bufs, 1);

    // Handle empty buffer: complete immediately with 0 bytes
    if (buf_count == 0)
    {
        op.bytes_transferred = 0;
        op.empty_buffer = true;
        if (ec)
            *ec = std::error_code();
        if (bytes_transferred)
            *bytes_transferred = 0;
        svc_.sched_.post(&op);
        return std::noop_coroutine();
    }

    // Store buffer information in operation
    op.buffer_ptr = bufs[0].data();
    op.buffer_size = bufs[0].size();
    op.file_offset = position_;

    // Symmetric transfer to initiator - I/O starts after caller is suspended
    return write_initiator_.start<&gcd_file_impl_internal::do_write_io>(this);
}

void
gcd_file_impl_internal::do_write_io()
{
    auto& op = wr_;

    svc_.work_started();

    // Check if channel is open
    if (!channel_)
    {
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(EBADF);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Create dispatch_data from buffer
    dispatch_data_t data = dispatch_data_create(
        op.buffer_ptr,
        op.buffer_size,
        nullptr,
        DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    // Capture pointers for completion block
    file_write_op* op_ptr = &op;
    gcd_file_service* svc_ptr = &svc_;

    // Submit write at specified offset via dispatch_io
    dispatch_io_write(
        channel_,
        static_cast<off_t>(op.file_offset),
        data,
        svc_.io_queue(),
        ^(bool done, dispatch_data_t remaining_data, int error) {
            if (done)
            {
                // Operation complete
                ssize_t result;

                if (error)
                {
                    // Error occurred
                    result = -error;
                }
                else
                {
                    // Success - calculate bytes written
                    size_t bytes_written = op_ptr->buffer_size;
                    if (remaining_data)
                    {
                        // Some data wasn't written
                        bytes_written -= dispatch_data_get_size(remaining_data);
                    }
                    result = static_cast<ssize_t>(bytes_written);
                }

                // Post to scheduler with result
                svc_ptr->work_finished();
                svc_ptr->sched_.post(op_ptr, result, 0);
            }
        });

    // Release the dispatch_data (GCD retains it internally)
    dispatch_release(data);
}

void
gcd_file_impl_internal::cancel() noexcept
{
    if (channel_)
    {
        // Close the channel with DISPATCH_IO_STOP to cancel pending operations
        // Pending operations will complete with ECANCELED
        dispatch_io_close(channel_, DISPATCH_IO_STOP);
    }
}

void
gcd_file_impl_internal::close_file() noexcept
{
    // Close dispatch_io channel first
    close_channel();

    // Then close the file descriptor
    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
    }
    position_ = 0;
}

//------------------------------------------------------------------------------
// gcd_file_impl

void
gcd_file_impl::release()
{
    if (internal_)
    {
        auto& svc = internal_->svc_;
        internal_->release_internal();
        internal_.reset();
        svc.destroy_impl(*this);
    }
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_KQUEUE

#endif // __APPLE__
