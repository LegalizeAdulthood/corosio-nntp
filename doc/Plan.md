# Sans-I/O NNTP Library Implementation Plan

## Overview

Implement a sans-I/O style protocol library for NNTP (Network News Transfer
Protocol) in `libs/nntp`, following the architectural patterns established by
the `boost_http` library. A sans-I/O library separates protocol state machine
and message formatting logic from I/O operations, enabling protocol handling
independent of how data is transmitted or received.

## Architecture Principles

- Protocol-centric: Core logic focuses purely on NNTP protocol handling
- I/O agnostic: No socket, file, or network operations in core library
- State machine based: Command processing follows well-defined state transitions
- Buffer management: Library works with externally-supplied buffers
- Error handling: System error codes for all error conditions
- Zero-copy where possible: Minimal data copying during parsing

## Core Components

### 1. Parser Components

#### 1.1 Response Parser
- Parse NNTP server responses (3-digit status code + optional text)
- Handle multi-line responses (terminated by "." on own line)
- Extract status codes and response data
- Support for continuation lines in article bodies

#### 1.2 Capability Parser
- Parse `CAPABILITIES` response (multiline format)
- Extract supported NNTP commands
- Identify supported `LIST` subcommands
- Parse extension information and parameters
- Track server capabilities for feature negotiation

#### 1.3 Article Parser
- Parse article metadata (headers)
- Extract RFC 5322 compliant message structure
- Handle article bodies and attachments
- Support for MIME encoding
- Validate UTF-8 in headers and body
- Decode RFC 2047 encoded-word syntax for non-ASCII headers

### 2. State Machines

#### 2.1 Client Connection State Machine
States:
- Disconnected
- Connecting
- Connected (awaiting server greeting)
- Authenticated
- Ready for commands
- Error/Disconnecting

#### 2.2 Command State Machine
- Track current command in progress
- Manage command pipelining
- Handle responses for multi-step operations

### 3. Client API

#### 3.1 Argument Value Types

NNTP commands use the following semantic value types for arguments:

| Type | Description | Format/Constraints | Example |
|---|---|---|---|
| `article-spec` | Article reference by number or message-id | Either numeric article number or message-id enclosed in `<>` | `"1234"` or `"<abc@example.com>"` |
| `article-num` | Numeric article identifier | Positive integer | `"42"` |
| `article-range` | Range of article numbers | `start-end` or `start-` | `"1-100"`, `"500-"` |
| `message-id` | RFC 5322 message identifier | Enclosed in angle brackets `<>` | `"<message@example.com>"` |
| `newsgroup` | Newsgroup name | Hierarchical dot-separated identifier | `"comp.lang.c"` |
| `wildmat` | Wildcard pattern for newsgroup matching | Shell-style pattern with `*` and `?` | `"comp.lang.*"`, `"comp.*.c"` |
| `header-name` | NNTP header field identifier | Field name (case-insensitive) | `"Subject"`, `"From"` |
| `search-pattern` | Pattern for header value searches | Wildmat or regex pattern | `"*boost*"` |
| `date` | Calendar date | YYMMDD format | `"200115"` (Jan 15, 2020) |
| `time` | Time of day | HHMMSS format | `"143022"` (14:30:22 UTC) |
| `username` | Authentication username | UTF-8 string | `"reader"` |
| `password` | Authentication password | UTF-8 string | `"secret"` |
| `mechanism` | SASL authentication mechanism | Mechanism identifier | `"LOGIN"`, `"PLAIN"` |
| `algorithm` | Compression algorithm | Algorithm identifier | `"DEFLATE"`, `"GZIP"` |

##### 3.1.1 Message-ID Requirements (NNTP)

- RFC 3977 grammar: `message-id = "<" 1*248A-NOTGT ">"`.
- Length: 1 to 248 `A-NOTGT` characters between the angle brackets.
- `A-CHAR` is US-ASCII `%x21-7E` (printable, excluding controls and SP).
- `A-NOTGT` is `A-CHAR` excluding `>`.
- ASCII only: no UTF-8 or non-ASCII code points.
- No surrounding whitespace, comments, or folding.

##### 3.1.2 Newsgroup-Name Requirements (NNTP)

