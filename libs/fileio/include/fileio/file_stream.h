#ifndef NNTP_FILE_STREAM_H
#define NNTP_FILE_STREAM_H

#include <boost/corosio/io_stream.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include <boost/corosio/detail/platform.hpp>
#include <filesystem>
#include <system_error>
#include <cstdint>

namespace nntp {

namespace detail {
#if BOOST_COROSIO_HAS_IOCP
    class win_file_service;
    class win_file_impl;
#elif defined(__linux__)
    class uring_file_service;
    class uring_file_impl;
#elif defined(__APPLE__)
    class gcd_file_service;
    class gcd_file_impl;
#endif
}

/** Asynchronous file stream for Windows.

    This class provides async read and write operations for files
    on Windows using overlapped I/O. It satisfies the io_stream
    contract and can be used with generic stream algorithms.

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe. All operations must be serialized.

    @par Example
    @code
    capy::task<> read_file_example(capy::execution_context& ctx)
    {
        file_stream fs(ctx);

        // Open file for reading
        std::error_code ec = fs.open("data.txt", file_stream::read_only);
        if (ec)
            co_return;

        std::vector<char> buffer(4096);
        auto [ec2, n] = co_await fs.read_some(capy::buffer(buffer));

        if (ec2.failed() && ec2 != capy::cond::eof)
            capy::detail::throw_system_error(ec2);

        // Process buffer...
        fs.close();
    }
    @endcode
*/
class file_stream : public boost::corosio::io_stream
{
public:
    /** File access mode flags. */
    enum access_mode
    {
        read_only = 1,      ///< Open for reading
        write_only = 2,     ///< Open for writing
        read_write = 3      ///< Open for reading and writing
    };

    /** File creation disposition flags. */
    enum creation_mode
    {
        open_existing,      ///< Open existing file, fail if doesn't exist
        create_new,         ///< Create new file, fail if exists
        create_always,      ///< Create new file, overwrite if exists
        open_always         ///< Open existing or create new
    };

    /** Construct file stream.

        @param ctx The execution context to bind this stream to.
    */
    explicit file_stream(boost::capy::execution_context& ctx);

    /** Destructor - closes the file if open. */
    ~file_stream();

    /** Open a file for async I/O.

        Opens the specified file for asynchronous operations. The file
        is opened with FILE_FLAG_OVERLAPPED on Windows to enable async I/O.

        @param path Path to the file to open
        @param access Access mode (read_only, write_only, read_write)
        @param creation How to create/open the file

        @return Error code indicating success or failure
    */
    std::error_code open(
        std::filesystem::path const& path,
        access_mode access,
        creation_mode creation = open_existing);

    /** Check if the file is open.

        @return true if the file is currently open
    */
    bool is_open() const noexcept;

    /** Close the file.

        Closes the underlying file handle. Any pending operations
        will be cancelled.
    */
    void close();

    /** Get the current file position.

        @return Current file offset in bytes
    */
    std::uint64_t tell() const noexcept;

    /** Seek to a position in the file.

        Sets the file position for the next read or write operation.

        @param offset Byte offset from the beginning of the file
    */
    void seek(std::uint64_t offset) noexcept;

    /** Get the file size.

        @param ec Set to indicate error
        @return File size in bytes
    */
    std::uint64_t size(std::error_code& ec) const noexcept;

    /** Cancel pending I/O operations. */
    void cancel() noexcept;

private:
#if BOOST_COROSIO_HAS_IOCP
    detail::win_file_service* svc_ = nullptr;
    detail::win_file_impl* impl_ = nullptr;
#elif defined(__linux__)
    detail::uring_file_service* svc_ = nullptr;
    detail::uring_file_impl* impl_ = nullptr;
#elif defined(__APPLE__)
    detail::gcd_file_service* svc_ = nullptr;
    detail::gcd_file_impl* impl_ = nullptr;
#endif
};

} // namespace nntp

#endif // NNTP_FILE_STREAM_H
