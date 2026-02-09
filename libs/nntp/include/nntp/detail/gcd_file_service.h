#ifndef NNTP_DETAIL_GCD_FILE_SERVICE_H
#define NNTP_DETAIL_GCD_FILE_SERVICE_H

#ifdef __APPLE__

#include <boost/corosio/detail/config.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include "src/detail/intrusive.hpp"

#include <dispatch/dispatch.h>
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

class gcd_file_impl_internal;
class gcd_file_impl;

/** Service for managing GCD-based file I/O operations.

    This service initializes and manages Grand Central Dispatch (GCD)
    resources for asynchronous file I/O. It creates dispatch queues
    and integrates with the kqueue scheduler by posting completions.

    @note Internal implementation detail. Users interact with file_stream class.
*/
class gcd_file_service : public boost::capy::execution_context::service
{
    friend class gcd_file_impl_internal;

public:
    using key_type = gcd_file_service;

    /** Construct the service.

        @param ctx Reference to the owning execution_context.
    */
    explicit gcd_file_service(boost::capy::execution_context& ctx);

    /** Destroy the service. */
    ~gcd_file_service();

    gcd_file_service(gcd_file_service const&) = delete;
    gcd_file_service& operator=(gcd_file_service const&) = delete;

    /** Shutdown the service.

        Closes all files and releases GCD resources.
    */
    void shutdown() override;

    /** Create a new file implementation.

        @return Reference to newly created file implementation wrapper.
    */
    gcd_file_impl& create_impl();

    /** Destroy a file implementation wrapper.

        @param impl Reference to the wrapper to destroy.
    */
    void destroy_impl(gcd_file_impl& impl);

    /** Unregister a file implementation from tracking.

        Called by the internal implementation destructor.

        @param impl Reference to the internal implementation.
    */
    void unregister_impl(gcd_file_impl_internal& impl);

    /** Open a file.

        @param impl Reference to the internal implementation.
        @param path Path to the file.
        @param flags POSIX open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
        @param mode POSIX open mode flags (O_CREAT, O_TRUNC, etc.)
        @return Error code, empty if successful.
    */
    std::error_code open_file(
        gcd_file_impl_internal& impl,
        std::filesystem::path const& path,
        int flags,
        int mode);

    /** Get the GCD dispatch queue for file I/O.

        @return The dispatch queue.
    */
    dispatch_queue_t io_queue() noexcept { return io_queue_; }

    /** Notify service that work has started. */
    void work_started() noexcept;

    /** Notify service that work has finished. */
    void work_finished() noexcept;

private:
    /** Initialize GCD resources.

        @return Error code, empty if successful.
    */
    std::error_code init_gcd();

    /** Shutdown GCD resources. */
    void shutdown_gcd();

    /** Reference to the scheduler. */
    boost::corosio::detail::scheduler& sched_;

    /** GCD dispatch queue for file I/O operations. */
    dispatch_queue_t io_queue_ = nullptr;

    /** Mutex for thread-safe access to file lists. */
    std::mutex mutex_;

    /** List of active file implementations. */
    boost::corosio::detail::intrusive_list<gcd_file_impl_internal> file_list_;

    /** List of wrapper implementations. */
    boost::corosio::detail::intrusive_list<gcd_file_impl> wrapper_list_;
};

} // namespace nntp::detail

#endif // __APPLE__

#endif // NNTP_DETAIL_GCD_FILE_SERVICE_H
