#pragma once

#include <nntp/Article.h>
#include <nntp/MessageId.h>

#include <string>
#include <variant>

namespace nntp
{

class ArticleSpec
{
public:
    // clang-format off
    explicit ArticleSpec(Article article) : m_value(article) {}
    explicit ArticleSpec(MessageId message_id) : m_value(std::move(message_id)) {}
    // clang-format on
    
    ~ArticleSpec() = default;
    
    ArticleSpec(const ArticleSpec& other) = default;
    ArticleSpec(ArticleSpec&& other) noexcept = default;
    ArticleSpec& operator=(const ArticleSpec& other) = default;
    ArticleSpec& operator=(ArticleSpec&& other) noexcept = default;
    
    // clang-format off
    bool is_article() const noexcept { return std::holds_alternative<Article>(m_value); }
    bool is_message_id() const noexcept { return std::holds_alternative<MessageId>(m_value); }
    
    Article as_article() const { return std::get<Article>(m_value); }
    const MessageId& as_message_id() const { return std::get<MessageId>(m_value); }
    // clang-format on
    
    std::string value() const
    {
        return is_article() ? std::to_string(std::get<Article>(m_value).value()) : std::get<MessageId>(m_value).value();
    }

    // clang-format off
    friend bool operator==(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept { return lhs.m_value == rhs.m_value; }
    friend bool operator!=(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept { return lhs.m_value != rhs.m_value; }
    friend bool operator<(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept  { return lhs.m_value < rhs.m_value; }
    friend bool operator<=(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept { return lhs.m_value <= rhs.m_value; }
    friend bool operator>(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept  { return lhs.m_value > rhs.m_value; }
    friend bool operator>=(const ArticleSpec& lhs, const ArticleSpec& rhs) noexcept { return lhs.m_value >= rhs.m_value; }
    // clang-format on

private:
    std::variant<Article, MessageId> m_value;
};

} // namespace nntp
