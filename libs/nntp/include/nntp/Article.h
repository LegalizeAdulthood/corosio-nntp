#pragma once

#include <cstdint>
#include <stdexcept>

namespace nntp
{

class Article
{
public:
    using ValueType = std::uint64_t;

    Article() :
        m_value(1)
    {
    }

    explicit Article(ValueType value) :
        m_value(value)
    {
        if (m_value < 1)
        {
            throw std::invalid_argument("Article must be positive and non-zero.");
        }
    }

    // clang-format off
    ValueType value() const noexcept                            { return m_value; }
    explicit operator ValueType() const noexcept                { return m_value; }

    friend bool operator==(Article lhs, Article rhs) noexcept   { return lhs.m_value == rhs.m_value; }
    friend bool operator!=(Article lhs, Article rhs) noexcept   { return lhs.m_value != rhs.m_value; }
    friend bool operator<(Article lhs, Article rhs) noexcept    { return lhs.m_value <  rhs.m_value; }
    friend bool operator<=(Article lhs, Article rhs) noexcept   { return lhs.m_value <= rhs.m_value; }
    friend bool operator>(Article lhs, Article rhs) noexcept    { return lhs.m_value >  rhs.m_value; }
    friend bool operator>=(Article lhs, Article rhs) noexcept   { return lhs.m_value >= rhs.m_value; }
    // clang-format on

private:
    ValueType m_value;
};

} // namespace nntp
