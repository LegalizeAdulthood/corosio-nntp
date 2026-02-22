#include <nntp/Newsgroup.h>

#include "utf8.h"

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
