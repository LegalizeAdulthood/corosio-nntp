#ifdef __APPLE__

#include <fileio/detail/gcd_file_service.h>
#include <fileio/detail/gcd_file_impl.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_KQUEUE

#include "src/detail/kqueue/scheduler.hpp"
#include "src/detail/make_err.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace nntp::detail
{

gcd_file_service::gcd_file_service(boost::capy::execution_context& ctx)
    : sched_(ctx.use_service<boost::corosio::detail::kqueue_scheduler>())
{
    // Initialize GCD resources
    auto ec = init_gcd();
    if (ec)
    {
        // If GCD initialization fails, we can't proceed
        throw std::system_error(ec, "Failed to initialize GCD");
    }
}

gcd_file_service::~gcd_file_service()
{
    shutdown_gcd();
}

void
gcd_file_service::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Close all files and remove from list
    // The shared_ptrs held by file objects and operations will handle destruction
    for (auto* impl = file_list_.pop_front(); impl != nullptr;
         impl = file_list_.pop_front())
    {
        impl->close_file();
        // Note: impl may still be alive if operations hold shared_ptr
    }

    // Cleanup wrappers
    for (auto* w = wrapper_list_.pop_front(); w != nullptr;
         w = wrapper_list_.pop_front())
    {
        delete w;
    }

    // Shutdown GCD
    shutdown_gcd();
}

std::error_code
gcd_file_service::init_gcd()
{
    // Create a concurrent dispatch queue for file I/O
    // This queue will handle all file operations
    io_queue_ = dispatch_queue_create(
        "com.nntp.file_io",
        DISPATCH_QUEUE_CONCURRENT);

    if (!io_queue_)
    {
        return boost::corosio::detail::make_err(ENOMEM);
    }

    return {};
}

void
gcd_file_service::shutdown_gcd()
{
    if (io_queue_)
    {
        // Release the dispatch queue
        // GCD will clean up when all operations complete
        dispatch_release(io_queue_);
        io_queue_ = nullptr;
    }
}

gcd_file_impl&
gcd_file_service::create_impl()
{
    auto internal = std::make_shared<gcd_file_impl_internal>(*this);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        file_list_.push_back(internal.get());
    }

    auto* wrapper = new gcd_file_impl(std::move(internal));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        wrapper_list_.push_back(wrapper);
    }

    return *wrapper;
}

void
gcd_file_service::destroy_impl(gcd_file_impl& impl)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wrapper_list_.remove(&impl);
    }
    delete &impl;
}

void
gcd_file_service::unregister_impl(gcd_file_impl_internal& impl)
{
    std::lock_guard<std::mutex> lock(mutex_);
    file_list_.remove(&impl);
}

std::error_code
gcd_file_service::open_file(
    gcd_file_impl_internal& impl,
    std::filesystem::path const& path,
    int flags,
    int mode)
{
    impl.close_file();

    int fd = ::open(path.c_str(), flags, mode);
    if (fd == -1)
        return boost::corosio::detail::make_err(errno);

    impl.set_fd(fd);
    impl.set_position(0);

    // Create dispatch_io channel for this file
    impl.open_channel();

    return {};
}

boost::capy::execution_context&
gcd_file_service::context() noexcept
{
    return sched_.context();
}

void
gcd_file_service::work_started() noexcept
{
    sched_.on_work_started();
}

void
gcd_file_service::work_finished() noexcept
{
    sched_.on_work_finished();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_KQUEUE

#endif // __APPLE__
