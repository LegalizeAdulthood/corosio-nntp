#pragma once

#include <string>
#include <string_view>

namespace nntp
{

class Newsgroup
{
public:
    explicit Newsgroup(const std::string_view &value);

    const std::string &value() const noexcept
    {
        return m_value;
    }

private:
    std::string m_value;
};

inline bool operator==(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return lhs.value() == rhs.value();
}

inline bool operator!=(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator<(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return lhs.value() < rhs.value();
}

inline bool operator<=(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return lhs.value() <= rhs.value();
}

inline bool operator>(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return lhs.value() > rhs.value();
}

inline bool operator>=(const Newsgroup &lhs, const Newsgroup &rhs) noexcept
{
    return lhs.value() >= rhs.value();
}

} // namespace nntp
