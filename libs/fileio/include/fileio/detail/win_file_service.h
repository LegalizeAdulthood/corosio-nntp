#ifndef NNTP_DETAIL_WIN_FILE_SERVICE_H
#define NNTP_DETAIL_WIN_FILE_SERVICE_H

#ifdef _WIN32

#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_IOCP

#include <boost/capy/ex/execution_context.hpp>
#include "src/detail/intrusive.hpp"
#include "src/detail/iocp/mutex.hpp"
#include <filesystem>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace boost::corosio::detail
{
    struct overlapped_op;
    class win_scheduler;
}

namespace nntp::detail
{

class win_file_impl;
class win_file_impl_internal;

/** Windows IOCP file management service.

    This service owns all file implementations and coordinates their
    lifecycle with the IOCP. It provides:

    - File implementation allocation and deallocation
    - IOCP handle association for files
    - Graceful shutdown - destroys all implementations when io_context stops

    @par Thread Safety
    All public member functions are thread-safe.

    @note Only available on Windows platforms.
*/
class win_file_service
    : public boost::capy::execution_context::service
{
public:
    using key_type = win_file_service;

    /** Construct the file service.

        Obtains the IOCP handle from the scheduler service.

        @param ctx Reference to the owning execution_context.
    */
    explicit win_file_service(boost::capy::execution_context& ctx);

    /** Destroy the file service. */
    ~win_file_service();

    win_file_service(win_file_service const&) = delete;
    win_file_service& operator=(win_file_service const&) = delete;

    /** Shut down the service. */
    void shutdown() override;

    /** Create a new file implementation wrapper.
        The service owns the returned object.
    */
    win_file_impl& create_impl();

    /** Destroy a file implementation wrapper.
        Removes from tracking list and deletes.
    */
    void destroy_impl(win_file_impl& impl);

    /** Unregister a file implementation from the service list.
        Called by the internal impl destructor.
    */
    void unregister_impl(win_file_impl_internal& impl);

    /** Open a file and associate with IOCP.

        @param impl The file implementation internal to initialize.
        @param path The path to the file to open.
        @param access Access mode (GENERIC_READ, GENERIC_WRITE, etc.).
        @param creation Creation disposition (CREATE_NEW, OPEN_EXISTING, etc.).
        @return Error code, or success.
    */
    std::error_code open_file(
        win_file_impl_internal& impl,
        std::filesystem::path const& path,
        DWORD access,
        DWORD creation);

    /** Return the IOCP handle. */
    void* native_handle() const noexcept { return iocp_; }

    /** Post an overlapped operation for completion. */
    void post(boost::corosio::detail::overlapped_op* op);

    /** Notify scheduler of pending I/O work. */
    void work_started() noexcept;

    /** Notify scheduler that I/O work completed. */
    void work_finished() noexcept;

private:
    boost::corosio::detail::win_scheduler& sched_;
    boost::corosio::detail::win_mutex mutex_;
    boost::corosio::detail::intrusive_list<win_file_impl_internal> file_list_;
    boost::corosio::detail::intrusive_list<win_file_impl> wrapper_list_;
    void* iocp_;
};

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_IOCP

#endif // _WIN32

#endif // NNTP_DETAIL_WIN_FILE_SERVICE_H