- RFC 3977 grammar: `newsgroup-name = 1*wildmat-exact`.
- Minimum length: 1 character.
- `wildmat-exact` is `%x22-29 / %x2B / %x2D-3E / %x40-5A / %x5E-7E / UTF8-non-ascii`.
- Excludes wildcard characters: `! * , ? [ \ ]`.
- Excludes: space, control characters.
- Allows UTF-8 for international newsgroup names.
- Typical format: hierarchical dot-separated components (e.g., `comp.lang.c`)

#### 3.2 Method Interface

The protocol handler provides a typed method interface for newsreader clients. Each NNTP command from section 4.1 has a corresponding method on the handler. Clients call these methods directly, passing command arguments as strings:

```cpp
// Example: Group selection command becomes a method call
error_code result = handler.group("comp.lang.c", request_id);
if (!result) {
    // Process response
}
```

Method signatures follow the pattern:
```cpp
std::error_code command_name(argument1, argument2, ..., request_id&);
```

where:
- Arguments are strings (`string_view`) supplied by the caller
- The handler validates arguments against server capabilities before execution
- `request_id` is assigned and returned for response correlation
- Return value is `error_code` - zero indicates successful submission
- The handler prevents execution of commands not advertised by the server

### 4. Protocol Data Structures

#### 4.1 Commands

Standard RFC 3977 Connection Commands:
- `CAPABILITIES`: Advertise server capabilities
- `MODE READER`: Switch to reader mode
- `QUIT`: Close connection
- `HELP`: List available commands
- `DATE`: Get server date/time

Standard RFC 3977 Article Selection Commands:
- `STAT article-spec`: Get article status
- `NEXT`: Select next article
- `LAST`: Select previous article
- `ARTICLE article-spec`: Get entire article
- `HEAD article-spec`: Get article headers
- `BODY article-spec`: Get article body

Standard RFC 3977 Newsgroup Selection Commands:
- `GROUP newsgroup`: Select newsgroup
- `LISTGROUP [newsgroup [range]]`: List article numbers

Standard RFC 3977 Listing Commands:
- `LIST [ACTIVE [wildmat]]`: List newsgroups
- `LIST ACTIVE.TIMES [wildmat]`: List newsgroups with timestamps
- `LIST NEWSGROUPS [wildmat]`: List newsgroup descriptions
- `LIST DISTRIB.PATS`: Distribution patterns
- `LIST HEADERS [MSGID|RANGE]`: Supported header fields
- `LIST OVERVIEW.FMT`: Overview format fields
- `NEWGROUPS date time [GMT]`: List new newsgroups
- `NEWNEWS wildmat date time [GMT]`: List new articles

RFC 6048 Extended LIST Subcommands:
- `LIST COUNTS [wildmat]`: List article counts by newsgroup
- `LIST DISTRIBUTIONS`: Distributions list
- `LIST MODERATORS [wildmat]`: Moderator information
- `LIST MOTD`: Message of the day
- `LIST SUBSCRIPTIONS [wildmat]`: Subscriptions

Standard RFC 3977 Article Transfer Commands:
- `XOVER message-spec`: Get article overview data
- `XPAT header message-spec pattern`: Search headers
- `XHDR header [message-spec]`: Get header from articles
- `XGTITLE [wildmat]`: Get newsgroup titles
- `XPATH message-id`: Get article path

Standard RFC 3977 Post/Feed Commands:
- `POST`: Begin posting article
- `IHAVE message-id`: Begin article transmission
- `CHECK message-id`: Check if article exists
- `TAKETHIS`: Accept article in stream mode
- `SLAVE`: Set connection to slave mode

Standard RFC 3977 Authentication Commands:
- `AUTHINFO USER username`: Authenticate username
- `AUTHINFO PASS password`: Authenticate password
- `AUTHINFO SASL mechanism`: Begin SASL authentication

INN-Specific Extensions:
- `XREPLIC [start-time [end-time [flag]]]`: Replication info
- `XROVER message-spec`: Extended overview
- `XFEATURE [feature ...]`: Feature negotiation
- `COMPRESS algorithm`: Enable compression (GZIP/DEFLATE)
- `STREAMING`: Enable streaming mode

