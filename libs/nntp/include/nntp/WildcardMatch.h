#pragma once

#include <nntp/Newsgroup.h>
#include <nntp/WildcardMatchPattern.h>

#include <string>
#include <string_view>
#include <vector>

namespace nntp
{

class WildcardMatch
{
public:
    WildcardMatch() :
        WildcardMatch("*")
    {
    }

    explicit WildcardMatch(std::string_view wildmat);

    WildcardMatch(const WildcardMatch &) = default;
    WildcardMatch(WildcardMatch &&) noexcept = default;
    WildcardMatch &operator=(const WildcardMatch &) = default;
    WildcardMatch &operator=(WildcardMatch &&) noexcept = default;
    ~WildcardMatch() = default;

    // clang-format off
    const std::string &value() const noexcept { return m_value; }
    // clang-format on

    bool match(const Newsgroup &group) const;

private:
    struct PatternEntry
    {
        WildcardMatchPattern pattern;
        bool negated;
    };

    std::string m_value;
    std::vector<PatternEntry> m_patterns;
};

inline bool operator==(const WildcardMatch &lhs, const WildcardMatch &rhs) noexcept
{
    return lhs.value() == rhs.value();
}

inline bool operator!=(const WildcardMatch &lhs, const WildcardMatch &rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace nntp
