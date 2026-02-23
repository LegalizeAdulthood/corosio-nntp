#include <nntp/Client.h>

#include <boost/capy/test/fuse.hpp>
#include <boost/capy/test/stream.hpp>

#include <gtest/gtest.h>

using namespace nntp;
using namespace boost::capy;

TEST(TestClient, onConnectSuccess)
{
    test::fuse f;
    test::stream stream(f);
    stream.provide("200 example.org InterNetNews NNRP server INN 2.7.3 ready (posting ok)\r\n");
    any_stream connection{stream};

    test::fuse::result result = f.armed(
        [&](test::fuse &) -> task<void>
        {
            co_await Client::connect(connection);
            co_return;
        });

    EXPECT_TRUE(result);
}
