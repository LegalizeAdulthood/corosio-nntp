#pragma once

#include <string>
#include <string_view>

namespace nntp
{

class MessageId
{
public:
    explicit MessageId(const std::string_view &value);

    // clang-format off
    const std::string &value() const noexcept { return m_value; }

    friend bool operator==(const MessageId& lhs, const MessageId& rhs) noexcept { return lhs.m_value == rhs.m_value; }
    friend bool operator!=(const MessageId& lhs, const MessageId& rhs) noexcept { return lhs.m_value != rhs.m_value; }
    friend bool operator<(const MessageId& lhs, const MessageId& rhs) noexcept  { return lhs.m_value < rhs.m_value; }
    friend bool operator<=(const MessageId& lhs, const MessageId& rhs) noexcept { return lhs.m_value <= rhs.m_value; }
    friend bool operator>(const MessageId& lhs, const MessageId& rhs) noexcept  { return lhs.m_value > rhs.m_value; }
    friend bool operator>=(const MessageId& lhs, const MessageId& rhs) noexcept { return lhs.m_value >= rhs.m_value; }
    // clang-format on

private:
    std::string m_value;
};

} // namespace nntp
