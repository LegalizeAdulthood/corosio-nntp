#ifdef _WIN32

#include <nntp/detail/file_ops.h>
#include <nntp/detail/win_file_impl.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_IOCP

#include "src/detail/make_err.hpp"
#include "src/detail/resume_coro.hpp"

namespace nntp::detail {

//------------------------------------------------------------------------------
// Operation constructors

file_read_op::file_read_op(win_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::overlapped_op(&do_complete)
    , internal(internal_)
{
    cancel_func_ = &do_cancel_impl;
}

file_write_op::file_write_op(win_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::overlapped_op(&do_complete)
    , internal(internal_)
{
    cancel_func_ = &do_cancel_impl;
}

//------------------------------------------------------------------------------
// Cancellation functions

void file_read_op::do_cancel_impl(
    boost::corosio::detail::overlapped_op* base) noexcept
{
    auto* op = static_cast<file_read_op*>(base);

    // Cancel the I/O operation on the file handle
    // CancelIoEx cancels pending overlapped operations for this handle
    if (op->internal.is_open())
    {
        ::CancelIoEx(
            op->internal.native_handle(),
            static_cast<LPOVERLAPPED>(base));
    }
}

void file_write_op::do_cancel_impl(
    boost::corosio::detail::overlapped_op* base) noexcept
{
    auto* op = static_cast<file_write_op*>(base);

    // Cancel the I/O operation on the file handle
    if (op->internal.is_open())
    {
        ::CancelIoEx(
            op->internal.native_handle(),
            static_cast<LPOVERLAPPED>(base));
    }
}

//------------------------------------------------------------------------------
// file_read_op completion handler

void
file_read_op::do_complete(
    void* owner,
    boost::corosio::detail::scheduler_op* base,
    std::uint32_t /*bytes*/,
    std::uint32_t /*error*/)
{
    auto* op = static_cast<file_read_op*>(base);

    // Destroy path - called when the io_context is shutting down
    if (!owner)
    {
        op->cleanup_only();
        op->internal_ptr.reset();
        return;
    }

    // Normal completion path
    // The overlapped_op base class invoke_handler() will:
    // 1. Check if cancelled and set ec_out to capy::error::canceled
    // 2. Check dwError and convert to std::error_code
    // 3. Check for EOF (bytes_transferred == 0 on read)
    // 4. Write bytes_transferred to bytes_out
    // 5. Resume the coroutine on the proper executor

    // Update file position after successful read
    // This must happen before invoke_handler() so the position is correct
    // when the user's coroutine resumes
    if (op->dwError == 0 && op->bytes_transferred > 0)
    {
        op->internal.position_ += op->bytes_transferred;
    }

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Resume the user's coroutine
    op->invoke_handler();
}

//------------------------------------------------------------------------------
// file_write_op completion handler

void
file_write_op::do_complete(
    void* owner,
    boost::corosio::detail::scheduler_op* base,
    std::uint32_t /*bytes*/,
    std::uint32_t /*error*/)
{
    auto* op = static_cast<file_write_op*>(base);

    // Destroy path - called when the io_context is shutting down
    if (!owner)
    {
        op->cleanup_only();
        op->internal_ptr.reset();
        return;
    }

    // Normal completion path
    // The overlapped_op base class invoke_handler() will:
    // 1. Check if cancelled and set ec_out to capy::error::canceled
    // 2. Check dwError and convert to std::error_code
    // 3. Write bytes_transferred to bytes_out
    // 4. Resume the coroutine on the proper executor

    // Update file position after successful write
    // This must happen before invoke_handler() so the position is correct
    // when the user's coroutine resumes
    if (op->dwError == 0 && op->bytes_transferred > 0)
    {
        op->internal.position_ += op->bytes_transferred;
    }

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Resume the user's coroutine
    op->invoke_handler();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_IOCP

#endif // _WIN32
