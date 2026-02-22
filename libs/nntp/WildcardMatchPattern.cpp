#include <nntp/WildcardMatchPattern.h>

#include <stdexcept>
#include <string_view>

namespace nntp
{

namespace
{

bool is_allowed_ascii(unsigned char c) noexcept
{
    return (c >= 0x22 && c <= 0x29) || // " # $ % & ' ( )
        c == 0x2B ||                   // +
        (c >= 0x2D && c <= 0x3E) ||    // - . / 0-9 : ; < = >
        (c >= 0x40 && c <= 0x5A) ||    // @ A-Z
        (c >= 0x5E && c <= 0x7E) ||    // ^ _ ` a-z { | } ~
        c == '*' || c == '?';          // wildcards
}

bool is_valid_utf8(std::string_view value) noexcept
{
    const auto *bytes = reinterpret_cast<const unsigned char *>(value.data());
    const std::size_t size = value.size();

    for (std::size_t i = 0; i < size;)
    {
        const unsigned char c = bytes[i];

        if (c <= 0x7F)
        {
            ++i;
            continue;
        }

        std::size_t remaining;
        if ((c & 0xE0) == 0xC0)
        {
            remaining = 1;
            if (c < 0xC2)
            {
                return false;
            }
        }
        else if ((c & 0xF0) == 0xE0)
        {
            remaining = 2;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            remaining = 3;
            if (c > 0xF4)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (i + remaining >= size)
        {
            return false;
        }

        for (std::size_t j = 1; j <= remaining; ++j)
        {
            const unsigned char cc = bytes[i + j];
            if ((cc & 0xC0) != 0x80)
            {
                return false;
            }
        }

        i += remaining + 1;
    }

    return true;
}

bool is_valid_wildmat_pattern(std::string_view pattern) noexcept
{
    if (pattern.empty() || !is_valid_utf8(pattern))
    {
        return false;
    }

    const auto *bytes = reinterpret_cast<const unsigned char *>(pattern.data());
    const std::size_t size = pattern.size();

    for (std::size_t i = 0; i < size;)
    {
        const unsigned char c = bytes[i];

        if (c <= 0x7F)
        {
            if (!is_allowed_ascii(c))
            {
                return false;
            }
            ++i;
            continue;
        }

        std::size_t remaining = 0;
        if ((c & 0xE0) == 0xC0)
        {
            remaining = 1;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            remaining = 2;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            remaining = 3;
        }

        i += remaining + 1;
    }

    return true;
}

size_t utf8_char_length(unsigned char first_byte) noexcept
{
    if ((first_byte & 0x80) == 0)
    {
        return 1;
    }
    if ((first_byte & 0xE0) == 0xC0)
    {
        return 2;
    }
    if ((first_byte & 0xF0) == 0xE0)
    {
        return 3;
    }
    if ((first_byte & 0xF8) == 0xF0)
    {
        return 4;
    }
    return 1;
}

} // namespace

WildcardMatchPattern::WildcardMatchPattern(std::string_view pattern) :
    m_pattern(pattern)
{
    if (!is_valid_wildmat_pattern(m_pattern))
    {
        throw std::invalid_argument("Invalid wildmat pattern");
    }
}

bool WildcardMatchPattern::match(const Newsgroup &group) const
{
    const auto *p_bytes = reinterpret_cast<const unsigned char *>(m_pattern.data());
    const auto *t_bytes = reinterpret_cast<const unsigned char *>(group.value().data());

    std::size_t pi = 0;
    std::size_t ti = 0;
    std::size_t star_pi = 0;
    std::size_t star_ti = 0;
    bool star_found = false;

    while (ti < group.value().size())
    {
        if (pi < m_pattern.size() && p_bytes[pi] == '*')
        {
            star_found = true;
            star_pi = pi + 1;
            star_ti = ti;
            ++pi;
            continue;
        }

        if (pi < m_pattern.size() && p_bytes[pi] == '?')
        {
            ti += utf8_char_length(t_bytes[ti]);
            ++pi;
            continue;
        }

        if (pi < m_pattern.size())
        {
            size_t p_len = utf8_char_length(p_bytes[pi]);
            size_t t_len = utf8_char_length(t_bytes[ti]);

            if (p_len == t_len && m_pattern.substr(pi, p_len) == group.value().substr(ti, t_len))
            {
                pi += p_len;
                ti += t_len;
                continue;
            }
        }

        if (star_found)
        {
            pi = star_pi;
            star_ti += utf8_char_length(t_bytes[star_ti]);
            ti = star_ti;
            continue;
        }

        return false;
    }

    while (pi < m_pattern.size() && p_bytes[pi] == '*')
    {
        ++pi;
    }

    return pi == m_pattern.size();
}

} // namespace nntp
