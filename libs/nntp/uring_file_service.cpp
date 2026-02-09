#ifdef __linux__

#include <nntp/detail/uring_file_service.h>
#include <nntp/detail/uring_file_impl.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_EPOLL

#include "src/detail/epoll/scheduler.hpp"
#include "src/detail/make_err.hpp"

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

namespace nntp::detail
{

uring_file_service::uring_file_service(boost::capy::execution_context& ctx)
    : sched_(ctx.use_service<boost::corosio::detail::epoll_scheduler>())
{
    // Initialize io_uring instance
    auto ec = init_uring();
    if (ec)
    {
        // If io_uring initialization fails, we can't proceed
        throw std::system_error(ec, "Failed to initialize io_uring");
    }
}

uring_file_service::~uring_file_service()
{
    shutdown_uring();
}

void
uring_file_service::shutdown()
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

    // Shutdown io_uring
    shutdown_uring();
}

std::error_code
uring_file_service::init_uring()
{
    // Initialize io_uring with 64 entry queue
    int ret = io_uring_queue_init(64, &ring_, 0);
    if (ret < 0)
    {
        return boost::corosio::detail::make_err(-ret);
    }

    uring_initialized_ = true;

    // Get ring fd for epoll integration
    ring_fd_ = ring_.ring_fd;

    // Register with epoll to get notification when CQ has entries
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // Edge-triggered
    ev.data.ptr = this;

    int epoll_fd = sched_.epoll_fd();
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ring_fd_, &ev) == -1)
    {
        int err = errno;
        io_uring_queue_exit(&ring_);
        uring_initialized_ = false;
        return boost::corosio::detail::make_err(err);
    }

    return {};
}

void
uring_file_service::shutdown_uring()
{
    if (uring_initialized_)
    {
        // Unregister from epoll
        if (ring_fd_ != -1)
        {
            int epoll_fd = sched_.epoll_fd();
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ring_fd_, nullptr);
            ring_fd_ = -1;
        }

        // Exit io_uring
        io_uring_queue_exit(&ring_);
        uring_initialized_ = false;
    }
}

void
uring_file_service::poll_completions()
{
    if (!uring_initialized_)
        return;

    io_uring_cqe* cqe;

    // Process all available completions
    while (io_uring_peek_cqe(&ring_, &cqe) == 0)
    {
        // Extract operation from user_data
        auto* op = static_cast<boost::corosio::detail::scheduler_op*>(
            io_uring_cqe_get_data(cqe));

        if (op)
        {
            // Store result in operation (will be processed by do_complete)
            // The result is stored in cqe->res (bytes transferred or -errno)
            // We'll pass it to the scheduler via post

            // Mark CQE as seen
            io_uring_cqe_seen(&ring_, cqe);

            // Post to scheduler for completion
            // The scheduler will call the operation's completion handler
            sched_.post(op);
        }
        else
        {
            // No operation associated, just mark as seen
            io_uring_cqe_seen(&ring_, cqe);
        }
    }
}

uring_file_impl&
uring_file_service::create_impl()
{
    auto internal = std::make_shared<uring_file_impl_internal>(*this);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        file_list_.push_back(internal.get());
    }

    auto* wrapper = new uring_file_impl(std::move(internal));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        wrapper_list_.push_back(wrapper);
    }

    return *wrapper;
}

void
uring_file_service::destroy_impl(uring_file_impl& impl)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wrapper_list_.remove(&impl);
    }
    delete &impl;
}

void
uring_file_service::unregister_impl(uring_file_impl_internal& impl)
{
    std::lock_guard<std::mutex> lock(mutex_);
    file_list_.remove(&impl);
}

std::error_code
uring_file_service::open_file(
    uring_file_impl_internal& impl,
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
    return {};
}

boost::capy::execution_context&
uring_file_service::context() noexcept
{
    return sched_.context();
}

void
uring_file_service::work_started() noexcept
{
    sched_.on_work_started();
}

void
uring_file_service::work_finished() noexcept
{
    sched_.on_work_finished();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_EPOLL

#endif // __linux__
