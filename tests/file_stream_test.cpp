#include <nntp/file_stream.h>
#include <boost/corosio/io_context.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/cond.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <array>
#include <string>

using namespace nntp;
using namespace boost;

TEST(FileStream, WriteAndReadFile)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_file.txt";

    auto task = [&]() -> capy::task<>
    {
        // Write file
        {
            file_stream file(ctx);
            auto ec = file.open(temp, file_stream::write_only, file_stream::create_always);
            EXPECT_FALSE(ec);

            std::string_view data = "Test content\n";
            auto [write_ec, n] = co_await file.write_some(capy::const_buffer(data.data(), data.size()));
            EXPECT_FALSE(write_ec);
            EXPECT_EQ(n, data.size());

            file.close();
        }

        // Read file back
        {
            file_stream file(ctx);
            auto ec = file.open(temp, file_stream::read_only);
            EXPECT_FALSE(ec);

            std::array<char, 100> buffer;
            auto [read_ec, n] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_FALSE(read_ec);
            EXPECT_EQ(n, 13);
            EXPECT_EQ(std::string_view(buffer.data(), n), "Test content\n");

            file.close();
        }
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, SequentialReadsWithPositionTracking)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_seq.txt";

    auto task = [&]() -> capy::task<>
    {
        // Create file with known content
        {
            file_stream file(ctx);
            file.open(temp, file_stream::write_only, file_stream::create_always);

            std::string_view data = "0123456789ABCDEF";
            co_await file.write_some(capy::const_buffer(data.data(), data.size()));
            file.close();
        }

        // Read in chunks
        {
            file_stream file(ctx);
            file.open(temp, file_stream::read_only);

            std::array<char, 4> buffer;
            auto [ec1, n1] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_FALSE(ec1);
            EXPECT_EQ(n1, 4);
            EXPECT_EQ(std::string_view(buffer.data(), 4), "0123");

            auto [ec2, n2] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_FALSE(ec2);
            EXPECT_EQ(n2, 4);
            EXPECT_EQ(std::string_view(buffer.data(), 4), "4567");

            file.close();
        }
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, RandomAccessWithSeek)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_seek.txt";

    auto task = [&]() -> capy::task<>
    {
        // Create file
        {
            file_stream file(ctx);
            file.open(temp, file_stream::write_only, file_stream::create_always);
            std::string_view data = "0123456789";
            co_await file.write_some(capy::const_buffer(data.data(), data.size()));
            file.close();
        }

        // Random access
        {
            file_stream file(ctx);
            file.open(temp, file_stream::read_only);

            // Seek to offset 5
            file.seek(5);
            EXPECT_EQ(file.tell(), 5);

            std::array<char, 3> buffer;
            auto [ec, n] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_FALSE(ec);
            EXPECT_EQ(n, 3);
            EXPECT_EQ(std::string_view(buffer.data(), 3), "567");

            // Seek back to beginning
            file.seek(0);
            EXPECT_EQ(file.tell(), 0);

            auto [ec2, n2] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_EQ(std::string_view(buffer.data(), 3), "012");

            file.close();
        }
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, FileSizeQuery)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_size.txt";

    auto task = [&]() -> capy::task<>
    {
        // Create file with known size
        {
            file_stream file(ctx);
            file.open(temp, file_stream::write_only, file_stream::create_always);

            std::string data(1024, 'X');
            co_await file.write_some(capy::const_buffer(data.data(), data.size()));
            file.close();
        }

        // Query size
        {
            file_stream file(ctx);
            file.open(temp, file_stream::read_only);

            std::error_code ec;
            auto size = file.size(ec);
            EXPECT_FALSE(ec);
            EXPECT_EQ(size, 1024);

            file.close();
        }
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, ReadWriteMode)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_rw.txt";

    auto task = [&]() -> capy::task<>
    {
        file_stream file(ctx);
        file.open(temp, file_stream::read_write, file_stream::create_always);

        // Write data
        std::string_view data = "Initial data";
        auto [write_ec, written] = co_await file.write_some(capy::const_buffer(data.data(), data.size()));
        EXPECT_FALSE(write_ec);
        EXPECT_EQ(written, data.size());

        // Seek back and read
        file.seek(0);

        std::array<char, 20> buffer;
        auto [read_ec, n] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
        EXPECT_FALSE(read_ec);
        EXPECT_EQ(n, data.size());
        EXPECT_EQ(std::string_view(buffer.data(), n), data);

        file.close();
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, EOFDetection)
{
    corosio::io_context ctx;
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "test_eof.txt";

    auto task = [&]() -> capy::task<>
    {
        // Create small file
        {
            file_stream file(ctx);
            file.open(temp, file_stream::write_only, file_stream::create_always);
            std::string_view data = "ABC";
            co_await file.write_some(capy::const_buffer(data.data(), data.size()));
            file.close();
        }

        // Read past EOF
        {
            file_stream file(ctx);
            file.open(temp, file_stream::read_only);

            std::array<char, 10> buffer;

            // First read gets all data
            auto [ec1, n1] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_FALSE(ec1);
            EXPECT_EQ(n1, 3);

            // Second read should hit EOF
            auto [ec2, n2] = co_await file.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
            EXPECT_EQ(ec2, capy::cond::eof);
            EXPECT_EQ(n2, 0);

            file.close();
        }
    };

    capy::run_async(ctx.get_executor())(task());
    ctx.run();

    std::filesystem::remove(temp);
}

TEST(FileStream, OpenNonExistentFile)
{
    corosio::io_context ctx;

    file_stream file(ctx);
    auto ec = file.open("/nonexistent/path/file.txt", file_stream::read_only);

    EXPECT_TRUE(ec);
}

TEST(FileStream, OperationsOnClosedFile)
{
    corosio::io_context ctx;

    file_stream file(ctx);

    std::error_code ec;
    auto size = file.size(ec);

    EXPECT_EQ(ec, std::errc::bad_file_descriptor);
}

