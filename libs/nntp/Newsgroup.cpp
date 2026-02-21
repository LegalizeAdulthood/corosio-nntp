#include <nntp/Newsgroup.h>

#include <stdexcept>

namespace nntp
{

namespace
{

bool is_allowed_ascii(const unsigned char c) noexcept
{
    return (c >= 0x22 && c <= 0x29) || // " # $ % & ' ( )
        (c == 0x2B) ||                 // +
        (c >= 0x2D && c <= 0x3E) ||    // - . / 0-9 : ; < = >
        (c >= 0x40 && c <= 0x5A) ||    // @ A-Z
        (c >= 0x5E && c <= 0x7E);      // ^ _ ` a-z { | } ~
}

bool is_valid_utf8(const std::string_view value) noexcept
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

        i += (remaining + 1);
    }

    return true;
}

bool is_valid_newsgroup(const std::string_view value) noexcept
{
    for (unsigned char c : value)
    {
        if (c <= 0x7F)
        {
            if (!is_allowed_ascii(c))
            {
                return false;
            }
        }
        else
        {
            return is_valid_utf8(value);
        }
    }

    return true;
}

} // namespace

Newsgroup::Newsgroup(const std::string_view &value) :
    m_value(value)
{
    if (m_value.empty())
    {
        throw std::invalid_argument("Newsgroup cannot be empty");
    }

    if (!is_valid_newsgroup(m_value))
    {
        throw std::invalid_argument("Newsgroup contains invalid characters");
    }
}

} // namespace nntp
