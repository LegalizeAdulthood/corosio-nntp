#ifndef NNTP_TEST_MOCK_FILE_STREAM_H
#define NNTP_TEST_MOCK_FILE_STREAM_H

#include <fileio/file_stream.h>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/cond.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/test/fuse.hpp>

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

namespace nntp::test {

/** A mock file stream for testing I/O operations.

    This class provides a testable file-like interface where data
    can be staged for reading and expected data can be validated on
    writes. Unlike mocket which wraps a real socket, this is a pure
    mock with no actual file I/O.

    When reading, data comes from the `provide()` buffer first.
    When writing, data is validated against the `expect()` buffer.
    All operations complete synchronously.

    The mock is strict: `close()` always verifies that all expectations
    are met. Unmet expectations cause test failure via fuse.

    @par Thread Safety
    Not thread-safe. All operations must occur on a single thread.

    @see file_stream
*/
class mock_file_stream
{
    std::string provide_;
    std::string expect_;
    boost::capy::test::fuse* fuse_;
    std::size_t position_ = 0;
    std::size_t file_size_ = 0;
    std::size_t max_read_size_;
    std::size_t max_write_size_;
    bool is_open_ = false;

    template<class MutableBufferSequence>
    std::size_t
    consume_provide(MutableBufferSequence const& buffers) noexcept;

    template<class ConstBufferSequence>
    bool
    validate_expect(
        ConstBufferSequence const& buffers,
        std::size_t& bytes_written);

public:
    template<class MutableBufferSequence>
    class read_some_awaitable;

    template<class ConstBufferSequence>
    class write_some_awaitable;

    /** Destructor - verifies expectations as safety net.
    */
    ~mock_file_stream();

    /** Construct a mock file stream.

        @param f The fuse for error injection testing.
        @param max_read_size Maximum bytes per read operation.
        @param max_write_size Maximum bytes per write operation.
    */
    explicit mock_file_stream(
        boost::capy::test::fuse& f,
        std::size_t max_read_size = std::size_t(-1),
        std::size_t max_write_size = std::size_t(-1));

    mock_file_stream(mock_file_stream const&) = delete;
    mock_file_stream& operator=(mock_file_stream const&) = delete;

    /** Stage data for reads.

        Appends the given string to this mock's provide buffer.
        When `read_some` is called, it will receive this data.

        @param data The data to provide.
    */
    void provide(std::string data);

    /** Set expected data for writes.

        Appends the given string to this mock's expect buffer.
        When the caller writes to this mock, the written data
        must match the expected data. On mismatch, `fuse::fail()`
        is called.

        @param data The expected data.
    */
    void expect(std::string data);

    /** Set the simulated file size.

        @param size The file size to return from `size()`.
    */
    void set_file_size(std::size_t size);

    /** Open the mock file stream.

        This just sets the open state flag. No actual file I/O occurs.

        @param path File path (ignored in mock).
        @param access Access mode (ignored in mock).
        @param creation Creation mode (ignored in mock).

        @return Always succeeds with empty error_code.
    */
    std::error_code open(
        std::filesystem::path const& path,
        file_stream::access_mode access,
        file_stream::creation_mode creation = file_stream::open_existing);

    /** Check if the mock file stream is open.

        @return `true` if open() was called.
    */
    bool is_open() const noexcept;

    /** Close the mock file stream and verify test expectations.

        Verifies that both the `expect()` and `provide()` buffers
        are empty. If either buffer contains unconsumed data,
        returns `test_failure` and calls `fuse::fail()`.

        This is a strict mock - verification always happens.

        @return An error code indicating success or failure.
            Returns `error::test_failure` if buffers are not empty.
    */
    std::error_code close();

    /** Get the current file position.

        @return Current simulated file offset in bytes.
    */
    std::uint64_t tell() const noexcept;

    /** Seek to a position in the file.

        Sets the simulated file position for the next read or write.

        @param offset Byte offset from the beginning of the file.
    */
    void seek(std::uint64_t offset) noexcept;

    /** Get the simulated file size.

        @param ec Set to indicate error (always success for mock).
        @return Simulated file size in bytes.
    */
    std::uint64_t size(std::error_code& ec) const noexcept;