#### 4.2 Responses

- 1xx: Informational
- 2xx: Success
- 3xx: Success, further action needed
- 4xx: Temporary failure
- 5xx: Permanent failure

### 5. Configuration
- Buffer size limits
- Maximum timeout values
- Pipelining support
- Character encoding handling
- UTF-8 validation per RFC 3977 section 3.1

### 6. UTF-8 Support per RFC 3977

RFC 3977 UTF-8 Rules (Section 3.1):

Command and Response Lines:
- Maximum 998 octets (commands)
- Maximum 1024 octets (responses)
- Most text must be US-ASCII only

UTF-8 Allowed In:
- Newsgroup names (section 6.1.1)
- Article headers (RFC 5322/RFC 2047)
- CAPABILITIES response text (section 3.3.1)
- LIST NEWSGROUPS descriptions
- Multi-line response bodies (articles, overview)

UTF-8 Forbidden In:
- Command names (always ASCII)
- Response codes (always ASCII digits)
- Message IDs (RFC 5322 format, no UTF-8)
- Article numbers (numeric only)
- Date/time values
- Username/password fields

Encoding Requirements:
- UTF-8 encoded strings must be valid sequences
- Invalid UTF-8 should be rejected as protocol error
- Newsgroup names with UTF-8 must use percent-encoding in some contexts
- Article headers with non-ASCII use RFC 2047 MIME encoding

### 7. Command-to-Capability Mapping

Each NNTP command listed below maps to a typed method on the protocol handler interface. Newsreader clients call these methods directly, supplying command arguments as string parameters. The library enforces that only commands supported by the server (based on its advertised capabilities) can be called.

RFC 3977 Capability Requirements and INN-Specific Extensions:

| Capability | Description |
|---|---|
| `IMPLEMENTATION` | Server implementation string (informational) |
| `VERSION` | Server version string (informational) |
| `READER` | Server supports reader commands (`STAT`, `NEXT`, `LAST`, `ARTICLE`, `HEAD`, `BODY`, `GROUP`, `LISTGROUP`, etc.) |
| `POSTING-OK` | Server allows article posting (`POST` command) |
| `POST` | Server supports posting (alias for `POSTING-OK` in some servers) |
| `IHAVE` | Server supports feed mode (`IHAVE`, `CHECK`, `TAKETHIS` commands) |
| `STREAMING` | Server supports streaming mode |
| `STARTTLS` | Server supports TLS upgrade |
| `AUTHINFO SASL` | Server supports SASL authentication (`AUTHINFO SASL` command) |
| `COMPRESS DEFLATE` | Server supports DEFLATE compression |
| `COMPRESS GZIP` | Server supports GZIP compression |
| `LIST ACTIVE` | `LIST ACTIVE` subcommand supported |
| `LIST NEWSGROUPS` | `LIST NEWSGROUPS` subcommand supported |
| `LIST COUNTS` | `LIST COUNTS` subcommand supported (RFC 6048) |
| `LIST DISTRIBUTIONS` | `LIST DISTRIBUTIONS` subcommand supported |
| `LIST OVERVIEW.FMT` | `LIST OVERVIEW.FMT` subcommand supported |
| `XOVER` | `XOVER` command supported |
| `XHDR` | `XHDR` command supported |
| `XPAT` | `XPAT` command supported |
| `XGTITLE` | `XGTITLE` command supported |
| | **INN-Specific Extensions** |
| `XREPLIC` | `XREPLIC` command supported |
| `XROVER` | `XROVER` command supported |
| `XFEATURE` | Feature negotiation supported |

## File Structure

