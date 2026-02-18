# Control/Data Flow for a Multi-Server NNTP News Reader

## Read trn Style Access File

The access file is an INI file with named sections.  Each section has a
name in square brackets ([]) and a list of name/value pairs.  A line
beginning with '#' is a comment line and is to be skipped.  Sections
may or may not be separated by blank lines, but a line beginning with
'[' always designates a new section.

A section either names a collection of settings for a News Source
or it names a Browsing Group.  A Browsing Group has the section name
'Group N' where N defines the order in which trn will contact a server
and display articles.  A Browsing Group references a News Source by
it's section name.

With asynchronous coroutines, we want to read this file and split the input
into lines.  The lines are fed to an INI parser that creates a map from
section name to name/value pairs.

A News Source may be defined and unreferenced by any Browsing Group.

## Browsing Groups

For each Browsing Group:

- If the News Source for this Browsing Group does not exist, skip the
  group.
- If the News Source is not NNTP, skip the group
- Initialize Server structure with information from the Browsing Group
  and associated News Source
- Read newsrc file associated with Browsing Group
  - Split the newsrc file into lines
  - Lines contain a newsgroup name, a subscription identifier and an
    optional list of articles
  - Store a map of newsgroup map to subscription status and article list in
    the Server structure.
- Resolve host name and port name via DNS
- For each resolver result:
  - Attempt connection to the IP address and port
  - If no resolved addresses connected, skip this Browsing Group.
- Receive NNTP greeting from server
- Send CAPABILITIES command to server
- Read capabilities list from server
- Parse capabilities list into Server structure
- Check for new newsgroups
- Check for new articles in subscribed groups
- Read headers of new articles
- Perform KILL processing of new articles
- Apply threading/scoring algorithm to new articles
- Display new article summary for current Browsing Group
- Console input navigates between:
  - Browsing Groups
  - Newsgroups within a Browsing Group
  - Articles within a newsgroup

From the user's perspective, things are "blocking" until enough information
is obtained about the first valid Browsing Group to either:

- Prompt the user about new newsgroups
- Prompt the user to read new articles in subscribed newsgroup

If no new newsgroups and no new articles for this Browsing Group are
available, then the next Browsing Group is shown.

While waiting for user input, the reader should be asynchronously gathering
information on Browsing Groups, newsgroups and articles.  It should be
eagerly applying KILL file rules and other processing while the user is
reading an article or deciding about newsgroup subscription.

# NNTP Conversation

Just as [boost.http (unofficial)](https://github.com/cppalliance/http) models
the HTTP request and response in a manner that is ignorant of the underlying
transport, an NNTP client (and server) doesn't care about the particular I/O
transport involved.  The client sends commands to a server and receives
responses.  The server accepts commands and sends responses.

Unlike HTTP, NNTP is stateful.  There is a notion of the current newsgroup,
article, etc.  The connection is always persistent.  Additionally, there is the
STARTTLS command that switches a connection from insecure to secure using TLS
after the connection has been made.  This is the one command that directly
interacts with the underlying transport.

This implies that there is an NNTP library that understands the vocabulary of
usenet: newsgroup names, wildcard patterns, articles, article numbers, message
ids, the anatomy of an article: header, body and so-on.  Like the boost.http
library there's a lot of vocabulary here that should be represented explicitly
by types for composition and clarity of API.  There's a bunch of stuff that can
be stolen from boost.http, but there are also some important differences with
regards to message formatting.  HTTP doesn't line-fold headers, for instance.

From slack discussion on `STARTTLS`:

sgerbino wrote:
> Claude suggested something like this:

```cpp
// Caller -- owns the TLS policy
capy::task<void>
serve_nntp(tcp_socket sock, tls_context* tls_ctx, bool implicit_tls)
{
    capy::any_stream stream(std::move(sock));

    // Port 563: wrap in TLS before protocol starts
    if(implicit_tls)
    {
        openssl_stream tls(std::move(stream), *tls_ctx);
        co_await tls.handshake(tls_stream::server);
        stream = capy::any_stream(std::move(tls));
    }

    nntp_session session;
    auto act = co_await session.run(
        stream, tls_ctx && !implicit_tls);

    if(act == nntp_session::action::starttls)
    {
        // 382 already sent by protocol layer
        openssl_stream tls(std::move(stream), *tls_ctx);
        co_await tls.handshake(tls_stream::server);
        stream = capy::any_stream(std::move(tls));

        // Re-enter -- TLS available=false prevents double upgrade
        co_await session.run(stream, false);
    }
}
```

vinnie wrote:
> type-erase two streams
> alternatively encapsulate the non-regular behavior in a function pointer or a virtual member

All of this seems to imply that the first step is to create a sans-I/O NNTP
library with the vocabulary involved in NNTP conversations.
