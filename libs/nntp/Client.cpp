#include <nntp/Client.h>

#include <stdexcept>

namespace nntp
{

Client::Client(boost::capy::any_stream &stream) :
    m_stream(stream)
{
    throw std::runtime_error("not implemented");
}

} // namespace nntp
