#ifndef NNTP_DETAIL_WIN_FILE_IMPL_H
#define NNTP_DETAIL_WIN_FILE_IMPL_H

#ifdef _WIN32

#include <fileio/detail/file_ops.h>
#include <boost/corosio/io_stream.hpp>
#include <boost/corosio/io_buffer_param.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/coro.hpp>
#include "src/detail/intrusive.hpp"
#include "src/detail/cached_initiator.hpp"
#include <memory>
#include <cstdint>
#include <stop_token>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nntp::detail {

class win_file_service;
class win_file_impl;

/** Internal file state for IOCP-based I/O.

    This class contains the actual state for a single file, including
    the native file handle and pending operations. It derives from
    enable_shared_from_this so operations can extend its lifetime.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class win_file_impl_internal
    : public boost::corosio::detail::intrusive_list<win_file_impl_internal>::node
    , public std::enable_shared_from_this<win_file_impl_internal>
{
    friend class win_file_service;
    friend class win_file_impl;
    friend struct file_read_op;
    friend struct file_write_op;

public:
    explicit win_file_impl_internal(win_file_service& svc) noexcept;
    ~win_file_impl_internal();

    /** Called by wrapper's destructor. */
    void release_internal();

    /** Asynchronously read data from the file.

        @param h Coroutine handle to resume
        @param ex Executor to resume on
        @param buffers Buffer sequence to read into
        @param token Cancellation token
        @param ec Output error code
        @param bytes_transferred Output bytes transferred

        @return Coroutine handle to resume (may be noop_coroutine if suspended)
    */
    std::coroutine_handle<> read_some(
        std::coroutine_handle<> h,
        boost::capy::executor_ref ex,
        boost::corosio::io_buffer_param buffers,
        std::stop_token token,
        std::error_code* ec,
        std::size_t* bytes_transferred);

    /** Asynchronously write data to the file.

        @param h Coroutine handle to resume
        @param ex Executor to resume on
        @param buffers Buffer sequence to write from
        @param token Cancellation token
        @param ec Output error code
        @param bytes_transferred Output bytes transferred

        @return Coroutine handle to resume (may be noop_coroutine if suspended)
    */
    std::coroutine_handle<> write_some(
        std::coroutine_handle<> h,
        boost::capy::executor_ref ex,
        boost::corosio::io_buffer_param buffers,
        std::stop_token token,
        std::error_code* ec,
        std::size_t* bytes_transferred);

    /** Get the native file handle. */
    HANDLE native_handle() const noexcept { return handle_; }

    /** Get the current file position. */
    std::uint64_t position() const noexcept { return position_; }

    /** Set the file position for next I/O operation. */
    void set_position(std::uint64_t pos) noexcept { position_ = pos; }

    /** Check if the file is open. */
    bool is_open() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }

    /** Cancel pending I/O operations. */
    void cancel() noexcept;

    /** Close the file handle. */
    void close_file() noexcept;

    /** Set the file handle (used by service during open). */
    void set_handle(HANDLE h) noexcept { handle_ = h; }

    /** Execute the read I/O operation (called by initiator coroutine). */
    void do_read_io();

    /** Execute the write I/O operation (called by initiator coroutine). */
    void do_write_io();

private:
    win_file_service& svc_;
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::uint64_t position_ = 0;

    // Operation states
    file_read_op rd_;
    file_write_op wr_;

    // Async operation management
    boost::corosio::detail::cached_initiator read_initiator_;
    boost::corosio::detail::cached_initiator write_initiator_;
};

/** File implementation wrapper for IOCP-based I/O.

    This class is the public-facing file impl that holds a shared_ptr
    to the internal state. The shared_ptr is hidden from the public interface.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class win_file_impl
    : public boost::corosio::io_stream::io_stream_impl
    , public boost::corosio::detail::intrusive_list<win_file_impl>::node
{
public:
    explicit win_file_impl(std::shared_ptr<win_file_impl_internal> internal) noexcept
        : internal_(std::move(internal))
    {
    }

    void release() override;

    std::coroutine_handle<> read_some(
        std::coroutine_handle<> h,
        boost::capy::executor_ref ex,
        boost::corosio::io_buffer_param buffers,
        std::stop_token token,
        std::error_code* ec,
        std::size_t* bytes_transferred) override
    {
        return internal_->read_some(h, ex, buffers, token, ec, bytes_transferred);
    }

    std::coroutine_handle<> write_some(
        std::coroutine_handle<> h,
        boost::capy::executor_ref ex,
        boost::corosio::io_buffer_param buffers,
        std::stop_token token,
        std::error_code* ec,
        std::size_t* bytes_transferred) override
    {
        return internal_->write_some(h, ex, buffers, token, ec, bytes_transferred);
    }

    void cancel() noexcept
    {
        internal_->cancel();
    }

    win_file_impl_internal* get_internal() const noexcept
    {
        return internal_.get();
    }

private:
    std::shared_ptr<win_file_impl_internal> internal_;
};

} // namespace nntp::detail

#endif // _WIN32

#endif // NNTP_DETAIL_WIN_FILE_IMPL_H
