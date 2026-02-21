#include <nntp/ArticleRange.h>

#include <charconv>
#include <stdexcept>

namespace nntp
{

ArticleRange::ArticleRange(Article begin) noexcept :
    m_begin(begin),
    m_end(begin),
    m_bounded(false)
{
}

ArticleRange::ArticleRange(Article begin, Article end) :
    m_begin(begin),
    m_end(end)
{
    if (begin.value() > end.value())
    {
        throw std::invalid_argument("ArticleRange: begin must be <= end");
    }
}

std::optional<ArticleRange> ArticleRange::parse(std::string_view range_str) noexcept
{
    if (range_str.empty())
    {
        return std::nullopt;
    }

    auto hyphen_pos = range_str.find('-');
    if (hyphen_pos == std::string_view::npos)
    {
        return std::nullopt;
    }

    std::string_view begin_str = range_str.substr(0, hyphen_pos);
    if (begin_str.empty())
    {
        return std::nullopt;
    }

    Article::ValueType begin_num = 0;
    auto [begin_ptr, begin_ec] = std::from_chars(begin_str.data(), begin_str.data() + begin_str.size(), begin_num);

    if (begin_ec != std::errc() || begin_num == 0)
    {
        return std::nullopt;
    }

    Article begin_article;
    try
    {
        begin_article = Article(begin_num);
    }
    catch (const std::invalid_argument &)
    {
        return std::nullopt;
    }

    if (hyphen_pos == range_str.size() - 1)
    {
        return ArticleRange(begin_article);
    }

    std::string_view end_str = range_str.substr(hyphen_pos + 1);
    if (end_str.empty())
    {
        return std::nullopt;
    }

    Article::ValueType end_num = 0;
    auto [end_ptr, end_ec] = std::from_chars(end_str.data(), end_str.data() + end_str.size(), end_num);

    if (end_ec != std::errc() || end_num == 0)
    {
        return std::nullopt;
    }

    Article end_article;
    try
    {
        end_article = Article(end_num);
    }
    catch (const std::invalid_argument &)
    {
        return std::nullopt;
    }

    if (begin_num > end_num)
    {
        return std::nullopt;
    }

    try
    {
        return ArticleRange(begin_article, end_article);
    }
    catch (const std::invalid_argument &)
    {
        return std::nullopt;
    }
}

std::string to_string(const ArticleRange &range)
{
    std::string result = std::to_string(range.begin().value());
    result += '-';
    if (range.is_bounded())
    {
        result += std::to_string(range.end().value());
    }
    return result;
}

} // namespace nntp
