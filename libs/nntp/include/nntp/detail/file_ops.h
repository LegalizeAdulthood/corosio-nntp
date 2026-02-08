#ifndef NNTP_DETAIL_FILE_OPS_H
#define NNTP_DETAIL_FILE_OPS_H

#ifdef _WIN32

#include <boost/corosio/detail/config.hpp>
#include "src/detail/iocp/overlapped_op.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <memory>
#include <cstdint>

namespace boost::corosio::detail
{
    struct overlapped_op;
    struct scheduler_op;
}

namespace nntp::detail
{

class win_file_impl_internal;

/** File read operation state.

    This structure represents a single read operation on a file.
    It contains the OVERLAPPED structure required for async I/O
    and stores operation-specific state.

    @note Internal implementation detail.
*/
struct file_read_op : boost::corosio::detail::overlapped_op
{
    /** Buffer pointer for the read operation. */
    void* buffer_ptr = nullptr;

    /** Size of the buffer in bytes. */
    DWORD buffer_size = 0;

    /** File offset for this read operation. */
    std::uint64_t file_offset = 0;

    /** Reference to the internal implementation. */
    win_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<win_file_impl_internal> internal_ptr;

    /** Completion callback invoked by IOCP thread.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param bytes Number of bytes transferred
        @param error Windows error code (0 for success)
    */
    static void do_complete(
        void* owner,
        boost::corosio::detail::scheduler_op* base,
        std::uint32_t bytes,
        std::uint32_t error);

    /** Cancellation callback.

        Called when the operation needs to be cancelled.

        @param op Pointer to this operation
    */
    static void do_cancel_impl(
        boost::corosio::detail::overlapped_op* op) noexcept;

    /** Construct read operation.

        @param internal_ Reference to the file implementation
    */
    explicit file_read_op(win_file_impl_internal& internal_) noexcept;
};

/** File write operation state.

    This structure represents a single write operation on a file.
    It contains the OVERLAPPED structure required for async I/O
    and stores operation-specific state.

    @note Internal implementation detail.
*/
struct file_write_op : boost::corosio::detail::overlapped_op
{
    /** Buffer pointer for the write operation. */
    const void* buffer_ptr = nullptr;

    /** Size of the buffer in bytes. */
    DWORD buffer_size = 0;

    /** File offset for this write operation. */
    std::uint64_t file_offset = 0;

    /** Reference to the internal implementation. */
    win_file_impl_internal& internal;

    /** Shared pointer to keep internal alive during async operation. */
    std::shared_ptr<win_file_impl_internal> internal_ptr;

    /** Completion callback invoked by IOCP thread.

        @param owner Pointer to the service that owns this operation
        @param base Pointer to the base scheduler_op (this operation)
        @param bytes Number of bytes transferred
        @param error Windows error code (0 for success)
    */
    static void do_complete(
        void* owner,
        boost::corosio::detail::scheduler_op* base,
        std::uint32_t bytes,
        std::uint32_t error);

    /** Cancellation callback.

        Called when the operation needs to be cancelled.

        @param op Pointer to this operation
    */
    static void do_cancel_impl(
        boost::corosio::detail::overlapped_op* op) noexcept;

    /** Construct write operation.

        @param internal_ Reference to the file implementation
    */
    explicit file_write_op(win_file_impl_internal& internal_) noexcept;
};

} // namespace nntp::detail

#endif // _WIN32

#endif // NNTP_DETAIL_FILE_OPS_H
