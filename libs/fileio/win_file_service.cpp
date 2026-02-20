#ifdef _WIN32

#include <fileio/detail/win_file_service.h>
#include <fileio/detail/win_file_impl.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_IOCP

#include "src/detail/iocp/scheduler.hpp"
#include "src/detail/iocp/completion_key.hpp"
#include "src/detail/make_err.hpp"

namespace nntp::detail
{

win_file_service::win_file_service(boost::capy::execution_context& ctx)
    : sched_(ctx.use_service<boost::corosio::detail::win_scheduler>())
    , iocp_(sched_.native_handle())
{
}

win_file_service::~win_file_service()
{
}

void
win_file_service::shutdown()
{
    std::scoped_lock lock(mutex_);

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
}

win_file_impl&
win_file_service::create_impl()
{
    auto internal = std::make_shared<win_file_impl_internal>(*this);

    {
        std::scoped_lock lock(mutex_);
        file_list_.push_back(internal.get());
    }

    auto* wrapper = new win_file_impl(std::move(internal));

    {
        std::scoped_lock lock(mutex_);
        wrapper_list_.push_back(wrapper);
    }

    return *wrapper;
}

void
win_file_service::destroy_impl(win_file_impl& impl)
{
    {
        std::scoped_lock lock(mutex_);
        wrapper_list_.remove(&impl);
    }
    delete &impl;
}

void
win_file_service::unregister_impl(win_file_impl_internal& impl)
{
    std::scoped_lock lock(mutex_);
    file_list_.remove(&impl);
}

std::error_code
win_file_service::open_file(
    win_file_impl_internal& impl,
    std::filesystem::path const& path,
    DWORD access,
    DWORD creation)
{
    impl.close_file();

    HANDLE h = ::CreateFileW(
        path.c_str(),
        access,
        FILE_SHARE_READ,
        nullptr,
        creation,
        FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return boost::corosio::detail::make_err(::GetLastError());

    // Associate with IOCP
    HANDLE result = ::CreateIoCompletionPort(
        h,
        static_cast<HANDLE>(iocp_),
        boost::corosio::detail::key_io,
        0);

    if (result == nullptr)
    {
        DWORD dwError = ::GetLastError();
        ::CloseHandle(h);
        return boost::corosio::detail::make_err(dwError);
    }

    impl.set_handle(h);
    impl.set_position(0);
    return {};
}

void
win_file_service::post(boost::corosio::detail::overlapped_op* op)
{
    sched_.post(op);
}

void
win_file_service::work_started() noexcept
{
    sched_.work_started();
}

void
win_file_service::work_finished() noexcept
{
    sched_.work_finished();
}

} // namespace nntp::detail

#endif // BOOST_COROSIO_HAS_IOCP

#endif // _WIN32
