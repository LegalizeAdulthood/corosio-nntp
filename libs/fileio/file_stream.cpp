#include <fileio/file_stream.h>
#include <boost/corosio/detail/platform.hpp>

#if BOOST_COROSIO_HAS_IOCP

#include <fileio/detail/win_file_service.h>
#include <fileio/detail/win_file_impl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nntp {

using file_service = detail::win_file_service;
using file_impl = detail::win_file_impl;

file_stream::file_stream(boost::capy::execution_context& ctx)
    : io_stream(ctx)
    , svc_(&ctx.use_service<file_service>())
{
    auto& wrapper = svc_->create_impl();
    impl_ = &wrapper;
    io_stream::impl_ = impl_;
}

file_stream::~file_stream()
{
    close();
}

std::error_code
file_stream::open(
    std::filesystem::path const& path,
    access_mode access,
    creation_mode creation)
{
    if (!impl_)
        return std::make_error_code(std::errc::bad_file_descriptor);

    // Convert access mode to Windows flags
    DWORD desired_access = 0;
    if (access & read_only)
        desired_access |= GENERIC_READ;
    if (access & write_only)
        desired_access |= GENERIC_WRITE;

    // Convert creation mode to Windows flags
    DWORD creation_disposition = 0;
    switch (creation)
    {
    case open_existing:
        creation_disposition = OPEN_EXISTING;
        break;
    case create_new:
        creation_disposition = CREATE_NEW;
        break;
    case create_always:
        creation_disposition = CREATE_ALWAYS;
        break;
    case open_always:
        creation_disposition = OPEN_ALWAYS;
        break;
    }

    // Open file through service
    return svc_->open_file(
        *impl_->get_internal(),
        path,
        desired_access,
        creation_disposition);
}

bool
file_stream::is_open() const noexcept
{
    return impl_ && impl_->get_internal()->is_open();
}

void
file_stream::close()
{
    if (impl_)
    {
        impl_->get_internal()->close_file();
    }
}

std::uint64_t
file_stream::tell() const noexcept
{
    if (!impl_)
        return 0;
    return impl_->get_internal()->position();
}

void
file_stream::seek(std::uint64_t offset) noexcept
{
    if (impl_)
    {
        impl_->get_internal()->set_position(offset);
    }
}

std::uint64_t
file_stream::size(std::error_code& ec) const noexcept
{
    if (!impl_ || !impl_->get_internal()->is_open())
    {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    LARGE_INTEGER size;
    if (!::GetFileSizeEx(impl_->get_internal()->native_handle(), &size))
    {
        ec = std::error_code(::GetLastError(), std::system_category());
        return 0;
    }

    ec = {};
    return static_cast<std::uint64_t>(size.QuadPart);
}

void
file_stream::cancel() noexcept
{
    if (impl_)
    {
        impl_->cancel();
    }
}

} // namespace nntp

#elif defined(__linux__)

#include <fileio/detail/uring_file_service.h>
#include <fileio/detail/uring_file_impl.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace nntp {

using file_service = detail::uring_file_service;
using file_impl = detail::uring_file_impl;

file_stream::file_stream(boost::capy::execution_context& ctx)
    : io_stream(ctx)
    , svc_(&ctx.use_service<file_service>())
{
    auto& wrapper = svc_->create_impl();
    impl_ = &wrapper;
    io_stream::impl_ = impl_;
}

file_stream::~file_stream()
{
    close();
}

std::error_code
file_stream::open(
    std::filesystem::path const& path,
    access_mode access,
    creation_mode creation)
{
    if (!impl_)
        return std::make_error_code(std::errc::bad_file_descriptor);

    // Convert access mode to POSIX flags
    int flags = 0;
    if (access == read_only)
        flags = O_RDONLY;
    else if (access == write_only)
        flags = O_WRONLY;
    else if (access == (read_only | write_only))
        flags = O_RDWR;

    // Convert creation mode to POSIX flags
    switch (creation)
    {
    case open_existing:
        // No additional flags - file must exist
        break;
    case create_new:
        flags |= O_CREAT | O_EXCL;
        break;
    case create_always:
        flags |= O_CREAT | O_TRUNC;
        break;
    case open_always:
        flags |= O_CREAT;
        break;
    }

    // Open file through service
    return svc_->open_file(
        *impl_->get_internal(),
        path,
        flags,
        0644);  // Default permissions: rw-r--r--
}

