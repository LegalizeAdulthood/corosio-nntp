#include <nntp/test/mock_file_stream.h>
#include <gtest/gtest.h>

using namespace nntp::test;
using namespace boost;

TEST(MockFileStream, BasicProvideAndRead)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.provide("Hello, World!");

    std::array<char, 20> buffer;
    auto awaitable = mock.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
    ASSERT_TRUE(awaitable.await_ready());
    auto [ec, n] = awaitable.await_resume();

    EXPECT_FALSE(ec);
    EXPECT_EQ(n, 13);
    EXPECT_EQ(std::string_view(buffer.data(), n), "Hello, World!");
}

TEST(MockFileStream, BasicExpectAndWrite)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.expect("Test data");

    std::string_view data = "Test data";
    auto awaitable = mock.write_some(capy::const_buffer(data.data(), data.size()));
    ASSERT_TRUE(awaitable.await_ready());
    auto [ec, n] = awaitable.await_resume();

    EXPECT_FALSE(ec);
    EXPECT_EQ(n, 9);

    auto close_ec = mock.close();
    EXPECT_FALSE(close_ec);
}

TEST(MockFileStream, ChunkedReads)
{
    capy::test::fuse f;
    mock_file_stream mock(f, 5);
    mock.provide("Hello, World!");

    std::string result;
    std::array<char, 20> buffer;

    while (true)
    {
        auto awaitable = mock.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
        ASSERT_TRUE(awaitable.await_ready());
        auto [ec, n] = awaitable.await_resume();
        if (ec == capy::cond::eof)
            break;
        EXPECT_FALSE(ec);
        result.append(buffer.data(), n);
    }

    EXPECT_EQ(result, "Hello, World!");
}

TEST(MockFileStream, PositionTracking)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.provide("0123456789");

    EXPECT_EQ(mock.tell(), 0);

    std::array<char, 5> buffer;
    {
        auto awaitable = mock.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
        ASSERT_TRUE(awaitable.await_ready());
        auto [ec1, n1] = awaitable.await_resume();
        EXPECT_EQ(mock.tell(), 5);
    }

    mock.seek(2);
    EXPECT_EQ(mock.tell(), 2);

    {
        auto awaitable = mock.read_some(capy::mutable_buffer(buffer.data(), buffer.size()));
        ASSERT_TRUE(awaitable.await_ready());
        auto [ec2, n2] = awaitable.await_resume();
        EXPECT_EQ(mock.tell(), 7);
    }
}

TEST(MockFileStream, WriteValidationFailure)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.expect("Expected data");

    std::string_view wrong_data = "Wrong data!!!";
    auto awaitable = mock.write_some(capy::const_buffer(wrong_data.data(), wrong_data.size()));
    ASSERT_TRUE(awaitable.await_ready());
    auto [ec, n] = awaitable.await_resume();

    EXPECT_EQ(ec, capy::error::test_failure);
    EXPECT_EQ(n, 0);
}

TEST(MockFileStream, FileSizeSimulation)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.set_file_size(12345);

    std::error_code ec;
    auto size = mock.size(ec);

    EXPECT_FALSE(ec);
    EXPECT_EQ(size, 12345);
}

TEST(MockFileStream, CloseWithUnmetExpectations)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.open("test.txt", nntp::file_stream::write_only);
    mock.expect("Should have written this");

    auto ec = mock.close();
    EXPECT_EQ(ec, capy::error::test_failure);
}

TEST(MockFileStream, CloseWithUnconsumedProvide)
{
    capy::test::fuse f;
    mock_file_stream mock(f);
    mock.open("test.txt", nntp::file_stream::read_only);
    mock.provide("Data that was never read");

    auto ec = mock.close();
    EXPECT_EQ(ec, capy::error::test_failure);
}

TEST(MockFileStream, DestructorVerification)
{
    capy::test::fuse f;

    {
        mock_file_stream mock(f);
        mock.open("test.txt", nntp::file_stream::read_only);
        mock.expect("Forgotten expectation");
    }
}
