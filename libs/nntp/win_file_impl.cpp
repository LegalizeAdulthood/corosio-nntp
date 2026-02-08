#ifdef _WIN32

#include <nntp/detail/win_file_impl.h>
#include <nntp/detail/win_file_service.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_IOCP

#include "src/detail/make_err.hpp"

namespace nntp::detail {

//------------------------------------------------------------------------------
// win_file_impl_internal

win_file_impl_internal::win_file_impl_internal(win_file_service& svc) noexcept
    : svc_(svc)
    , rd_(*this)
    , wr_(*this)
{
}

win_file_impl_internal::~win_file_impl_internal()
{
    svc_.unregister_impl(*this);
}

void
win_file_impl_internal::release_internal()
{
    // Cancel pending I/O before closing to ensure operations
    // complete with ERROR_OPERATION_ABORTED via IOCP
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(handle_, nullptr);
    }
    close_file();
}

std::coroutine_handle<>
win_file_impl_internal::read_some(
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
    op.is_read_ = true;
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
        op.dwError = 0;
        op.empty_buffer = true;
        svc_.post(&op);
        return std::noop_coroutine();
    }

    // Store buffer information in operation
    op.buffer_ptr = bufs[0].data();
    op.buffer_size = static_cast<DWORD>(bufs[0].size());
    op.file_offset = position_;

    // Set file offset in OVERLAPPED structure
    op.Offset = static_cast<DWORD>(position_);
    op.OffsetHigh = static_cast<DWORD>(position_ >> 32);

    // Symmetric transfer to initiator - I/O starts after caller is suspended
    return read_initiator_.start<&win_file_impl_internal::do_read_io>(this);
}

void
win_file_impl_internal::do_read_io()
{
    auto& op = rd_;

    svc_.work_started();

    DWORD bytes_read = 0;
    BOOL result = ::ReadFile(
        handle_,
        op.buffer_ptr,
        op.buffer_size,
        &bytes_read,
        &op);

    if (result)
    {
        // Synchronous completion - but IOCP will still deliver completion packet
        // Do nothing here, let IOCP handle it
    }
    else
    {
        DWORD error = ::GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            // Immediate error - complete via post
            svc_.work_finished();
            op.dwError = error;
            svc_.post(&op);
            return;
        }
    }
    // Async operation in progress or sync completion - IOCP will deliver packet
}

std::coroutine_handle<>
win_file_impl_internal::write_some(
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
        op.dwError = 0;
        svc_.post(&op);
        return std::noop_coroutine();
    }

    // Store buffer information in operation
    op.buffer_ptr = bufs[0].data();
    op.buffer_size = static_cast<DWORD>(bufs[0].size());
    op.file_offset = position_;

    // Set file offset in OVERLAPPED structure
    op.Offset = static_cast<DWORD>(position_);
    op.OffsetHigh = static_cast<DWORD>(position_ >> 32);

    // Symmetric transfer to initiator - I/O starts after caller is suspended
    return write_initiator_.start<&win_file_impl_internal::do_write_io>(this);
}

void
win_file_impl_internal::do_write_io()
{
    auto& op = wr_;

    svc_.work_started();

    DWORD bytes_written = 0;
    BOOL result = ::WriteFile(
        handle_,
        op.buffer_ptr,
        op.buffer_size,
        &bytes_written,
        &op);

    if (result)
    {
        // Synchronous completion - but IOCP will still deliver completion packet
        // Do nothing here, let IOCP handle it
    }
    else
    {
        DWORD error = ::GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            // Immediate error - complete via post
            svc_.work_finished();
            op.dwError = error;
            svc_.post(&op);
            return;
        }
    }
    // Async operation in progress or sync completion - IOCP will deliver packet
}

void
win_file_impl_internal::cancel() noexcept
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(handle_, nullptr);
    }

    rd_.request_cancel();
    wr_.request_cancel();
}

void
win_file_impl_internal::close_file() noexcept
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    position_ = 0;
}

//------------------------------------------------------------------------------
// win_file_impl

void
win_file_impl::release()
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

#endif // BOOST_COROSIO_HAS_IOCP

#endif // _WIN32
