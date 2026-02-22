#include <nntp/WildcardMatch.h>

#include <algorithm>
#include <stdexcept>

namespace nntp
{

WildcardMatch::WildcardMatch(std::string_view wildmat) :
    m_value(wildmat)
{
    if (m_value.empty())
    {
        throw std::invalid_argument("WildcardMatch cannot be empty");
    }

    if (m_value.back() == ',')
    {
        throw std::invalid_argument("WildcardMatch cannot have trailing comma");
    }

    std::size_t pos = m_value.size();

    while (true)
    {
        std::size_t search_pos = (pos > 0) ? pos - 1 : 0;
        std::size_t comma_pos = m_value.rfind(',', search_pos);

        std::size_t start = (comma_pos == std::string_view::npos) ? 0 : comma_pos + 1;
        std::size_t end = pos;

        std::string pattern = m_value.substr(start, end - start);

        if (pattern.empty())
        {
            throw std::invalid_argument("WildcardMatch cannot have empty patterns");
        }

        const bool negated{pattern.front() == '!'};
        if (negated)
        {
            pattern = pattern.substr(1);
        }

        if (pattern.empty())
        {
            throw std::invalid_argument("WildcardMatch cannot have empty patterns after negation");
        }

        m_patterns.push_back({WildcardMatchPattern(pattern), negated});

        if (comma_pos == std::string_view::npos)
        {
            break;
        }

        pos = comma_pos;
    }
}

bool WildcardMatch::match(const Newsgroup &group) const
{
    const auto it =
        std::ranges::find_if(m_patterns, [&group](const PatternEntry &e) { return e.pattern.match(group); });

    return it != m_patterns.end() && !it->negated;
}

} // namespace nntp
