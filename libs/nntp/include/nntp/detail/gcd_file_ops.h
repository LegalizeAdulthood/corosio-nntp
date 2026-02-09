#ifndef NNTP_DETAIL_GCD_FILE_OPS_H
#define NNTP_DETAIL_GCD_FILE_OPS_H

#ifdef __APPLE__

#include <boost/corosio/detail/config.hpp>
#include "src/detail/scheduler_op.hpp"

#include <dispatch/dispatch.h>
#include <sys/types.h>
#include <coroutine>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>

namespace boost::corosio::detail
{
    struct scheduler_op;
}

namespace nntp::detail
{

class gcd_file_impl_internal;

/** File read operation state for GCD dispatch_io.

    This structure represents a single read operation on a file.
    It holds the dispatch_io state and is captured by the
    completion block for result processing.

    @note Internal implementation detail.
*/
struct file_read_op : boost::corosio::detail::scheduler_op
{
    /** Buffer pointer for the read operation. */
    void* buffer_ptr = nullptr;

    /** Size of the buffer in bytes. */
    std::size_t buffer_size = 0;

    /** File offset for this read operation. */
    off_t file_offset = 0;

    /** Reference to the internal implementation. */
    gcd_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<gcd_file_impl_internal> internal_ptr;

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

    /** Completion callback invoked when GCD operation completes.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param res Result from GCD (bytes or -errno)
        @param flags Flags (unused)
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
    explicit file_read_op(gcd_file_impl_internal& internal_) noexcept;
};

/** File write operation state for GCD dispatch_io.

    This structure represents a single write operation on a file.
    It holds the dispatch_io state and is captured by the
    completion block for result processing.

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
    gcd_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<gcd_file_impl_internal> internal_ptr;

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

    /** Completion callback invoked when GCD operation completes.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param res Result from GCD (bytes or -errno)
        @param flags Flags (unused)
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
    explicit file_write_op(gcd_file_impl_internal& internal_) noexcept;
};

} // namespace nntp::detail

#endif // __APPLE__

#endif // NNTP_DETAIL_GCD_FILE_OPS_H
