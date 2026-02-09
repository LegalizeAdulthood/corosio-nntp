#ifndef NNTP_DETAIL_URING_FILE_SERVICE_H
#define NNTP_DETAIL_URING_FILE_SERVICE_H

#ifdef __linux__

#include <boost/corosio/detail/config.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include "src/detail/intrusive.hpp"

#include <liburing.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <system_error>

namespace boost::corosio::detail
{
    class scheduler;
}

namespace nntp::detail
{

class uring_file_impl_internal;
class uring_file_impl;

/** Service for managing io_uring-based file I/O operations.

    This service initializes and manages an io_uring instance for
    asynchronous file I/O. It integrates with the epoll scheduler
    by registering the io_uring file descriptor, allowing the
    scheduler to be notified when completions are available.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class uring_file_service : public boost::capy::execution_context::service
{
    friend class uring_file_impl_internal;

public:
    using key_type = uring_file_service;

    /** Construct the service.

        @param ctx Reference to the owning execution_context.
    */
    explicit uring_file_service(boost::capy::execution_context& ctx);

    /** Destroy the service. */
    ~uring_file_service();

    uring_file_service(uring_file_service const&) = delete;
    uring_file_service& operator=(uring_file_service const&) = delete;

    /** Shutdown the service.

        Closes all files and shuts down io_uring instance.
    */
    void shutdown() override;

    /** Create a new file implementation.

        @return Reference to newly created file implementation wrapper.
    */
    uring_file_impl& create_impl();

    /** Destroy a file implementation wrapper.

        @param impl Reference to the wrapper to destroy.
    */
    void destroy_impl(uring_file_impl& impl);

    /** Unregister a file implementation from tracking.

        Called by the internal implementation destructor.

        @param impl Reference to the internal implementation.
    */
    void unregister_impl(uring_file_impl_internal& impl);

    /** Open a file.

        @param impl Reference to the internal implementation.
        @param path Path to the file.
        @param flags POSIX open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
        @param mode POSIX open mode flags (O_CREAT, O_TRUNC, etc.)
        @return Error code, empty if successful.
    */
    std::error_code open_file(
        uring_file_impl_internal& impl,
        std::filesystem::path const& path,
        int flags,
        int mode);

    /** Get the native io_uring handle.

        @return Pointer to io_uring structure.
    */
    io_uring* native_handle() noexcept { return &ring_; }

    /** Poll io_uring completion queue.

        Called by the epoll scheduler when the io_uring fd becomes readable.
        Processes all available completion queue entries and posts them
        to the scheduler.
    */
    void poll_completions();

    /** Notify service that work has started. */
    void work_started() noexcept;

    /** Notify service that work has finished. */
    void work_finished() noexcept;

private:
    /** Initialize io_uring instance.

        @return Error code, empty if successful.
    */
    std::error_code init_uring();

    /** Shutdown io_uring instance. */
    void shutdown_uring();

    /** Reference to the scheduler. */
    boost::corosio::detail::scheduler& sched_;

    /** io_uring instance. */
    io_uring ring_;

    /** io_uring file descriptor (for epoll integration). */
    int ring_fd_ = -1;

    /** Flag indicating if io_uring has been initialized. */
    bool uring_initialized_ = false;

    /** Mutex for thread-safe access to file lists. */
    std::mutex mutex_;

    /** List of active file implementations. */
    boost::corosio::detail::intrusive_list<uring_file_impl_internal> file_list_;

    /** List of wrapper implementations. */
    boost::corosio::detail::intrusive_list<uring_file_impl> wrapper_list_;
};

} // namespace nntp::detail

#endif // __linux__

#endif // NNTP_DETAIL_URING_FILE_SERVICE_H
