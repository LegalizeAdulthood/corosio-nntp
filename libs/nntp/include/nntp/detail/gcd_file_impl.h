#ifndef NNTP_DETAIL_GCD_FILE_IMPL_H
#define NNTP_DETAIL_GCD_FILE_IMPL_H

#ifdef __APPLE__

#include <nntp/detail/gcd_file_ops.h>
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

#include <dispatch/dispatch.h>

namespace nntp::detail {

class gcd_file_service;
class gcd_file_impl;

/** Internal file state for GCD-based I/O.

    This class contains the actual state for a single file, including
    the native file descriptor, dispatch_io channel, and pending operations.
    It derives from enable_shared_from_this so operations can extend its lifetime.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class gcd_file_impl_internal
    : public boost::corosio::detail::intrusive_list<gcd_file_impl_internal>::node
    , public std::enable_shared_from_this<gcd_file_impl_internal>
{
    friend class gcd_file_service;
    friend class gcd_file_impl;
    friend struct file_read_op;
    friend struct file_write_op;

public:
    explicit gcd_file_impl_internal(gcd_file_service& svc) noexcept;
    ~gcd_file_impl_internal();

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

    /** Get the native file descriptor. */
    int native_handle() const noexcept { return fd_; }

    /** Get the current file position. */
    std::uint64_t position() const noexcept { return position_; }

    /** Set the file position for next I/O operation. */
    void set_position(std::uint64_t pos) noexcept { position_ = pos; }

    /** Check if the file is open. */
    bool is_open() const noexcept { return fd_ != -1; }

    /** Cancel pending I/O operations. */
    void cancel() noexcept;

private:
    /** Set the file descriptor. */
    void set_fd(int fd) noexcept { fd_ = fd; }

    /** Close the file. */
    void close_file() noexcept;

    /** Create dispatch_io channel for the file. */
    void open_channel();

    /** Close dispatch_io channel. */
    void close_channel() noexcept;

    /** Submit a read operation via dispatch_io. */
    void do_read_io();

    /** Submit a write operation via dispatch_io. */
    void do_write_io();

    /** Reference to the service. */
    gcd_file_service& svc_;

    /** Native file descriptor. */
    int fd_ = -1;

    /** Current file position. */
    std::uint64_t position_ = 0;

    /** dispatch_io channel for this file. */
    dispatch_io_t channel_ = nullptr;

    /** Read operation state. */
    file_read_op rd_;

    /** Write operation state. */
    file_write_op wr_;

    /** Cached initiator for read operations. */
    boost::corosio::detail::cached_initiator read_initiator_;

    /** Cached initiator for write operations. */
    boost::corosio::detail::cached_initiator write_initiator_;
};

/** Wrapper for file implementation (io_stream interface).

    This class provides the io_stream interface and delegates to
    the internal implementation. It uses a shared_ptr to manage
    the lifetime of the internal state.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class gcd_file_impl
    : public boost::corosio::detail::intrusive_list<gcd_file_impl>::node
    , public boost::corosio::io_stream
{
    friend class gcd_file_service;

public:
explicit gcd_file_impl(std::shared_ptr<gcd_file_impl_internal> internal)
    : io_stream(internal->svc_.context())
    , internal_(std::move(internal))
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

    gcd_file_impl_internal* get_internal() const noexcept
    {
        return internal_.get();
    }

private:
    std::shared_ptr<gcd_file_impl_internal> internal_;
};

} // namespace nntp::detail

#endif // __APPLE__

#endif // NNTP_DETAIL_GCD_FILE_IMPL_H
