#include <nntp/Client.h>

#include <boost/capy/read_until.hpp>
#include <boost/capy/buffers/string_dynamic_buffer.hpp>

#include <stdexcept>

using namespace boost::capy;

namespace nntp
{

Client::Client(any_stream &stream) :
    m_stream(stream)
{
}

Task<Client> Client::connect(any_stream &stream)
{
    std::string line;
    string_dynamic_buffer buffer(&line);
    io_result<std::size_t> result = co_await read_until(stream, buffer, "\r\n", 5U);
    co_return Client(stream);
}

} // namespace nntp
