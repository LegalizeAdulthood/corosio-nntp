#pragma once

#include <cstddef>
#include <string_view>

namespace nntp
{

bool is_valid_utf8(std::string_view value) noexcept;

std::size_t utf8_char_length(unsigned char first_byte) noexcept;

} // namespace nntp