```
libs/nntp/
├── include/
│   └── nntp/
│       ├── client.h                    (EXISTING - update)
│       ├── config.hpp                  (NEW)
│       ├── error.hpp                   (NEW)
│       ├── parser/
│       │   ├── response_parser.hpp     (NEW)
│       │   ├── capability_parser.hpp   (NEW)
│       │   └── article_parser.hpp      (NEW)
│       ├── formatter/
│       │   └── command_formatter.hpp   (NEW)
│       ├── core/
│       │   ├── connection.hpp          (NEW)
│       │   ├── capabilities.hpp        (NEW)
│       │   ├── command.hpp             (NEW)
│       │   ├── response.hpp            (NEW)
│       │   └── article.hpp             (NEW)
│       └── detail/
│           ├── parser_state.hpp        (NEW)
│           ├── buffer_utils.hpp        (NEW)
│           └── constants.hpp           (NEW)
├── client.cpp                          (EXISTING - update)
├── error.cpp                           (NEW)
├── response_parser.cpp                 (NEW)
├── capability_parser.cpp               (NEW)
├── article_parser.cpp                  (NEW)
├── command_formatter.cpp               (NEW)
├── connection.cpp                      (NEW)
├── article.cpp                         (NEW)
├── buffer_utils.cpp                    (NEW)
└── CMakeLists.txt                      (EXISTING - update)
```

## Implementation Phases

### Phase 1: Foundation (Core Infrastructure)

#### Step 1: Define Error Categories
- Create `error.hpp` and `error.cpp`
- Define NNTP-specific error conditions
- Implement error code enumeration
- Add error message translations

#### Step 2: Define Configuration and Constants
- Create `config.hpp` for compile-time configuration
- Create `detail/constants.hpp` for protocol constants
- Define response code ranges
- Set buffer size limits and timeouts
- Define UTF-8 validation rules per RFC 3977 section 3.1
- Set maximum command line (998 octets) and response line (1024 octets) lengths

#### Step 3: Create Response Data Structures
- Implement `core/response.hpp` for server response representation
- Define response code enumeration
- Create response status class
- Add response text and data storage

#### Step 4: Create Capability Data Structures
- Implement `core/capabilities.hpp` for server capability tracking
- Define supported commands enumeration
- Track supported LIST subcommands
- Store IMPLEMENTATION string and VERSION
- Store SASL mechanisms list
- Parse and store capability parameters
- Add capability query interface (is_command_supported, is_list_subcommand_supported)
- Provide const reference access to capabilities
- Implement mapping of command to required capability (per RFC 3977)

#### Step 5: Create Command Data Structures
- Implement `core/command.hpp` for client commands
- Define command enumeration (all RFC 3977 + INN extensions)
- Create command argument storage with variant types
- Add command validation interface
- Support variable argument counts and types

#### Step 6: Create Article Data Structures
- Implement `core/article.hpp` for article representation
- Define article metadata class
- Create header map for headers
- Add article body storage

### Phase 2: Parsers (Protocol Parsing Logic)

#### Step 7: Implement Response Parser
- Create `parser/response_parser.hpp` and `.cpp`
- Parse 3-digit status codes
- Handle single and multi-line responses
- Extract response data
- Track parser state transitions
- Validate UTF-8 in response text where allowed (per RFC 3977 section 3.1)

#### Step 8: Implement Capability Parser
- Create `parser/capability_parser.hpp` and `.cpp`
- Parse `CAPABILITIES` multiline response
- Extract `IMPLEMENTATION` string from response
- Extract `VERSION` number from response
- Extract complete command list from capabilities
- Identify supported `LIST` subcommands
- Parse `SASL` mechanisms (if present)
- Parse extension information and parameters
- Build capability bitmap for efficient lookup
- Validate UTF-8 in `IMPLEMENTATION` and `VERSION` strings

#### Step 9: Implement Command Formatter
- Create `formatter/command_formatter.hpp` and `.cpp`
- Serialize typed command objects to `NNTP` protocol strings
- Format command arguments per RFC 3977 syntax
- Handle optional parameters and method overloads
- Quote strings and escape special characters
- Validate argument formats before serialization
- Validate line length before returning formatted command (998 octets max)
- Handle UTF-8 encoding where allowed per RFC 3977

#### Step 10: Create Buffer Utilities
- Implement `detail/buffer_utils.hpp` and `.cpp`
- Create buffer inspection functions
- Add safe buffer access utilities
- Implement line boundary detection
- Create view/substring helpers
- Add UTF-8 validation function
- Add UTF-8 sequence length detection
- Add line length validation (998 octets for commands, 1024 for responses)

### Phase 3: State Management (Protocol State Machines)