bool
file_stream::is_open() const noexcept
{
    return impl_ && impl_->get_internal()->is_open();
}

void
file_stream::close()
{
    if (impl_)
    {
        impl_->get_internal()->close_file();
    }
}

std::uint64_t
file_stream::tell() const noexcept
{
    if (!impl_)
        return 0;
    return impl_->get_internal()->position();
}

void
file_stream::seek(std::uint64_t offset) noexcept
{
    if (impl_)
    {
        impl_->get_internal()->set_position(offset);
    }
}

std::uint64_t
file_stream::size(std::error_code& ec) const noexcept
{
    if (!impl_ || !impl_->get_internal()->is_open())
    {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    struct stat st;
    if (::fstat(impl_->get_internal()->native_handle(), &st) == -1)
    {
        ec = std::error_code(errno, std::system_category());
        return 0;
    }

    ec = {};
    return static_cast<std::uint64_t>(st.st_size);
}

void
file_stream::cancel() noexcept
{
    if (impl_)
    {
        impl_->cancel();
    }
}

} // namespace nntp

#elif defined(__APPLE__)

#include <fileio/detail/gcd_file_service.h>
#include <fileio/detail/gcd_file_impl.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace nntp {

using file_service = detail::gcd_file_service;
using file_impl = detail::gcd_file_impl;

file_stream::file_stream(boost::capy::execution_context& ctx)
    : io_stream(ctx)
    , svc_(&ctx.use_service<file_service>())
{
    auto& wrapper = svc_->create_impl();
    impl_ = &wrapper;
    io_stream::impl_ = impl_;
}

file_stream::~file_stream()
{
    close();
}

std::error_code
file_stream::open(
    std::filesystem::path const& path,
    access_mode access,
    creation_mode creation)
{
    if (!impl_)
        return std::make_error_code(std::errc::bad_file_descriptor);

    // Convert access mode to POSIX flags
    int flags = 0;
    if (access == read_only)
        flags = O_RDONLY;
    else if (access == write_only)
        flags = O_WRONLY;
    else if (access == (read_only | write_only))
        flags = O_RDWR;

    // Convert creation mode to POSIX flags
    switch (creation)
    {
    case open_existing:
        // No additional flags - file must exist
        break;
    case create_new:
        flags |= O_CREAT | O_EXCL;
        break;
    case create_always:
        flags |= O_CREAT | O_TRUNC;
        break;
    case open_always:
        flags |= O_CREAT;
        break;
    }

    // Open file through service
    return svc_->open_file(
        *impl_->get_internal(),
        path,
        flags,
        0644);  // Default permissions: rw-r--r--
}

bool
file_stream::is_open() const noexcept
{
    return impl_ && impl_->get_internal()->is_open();
}

void
file_stream::close()
{
    if (impl_)
    {
        impl_->get_internal()->close_file();
    }
}

std::uint64_t
file_stream::tell() const noexcept
{
    if (!impl_)
        return 0;
    return impl_->get_internal()->position();
}

void
file_stream::seek(std::uint64_t offset) noexcept
{
    if (impl_)
    {
        impl_->get_internal()->set_position(offset);
    }
}

std::uint64_t
file_stream::size(std::error_code& ec) const noexcept
{
    if (!impl_ || !impl_->get_internal()->is_open())
    {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    struct stat st;
    if (::fstat(impl_->get_internal()->native_handle(), &st) == -1)
    {
        ec = std::error_code(errno, std::system_category());
        return 0;
    }

    ec = {};
    return static_cast<std::uint64_t>(st.st_size);
}

void
file_stream::cancel() noexcept
{
    if (impl_)
    {
        impl_->cancel();
    }
}

} // namespace nntp

#endif // __APPLE__
