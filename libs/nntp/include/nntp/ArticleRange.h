#pragma once

#include <nntp/Article.h>

#include <optional>
#include <string>
#include <string_view>

namespace nntp
{

/// Represents a range of NNTP article numbers
/// Always valid: default construction yields 1-1 (single article)
/// Can be bounded (begin-end) or unbounded (begin-)
class ArticleRange
{
public:
    /// Default constructor - represents the bounded range 1-1
    ArticleRange() noexcept = default;

    /// Constructor for unbounded range from single article (begin-)
    explicit ArticleRange(Article begin) noexcept;

    /// Constructor for bounded range from pair of articles (begin-end)
    /// Throws std::invalid_argument if begin > end
    ArticleRange(Article begin, Article end);

    /// Parse article range from string
    /// Returns std::nullopt if parsing fails
    static std::optional<ArticleRange> parse(std::string_view range_str) noexcept;

    // clang-format off
    /// Check if range is bounded (begin-end format)
    bool is_bounded() const noexcept { return m_bounded; }

    /// Get beginning of range
    Article begin() const noexcept { return m_begin; }

    /// Get end of range (always defined; only meaningful if bounded)
    Article end() const noexcept { return m_end; }
    // clang-format on

private:
    Article m_begin;
    Article m_end;
    bool m_bounded{true};
};

inline bool operator==(const ArticleRange &lhs, const ArticleRange &rhs) noexcept
{
    return lhs.begin() == rhs.begin() && lhs.end() == rhs.end() && lhs.is_bounded() == rhs.is_bounded();
}

inline bool operator!=(const ArticleRange &lhs, const ArticleRange &rhs) noexcept
{
    return !(lhs == rhs);
}

/// Convert article range to NNTP protocol string format
std::string to_string(const ArticleRange &range);

} // namespace nntp
