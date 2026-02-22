#pragma once

#include <nntp/Newsgroup.h>

#include <string>
#include <string_view>

namespace nntp
{

class WildcardMatchPattern
{
public:
    WildcardMatchPattern() :
        WildcardMatchPattern("*")
    {
    }

    explicit WildcardMatchPattern(std::string_view pattern);

    WildcardMatchPattern(const WildcardMatchPattern &) = default;
    WildcardMatchPattern(WildcardMatchPattern &&) noexcept = default;
    WildcardMatchPattern &operator=(const WildcardMatchPattern &) = default;
    WildcardMatchPattern &operator=(WildcardMatchPattern &&) noexcept = default;
    ~WildcardMatchPattern() = default;

    // clang-format off
    const std::string &value() const noexcept { return m_pattern; }
    // clang-format on

    bool match(const Newsgroup &group) const;

private:
    std::string m_pattern;
};

inline bool operator==(const WildcardMatchPattern &lhs, const WildcardMatchPattern &rhs) noexcept
{
    return lhs.value() == rhs.value();
}

inline bool operator!=(const WildcardMatchPattern &lhs, const WildcardMatchPattern &rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace nntp
