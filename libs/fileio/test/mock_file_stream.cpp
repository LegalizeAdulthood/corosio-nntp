#include <fileio/test/mock_file_stream.h>

namespace nntp::test {

mock_file_stream::
~mock_file_stream()
{
    // Safety net - verify if still open
    if (is_open_ && (!expect_.empty() || !provide_.empty()))
    {
        fuse_->fail();
    }
}

mock_file_stream::
mock_file_stream(
    boost::capy::test::fuse& f,
    std::size_t max_read_size,
    std::size_t max_write_size)
    : fuse_(&f)
    , max_read_size_(max_read_size)
    , max_write_size_(max_write_size)
{
}

void
mock_file_stream::
provide(std::string data)
{
    provide_.append(std::move(data));
}

void
mock_file_stream::
expect(std::string data)
{
    expect_.append(std::move(data));
}

void
mock_file_stream::
set_file_size(std::size_t size)
{
    file_size_ = size;
}

std::error_code
mock_file_stream::
open(
    std::filesystem::path const&,
    file_stream::access_mode,
    file_stream::creation_mode)
{
    is_open_ = true;
    return {};
}

bool
mock_file_stream::
is_open() const noexcept
{
    return is_open_;
}

std::error_code
mock_file_stream::
close()
{
    if (!is_open_)
        return {};

    is_open_ = false;

    // Strict verification - like mocket
    if (!expect_.empty())
    {
        fuse_->fail();
        return boost::capy::error::test_failure;
    }
    if (!provide_.empty())
    {
        fuse_->fail();
        return boost::capy::error::test_failure;
    }

    return {};
}

std::uint64_t
mock_file_stream::
tell() const noexcept
{
    return position_;
}

void
mock_file_stream::
seek(std::uint64_t offset) noexcept
{
    position_ = offset;
}

std::uint64_t
mock_file_stream::
size(std::error_code& ec) const noexcept
{
    ec = {};
    return file_size_;
}

} // namespace nntp::test
