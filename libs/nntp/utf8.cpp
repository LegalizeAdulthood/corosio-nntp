#include "utf8.h"

namespace nntp
{

std::size_t utf8_char_length(unsigned char first_byte) noexcept
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

        std::size_t char_len = utf8_char_length(c);

        if (char_len == 1)
        {
            return false;
        }

        if ((c & 0xE0) == 0xC0)
        {
            if (c < 0xC2)
            {
                return false;
            }
        }
        else if ((c & 0xF8) == 0xF0)
        {
            if (c > 0xF4)
            {
                return false;
            }
        }

        if (i + char_len > size)
        {
            return false;
        }

        for (std::size_t j = 1; j < char_len; ++j)
        {
            const unsigned char cc = bytes[i + j];
            if ((cc & 0xC0) != 0x80)
            {
                return false;
            }
        }

        i += char_len;
    }

    return true;
}

} // namespace nntp