    /** Initiate a read operation.

        Reads available data from the provide buffer. If the provide
        buffer has data, it is consumed. Otherwise, returns EOF.
        All operations complete synchronously.

        @param buffers The buffer sequence to read data into.

        @return An awaitable yielding `(error_code, std::size_t)`.
    */
    template<class MutableBufferSequence>
    auto read_some(MutableBufferSequence const& buffers)
    {
        return read_some_awaitable<MutableBufferSequence>(*this, buffers);
    }

    /** Initiate a write operation.

        Validates written data against the expect buffer. If the expect
        buffer has data, validates and consumes it. Otherwise, succeeds.
        All operations complete synchronously.

        @param buffers The buffer sequence containing data to write.

        @return An awaitable yielding `(error_code, std::size_t)`.
    */
    template<class ConstBufferSequence>
    auto write_some(ConstBufferSequence const& buffers)
    {
        return write_some_awaitable<ConstBufferSequence>(*this, buffers);
    }
};

//------------------------------------------------------------------------------

template<class MutableBufferSequence>
std::size_t
mock_file_stream::
consume_provide(MutableBufferSequence const& buffers) noexcept
{
    auto n = boost::capy::buffer_copy(
        buffers,
        boost::capy::make_buffer(provide_),
        max_read_size_);
    provide_.erase(0, n);
    return n;
}

template<class ConstBufferSequence>
bool
mock_file_stream::
validate_expect(
    ConstBufferSequence const& buffers,
    std::size_t& bytes_written)
{
    if (expect_.empty())
        return true;

    // Build the write data up to max_write_size_
    std::string written;
    auto total = boost::capy::buffer_size(buffers);
    if (total > max_write_size_)
        total = max_write_size_;
    written.resize(total);
    boost::capy::buffer_copy(
        boost::capy::make_buffer(written),
        buffers,
        max_write_size_);

    // Check if written data matches expect prefix
    auto const match_size = (std::min)(written.size(), expect_.size());
    if (std::memcmp(written.data(), expect_.data(), match_size) != 0)
    {
        fuse_->fail();
        bytes_written = 0;
        return false;
    }

    // Consume matched portion
    expect_.erase(0, match_size);
    bytes_written = written.size();
    return true;
}

//------------------------------------------------------------------------------

template<class MutableBufferSequence>
class mock_file_stream::read_some_awaitable
{
    mock_file_stream* m_;
    MutableBufferSequence buffers_;
    std::size_t n_ = 0;
    std::error_code ec_;

public:
    read_some_awaitable(
        mock_file_stream& m,
        MutableBufferSequence buffers) noexcept
        : m_(&m)
        , buffers_(std::move(buffers))
    {
    }

    read_some_awaitable(read_some_awaitable const&) = delete;
    read_some_awaitable& operator=(read_some_awaitable const&) = delete;

    bool await_ready()
    {
        // Always completes synchronously
        if (!m_->provide_.empty())
        {
            n_ = m_->consume_provide(buffers_);
            m_->position_ += n_;
        }
        else
        {
            ec_ = boost::capy::error::eof;
            n_ = 0;
        }
        return true;
    }

    template<class... Args>
    auto await_suspend(Args&&...) noexcept
    {
        // Never called - always completes synchronously
    }

    boost::capy::io_result<std::size_t> await_resume() const noexcept
    {
        return {ec_, n_};
    }
};

//------------------------------------------------------------------------------

template<class ConstBufferSequence>
class mock_file_stream::write_some_awaitable
{
    mock_file_stream* m_;
    ConstBufferSequence buffers_;
    std::size_t n_ = 0;
    std::error_code ec_;

public:
    write_some_awaitable(
        mock_file_stream& m,
        ConstBufferSequence buffers) noexcept
        : m_(&m)
        , buffers_(std::move(buffers))
    {
    }

    write_some_awaitable(write_some_awaitable const&) = delete;
    write_some_awaitable& operator=(write_some_awaitable const&) = delete;

    bool await_ready()
    {
        // Always completes synchronously
        if (!m_->validate_expect(buffers_, n_))
        {
            ec_ = boost::capy::error::test_failure;
            n_ = 0;
        }
        else
        {
            m_->position_ += n_;
        }
        return true;
    }

    template<class... Args>
    auto await_suspend(Args&&...) noexcept
    {
        // Never called - always completes synchronously
    }

    boost::capy::io_result<std::size_t> await_resume() const noexcept
    {
        return {ec_, n_};
    }
};

} // namespace nntp::test

#endif // NNTP_TEST_MOCK_FILE_STREAM_H
