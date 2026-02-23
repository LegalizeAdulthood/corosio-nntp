#pragma once

#include <nntp/ArticleRange.h>
#include <nntp/MessageId.h>
#include <nntp/Newsgroup.h>
#include <nntp/WildcardMatch.h>

#include <boost/capy/io/any_stream.hpp>
#include <boost/capy/task.hpp>

#include <string>
#include <string_view>

namespace nntp
{

template <typename T>
using Task = boost::capy::task<T>;

class Client
{
public:
    static Task<Client> connect(boost::capy::any_stream &stream);

    Client() = default;
    explicit Client(boost::capy::any_stream &stream);
    Client(const Client &rhs) = delete;
    Client &operator=(const Client &rhs) = delete;
    Client(Client &&rhs) noexcept = default;
    Client &operator=(Client &&rhs) noexcept = default;
    ~Client() = default;

    // RFC 3977 Connection Commands
    Task<std::string> capabilities();
    Task<void> mode_reader();
    Task<void> quit();
    Task<std::string> help();
    Task<std::string> date();

    // RFC 3977 Article Selection Commands
    Task<std::string> stat();
    Task<std::string> stat(unsigned int article_num);
    Task<std::string> stat(const MessageId &message_id);
    Task<std::string> next();
    Task<std::string> last();
    Task<std::string> article();
    Task<std::string> article(unsigned int article_num);
    Task<std::string> article(const MessageId &message_id);
    Task<std::string> head();
    Task<std::string> head(unsigned int article_num);
    Task<std::string> head(const MessageId &message_id);
    Task<std::string> body();
    Task<std::string> body(unsigned int article_num);
    Task<std::string> body(const MessageId &message_id);

    // RFC 3977 Newsgroup Selection Commands
    Task<std::string> group(const Newsgroup &newsgroup);
    Task<std::string> list_group();
    Task<std::string> list_group(const Newsgroup &newsgroup);
    Task<std::string> list_group(const Newsgroup &newsgroup, const ArticleRange &range);

    // RFC 3977 Listing Commands
    Task<std::string> list_active();
    Task<std::string> list_active(const WildcardMatch &pattern);
    Task<std::string> list_active_times();
    Task<std::string> list_active_times(const WildcardMatch &pattern);
    Task<std::string> list_newsgroups();
    Task<std::string> list_newsgroups(const WildcardMatch &pattern);
    Task<std::string> list_distrib_pats();
    Task<std::string> list_headers();
    Task<std::string> list_overview_fmt();
    Task<std::string> list_counts();
    Task<std::string> list_counts(const WildcardMatch &pattern);
    Task<std::string> list_distributions();
    Task<std::string> list_moderators();
    Task<std::string> list_moderators(const WildcardMatch &pattern);
    Task<std::string> list_motd();
    Task<std::string> list_subscriptions();
    Task<std::string> list_subscriptions(const WildcardMatch &pattern);
    Task<std::string> newgroups(std::string_view date, std::string_view time);
    Task<std::string> newgroups(std::string_view date, std::string_view time, bool gmt);
    Task<std::string> new_news(const WildcardMatch &pattern, std::string_view date, std::string_view time);
    Task<std::string> new_news(const WildcardMatch &pattern, std::string_view date, std::string_view time, bool gmt);

    // RFC 3977 Article Header/Overview Commands
    Task<std::string> hdr(std::string_view field);
    Task<std::string> hdr(std::string_view field, unsigned int article_num);
    Task<std::string> hdr(std::string_view field, const MessageId &message_id);
    Task<std::string> hdr(std::string_view field, const ArticleRange &range);
    Task<std::string> over();
    Task<std::string> over(const ArticleRange &range);

    // RFC 3977 Post/Feed Commands
    Task<std::string> post(std::string_view article_text);
    Task<std::string> i_have(const MessageId &message_id, std::string_view article_text);
    Task<std::string> check(const MessageId &message_id);
    Task<std::string> take_this(const MessageId &message_id, std::string_view article_text);
    Task<void> slave();

    // RFC 3977 Authentication Commands
    Task<std::string> auth_info_user(std::string_view username);
    Task<std::string> auth_info_pass(std::string_view password);
    Task<std::string> auth_info_sasl(std::string_view mechanism);

    // RFC 3977 TLS Support
    Task<void> start_tls();

    // RFC 2980 Extension Commands
    Task<std::string> x_over();
    Task<std::string> x_over(const ArticleRange &range);
    Task<std::string> x_hdr(std::string_view header);
    Task<std::string> x_hdr(std::string_view header, unsigned int article_num);
    Task<std::string> x_hdr(std::string_view header, const MessageId &message_id);
    Task<std::string> x_hdr(std::string_view header, const ArticleRange &range);
    Task<std::string> x_pat(std::string_view header, const ArticleRange &range, const WildcardMatch &pattern);
    Task<std::string> x_group_title();
    Task<std::string> x_group_title(const WildcardMatch &pattern);
    Task<std::string> x_path(const MessageId &message_id);

private:
    boost::capy::any_stream &m_stream;
};

} // namespace nntp
