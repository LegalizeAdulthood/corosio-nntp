#ifdef __linux__

#include <fileio/detail/uring_file_impl.h>
#include <fileio/detail/uring_file_service.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_EPOLL

#include "src/detail/make_err.hpp"

#include <unistd.h>
#include <liburing.h>

namespace nntp::detail {

//------------------------------------------------------------------------------
// uring_file_impl_internal

uring_file_impl_internal::uring_file_impl_internal(uring_file_service& svc) noexcept
    : svc_(svc)
    , rd_(*this)
    , wr_(*this)
{
}

uring_file_impl_internal::~uring_file_impl_internal()
{
    svc_.unregister_impl(*this);
}

void
uring_file_impl_internal::release_internal()
{
    // Cancel pending I/O before closing
    if (fd_ != -1)
    {
        cancel();
    }
    close_file();
}

std::coroutine_handle<>
uring_file_impl_internal::read_some(
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
    return read_initiator_.start<&uring_file_impl_internal::do_read_io>(this);
}

void
uring_file_impl_internal::do_read_io()
{
    auto& op = rd_;

    svc_.work_started();

    // Get io_uring instance
    io_uring* ring = svc_.native_handle();

    // Get SQE (Submission Queue Entry) from io_uring
    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    if (!sqe)
    {
        // No SQE available - this is a fatal error
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(ENOMEM);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Prepare read operation at specified file offset
    io_uring_prep_read(sqe, fd_, op.buffer_ptr, op.buffer_size, op.file_offset);

    // Set user_data to operation pointer for completion identification
    io_uring_sqe_set_data(sqe, &op);

    // Submit to kernel
    int ret = io_uring_submit(ring);
    if (ret < 0)
    {
        // Submission failed - complete with error
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(-ret);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Successfully submitted - kernel will perform async I/O
    // Completion will be delivered via io_uring CQ and processed by poll_completions()
}

std::coroutine_handle<>
uring_file_impl_internal::write_some(
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
    return write_initiator_.start<&uring_file_impl_internal::do_write_io>(this);
}

void
uring_file_impl_internal::do_write_io()
{
    auto& op = wr_;

    svc_.work_started();

    // Get io_uring instance
    io_uring* ring = svc_.native_handle();

    // Get SQE (Submission Queue Entry) from io_uring
    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    if (!sqe)
    {
        // No SQE available - this is a fatal error
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(ENOMEM);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Prepare write operation at specified file offset
    io_uring_prep_write(sqe, fd_, op.buffer_ptr, op.buffer_size, op.file_offset);

    // Set user_data to operation pointer for completion identification
    io_uring_sqe_set_data(sqe, &op);

    // Submit to kernel
    int ret = io_uring_submit(ring);
    if (ret < 0)
    {
        // Submission failed - complete with error
        svc_.work_finished();
        if (op.ec_out)
            *op.ec_out = boost::corosio::detail::make_err(-ret);
        if (op.bytes_out)
            *op.bytes_out = 0;
        svc_.sched_.post(&op);
        return;
    }

    // Successfully submitted - kernel will perform async I/O
    // Completion will be delivered via io_uring CQ and processed by poll_completions()
}

void
uring_file_impl_internal::cancel() noexcept
{
    if (fd_ != -1)
    {
        // Submit cancel operations for pending I/O
        file_read_op::do_cancel_impl(&rd_);
        file_write_op::do_cancel_impl(&wr_);
    }
}

void
uring_file_impl_internal::close_file() noexcept
{
    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
    }
    position_ = 0;
}

//------------------------------------------------------------------------------
// uring_file_impl

void
uring_file_impl::release()
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

#endif // BOOST_COROSIO_HAS_EPOLL

#endif // __linux__
