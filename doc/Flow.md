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
