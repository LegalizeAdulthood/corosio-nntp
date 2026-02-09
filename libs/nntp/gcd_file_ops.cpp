#ifdef __APPLE__

#include <nntp/detail/gcd_file_ops.h>
#include <nntp/detail/gcd_file_impl.h>
#include <nntp/detail/gcd_file_service.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_KQUEUE

#include "src/detail/make_err.hpp"
#include "src/detail/resume_coro.hpp"
#include <boost/capy/cond.hpp>

#include <dispatch/dispatch.h>

namespace nntp::detail {

//------------------------------------------------------------------------------
// Operation constructors

file_read_op::file_read_op(gcd_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::scheduler_op(&do_complete)
    , internal(internal_)
{
}

file_write_op::file_write_op(gcd_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::scheduler_op(&do_complete)
    , internal(internal_)
{
}

//------------------------------------------------------------------------------
// Cancellation functions

void file_read_op::do_cancel_impl(file_read_op* op) noexcept
{
    // Cancel the I/O operation by closing the dispatch_io channel
    // This will cause all pending operations to complete with ECANCELED
    if (op->internal.is_open() && op->internal.channel_)
    {
        dispatch_io_close(op->internal.channel_, DISPATCH_IO_STOP);
    }
}

void file_write_op::do_cancel_impl(file_write_op* op) noexcept
{
    // Cancel the I/O operation by closing the dispatch_io channel
    if (op->internal.is_open() && op->internal.channel_)
    {
        dispatch_io_close(op->internal.channel_, DISPATCH_IO_STOP);
    }
}

//------------------------------------------------------------------------------
// file_read_op completion handler

void
file_read_op::do_complete(
    void* owner,
    boost::corosio::detail::scheduler_op* base,
    std::int32_t res,
    std::uint32_t /*flags*/)
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
    // The res parameter contains:
    // - Positive value: number of bytes transferred
    // - Zero: EOF for read operations
    // - Negative value: -errno

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Process result
    if (res >= 0)
    {
        // Success: res is bytes transferred
        if (op->bytes_out)
            *op->bytes_out = static_cast<std::size_t>(res);

        // Update file position after successful read
        if (res > 0)
        {
            op->internal.position_ += res;
        }

        // Check for EOF
        if (res == 0)
        {
            if (op->ec_out)
                *op->ec_out = boost::capy::cond::eof;
        }
        else
        {
            if (op->ec_out)
                *op->ec_out = std::error_code();
        }
    }
    else
    {
        // Error: res is -errno
        if (op->bytes_out)
            *op->bytes_out = 0;

        if (op->ec_out)
        {
            // Check for cancellation
            if (-res == ECANCELED)
            {
                *op->ec_out = boost::capy::cond::canceled;
            }
            else
            {
                *op->ec_out = boost::corosio::detail::make_err(-res);
            }
        }
    }

    // Resume the user's coroutine
    op->invoke_handler();
}

//------------------------------------------------------------------------------
// file_write_op completion handler

void
file_write_op::do_complete(
    void* owner,
    boost::corosio::detail::scheduler_op* base,
    std::int32_t res,
    std::uint32_t /*flags*/)
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
    // The res parameter contains:
    // - Positive value: number of bytes transferred
    // - Negative value: -errno

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Process result
    if (res >= 0)
    {
        // Success: res is bytes transferred
        if (op->bytes_out)
            *op->bytes_out = static_cast<std::size_t>(res);

        // Update file position after successful write
        if (res > 0)
        {
            op->internal.position_ += res;
        }

        if (op->ec_out)
            *op->ec_out = std::error_code();
    }
    else
    {
        // Error: res is -errno
        if (op->bytes_out)
            *op->bytes_out = 0;

        if (op->ec_out)
        {
            // Check for cancellation
            if (-res == ECANCELED)
            {
                *op->ec_out = boost::capy::cond::canceled;
            }
            else
            {
                *op->ec_out = boost::corosio::detail::make_err(-res);
            }
        }
    }

    // Resume the user's coroutine
    op->invoke_handler();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_KQUEUE

#endif // __APPLE__
