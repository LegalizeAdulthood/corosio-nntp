#ifdef __linux__

#include <nntp/detail/uring_file_ops.h>
#include <nntp/detail/uring_file_impl.h>
#include <nntp/detail/uring_file_service.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_EPOLL

#include "src/detail/make_err.hpp"
#include "src/detail/resume_coro.hpp"
#include <boost/capy/cond.hpp>

#include <liburing.h>

namespace nntp::detail {

//------------------------------------------------------------------------------
// Operation constructors

file_read_op::file_read_op(uring_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::scheduler_op(&do_complete)
    , internal(internal_)
{
}

file_write_op::file_write_op(uring_file_impl_internal& internal_) noexcept
    : boost::corosio::detail::scheduler_op(&do_complete)
    , internal(internal_)
{
}

//------------------------------------------------------------------------------
// Cancellation functions

void file_read_op::do_cancel_impl(file_read_op* op) noexcept
{
    // Cancel the I/O operation using io_uring
    // io_uring_prep_cancel submits a cancellation request
    if (op->internal.is_open())
    {
        io_uring* ring = op->internal.svc_.native_handle();

        // Get an SQE for the cancel operation
        io_uring_sqe* sqe = io_uring_get_sqe(ring);
        if (sqe)
        {
            // Prepare cancel operation targeting this operation
            io_uring_prep_cancel(sqe, op, 0);

            // Submit the cancel request
            io_uring_submit(ring);
        }
    }
}

void file_write_op::do_cancel_impl(file_write_op* op) noexcept
{
    // Cancel the I/O operation using io_uring
    if (op->internal.is_open())
    {
        io_uring* ring = op->internal.svc_.native_handle();

        // Get an SQE for the cancel operation
        io_uring_sqe* sqe = io_uring_get_sqe(ring);
        if (sqe)
        {
            // Prepare cancel operation targeting this operation
            io_uring_prep_cancel(sqe, op, 0);

            // Submit the cancel request
            io_uring_submit(ring);
        }
    }
}

//------------------------------------------------------------------------------
// file_read_op completion handler

void
file_read_op::do_complete(
void* owner,
boost::corosio::detail::scheduler_op* base,
std::uint32_t res,
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
    // The res parameter from io_uring CQE contains:
    // - Positive value: number of bytes transferred
    // - Zero: EOF for read operations
    // - Negative value: -errno error code
    auto result = static_cast<std::int32_t>(res);

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Process result
    if (result >= 0)
    {
        // Success: result is bytes transferred
        if (op->bytes_out)
            *op->bytes_out = static_cast<std::size_t>(result);

        // Update file position after successful read
        if (result > 0)
        {
            op->internal.position_ += result;
        }

        // Check for EOF
        if (result == 0)
        {
            if (op->ec_out)
                *op->ec_out = std::error_code(static_cast<int>(boost::capy::cond::eof),
                                               boost::capy::detail::cond_cat);
        }
        else
        {
            if (op->ec_out)
                *op->ec_out = std::error_code();
        }
    }
    else
    {
        // Error: result is -errno
        if (op->bytes_out)
            *op->bytes_out = 0;

        if (op->ec_out)
        {
            // Check for cancellation
            if (-result == ECANCELED)
            {
                *op->ec_out = std::error_code(static_cast<int>(boost::capy::cond::canceled),
                                               boost::capy::detail::cond_cat);
            }
            else
            {
                *op->ec_out = boost::corosio::detail::make_err(-result);
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
std::uint32_t res,
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
    // The res parameter from io_uring CQE contains:
    // - Positive value: number of bytes transferred
    // - Negative value: -errno
    auto result = static_cast<std::int32_t>(res);

    // Hold shared_ptr to prevent premature destruction of internal
    auto prevent_premature_destruction = std::move(op->internal_ptr);

    // Process result
    if (result >= 0)
    {
        // Success: result is bytes transferred
        if (op->bytes_out)
            *op->bytes_out = static_cast<std::size_t>(result);

        // Update file position after successful write
        if (result > 0)
        {
            op->internal.position_ += result;
        }

        if (op->ec_out)
            *op->ec_out = std::error_code();
    }
    else
    {
        // Error: result is -errno
        if (op->bytes_out)
            *op->bytes_out = 0;

        if (op->ec_out)
        {
            // Check for cancellation
            if (-result == ECANCELED)
            {
                *op->ec_out = std::error_code(static_cast<int>(boost::capy::cond::canceled),
                                               boost::capy::detail::cond_cat);
            }
            else
            {
                *op->ec_out = boost::corosio::detail::make_err(-result);
            }
        }
    }

    // Resume the user's coroutine
    op->invoke_handler();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_EPOLL

#endif // __linux__