#### Step 11: Implement Connection State Machine
- Create `core/connection.hpp` and `.cpp`
- Define connection states
- Implement state transitions
- Handle connection lifecycle
- Add state validation
- Track server capabilities after CAPABILITIES response

#### Step 12: Implement Command State Machine
- Extend `core/command.hpp` with state tracking
- Add command queue/pipeline management
- Handle command acknowledgment
- Implement response correlation
- Validate commands against server capabilities (using RFC 3977 mapping)
- Return error for unsupported commands (per RFC 3977 section 5.3)
- Track which capabilities are required for each command
- Consult capabilities_to_commands mapping when validating

#### Step 13: Create Parser State Machine
- Implement `detail/parser_state.hpp`
- Track position in multi-line parsing
- Handle partial message buffers
- Manage state across buffer refills

### Phase 4: Integration (High-Level API)

#### Step 14: Create Protocol Handler
- Add high-level protocol management class
- Coordinate command submission
- Correlate commands with responses
- Implement command ordering
- Integrate capability negotiation

#### Step 15: Update Client Interface
- Update `client.h` and `client.cpp`
- Integrate new sans-I/O components
- Adapt existing I/O operations
- Add sans-I/O query methods
- Expose server capabilities to caller

#### Step 16: Update CMakeLists.txt
- Add new source files to build
- Ensure proper linking
- Update include directories

### Phase 5: Documentation and Examples

#### Step 17: Add API Documentation
- Document public interfaces
- Add usage examples in headers
- Create architecture guide
- Document state machines
- Document capability negotiation flow

#### Step 18: Create Unit Tests
- Test parser components independently
- Test state machines
- Test buffer handling
- Verify error conditions
- Test capability negotiation
- **Test specific for CAPABILITIES parsing and command validation**
- UTF-8 validation tests (valid sequences, invalid sequences)
- Line length boundary tests (998 and 1024 octets)
- RFC 2047 encoded-word parsing tests
- Tests for UTF-8 rejection in forbidden contexts (command names, response codes, message IDs)

## Implementation Details

### Key Design Decisions

1. Parser State Machines:
   - Use enumeration-based states for efficiency
   - Implement incremental parsing for streaming data
   - Support partial buffer processing

2. Buffer Strategy:
   - Accept external buffer ownership
   - Provide view types (string_view, span) for zero-copy
   - Report consumed bytes and required buffer size

3. Error Handling:
   - Use std::error_code for all errors
   - Define custom error category
   - Include recoverable vs. fatal errors

4. Type Safety:
   - Strong typing for commands and responses
   - Type-safe state machine transitions
   - Compile-time assertions where applicable

5. Command Validation:
   - All command submissions checked against capabilities
   - Return error_code if command not supported by server
   - Library prevents sending unsupported commands (fail-fast)
   - Query interface allows clients to check capability before command submission
   - Capability bitmap enables O(1) lookup time

## Success Criteria

1. All NNTP protocol operations parseable without I/O
2. Zero-copy parsing where possible
3. Protocol-agnostic from socket/file perspective
4. Comprehensive error handling
5. State machine invariants maintained
6. Thread-safe library design
7. Performance competitive with monolithic approaches
8. Clear separation of concerns
9. Testable in isolation
10. Production-ready error recovery
11. All `CAPABILITIES` response fields parsed accurately
12. Command submission prevented for unsupported commands
13. Capability queries available to application code
14. UTF-8 validation per RFC 3977 section 3.1
15. Proper line length enforcement (998/1024 octets)
16. RFC 2047 encoded-word support for article headers

## References

- NNTP RFC 3977 (NNTP Protocol) - See sections 3.1 (Line Format, UTF-8), 5.3 (Capability Requirements), 7 (Commands)
- NNTP RFC 4642 (NNTP over TLS)
- NNTP RFC 6048 (Additional NNTP Commands)
- RFC 5322 (Internet Message Format) - Article header format
- RFC 2047 (MIME Encoded-Word Syntax) - Non-ASCII headers
- RFC 3629 (UTF-8 Validation) - UTF-8 character validation
- SMTP RFC 5321 (command/response patterns)
- INN News Server (https://www.eyrie.org/~eagle/software/inn/) documentation
- HTTP sans-I/O implementations for patterns
