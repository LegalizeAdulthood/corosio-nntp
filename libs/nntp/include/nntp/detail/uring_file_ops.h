#ifndef NNTP_DETAIL_URING_FILE_OPS_H
#define NNTP_DETAIL_URING_FILE_OPS_H

#ifdef __linux__

#include <boost/corosio/detail/config.hpp>
#include "src/detail/scheduler_op.hpp"

#include <liburing.h>
#include <sys/types.h>
#include <cstdint>
#include <memory>
#include <utility>

namespace boost::corosio::detail
{
    struct scheduler_op;
}

namespace nntp::detail
{

class uring_file_impl_internal;

/** File read operation state for io_uring.

    This structure represents a single read operation on a file.
    It holds the io_uring submission state and acts as the user_data
    pointer for completion queue entries.

    @note Internal implementation detail.
*/
struct file_read_op : boost::corosio::detail::io_awaitable_op
{
    /** Buffer pointer for the read operation. */
    void* buffer_ptr = nullptr;

    /** Size of the buffer in bytes. */
    std::size_t buffer_size = 0;

    /** File offset for this read operation. */
    off_t file_offset = 0;

    /** Reference to the internal implementation. */
    uring_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<uring_file_impl_internal> internal_ptr;

    /** Completion callback invoked when CQE arrives.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param res Result from io_uring CQE (bytes or -errno)
        @param flags Flags from CQE
    */
    static void do_complete(
        void* owner,
        boost::corosio::detail::scheduler_op* base,
        std::uint32_t res,
        std::uint32_t flags);

    /** Cancellation callback.

        Called when the operation needs to be cancelled.

        @param op Pointer to this operation
    */
    static void do_cancel_impl(file_read_op* op) noexcept;

    /** Construct read operation.

        @param internal_ Reference to the file implementation
    */
    explicit file_read_op(uring_file_impl_internal& internal_) noexcept;
};

/** File write operation state for io_uring.

    This structure represents a single write operation on a file.
    It holds the io_uring submission state and acts as the user_data
    pointer for completion queue entries.

    @note Internal implementation detail.
*/
struct file_write_op : boost::corosio::detail::scheduler_op
{
    /** Buffer pointer for the write operation. */
    const void* buffer_ptr = nullptr;

    /** Size of the buffer in bytes. */
    std::size_t buffer_size = 0;

    /** File offset for this write operation. */
    off_t file_offset = 0;

    /** Reference to the internal implementation. */
    uring_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<uring_file_impl_internal> internal_ptr;

    /** Output parameters for operation results. */
    std::error_code* ec_out = nullptr;
    std::size_t* bytes_out = nullptr;

    /** Coroutine handle to resume. */
    std::coroutine_handle<> handler_;

    /** Cleanup without resuming coroutine. */
    void cleanup_only() noexcept {}

    /** Resume the coroutine. */
    void invoke_handler() noexcept
    {
        if (handler_)
            handler_.resume();
    }

    /** Completion callback invoked when CQE arrives.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param res Result from io_uring CQE (bytes or -errno)
        @param flags Flags from CQE
    */
    static void do_complete(
        void* owner,
        boost::corosio::detail::scheduler_op* base,
        std::uint32_t res,
        std::uint32_t flags);

    /** Cancellation callback.

        Called when the operation needs to be cancelled.

        @param op Pointer to this operation
    */
    static void do_cancel_impl(file_write_op* op) noexcept;

    /** Construct write operation.

        @param internal_ Reference to the file implementation
    */
    explicit file_write_op(uring_file_impl_internal& internal_) noexcept;
};

} // namespace nntp::detail

#endif // __linux__

#endif // NNTP_DETAIL_URING_FILE_OPS_H
