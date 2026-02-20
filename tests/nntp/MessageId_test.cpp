#include <nntp/MessageId.h>

#include <gtest/gtest.h>

using namespace nntp;

TEST(TestMessageIdConstruct, simpleMessageId)
{
    EXPECT_NO_THROW(MessageId("<abc@example.com>"));
}

TEST(TestMessageIdConstruct, messageIdWithMinLength)
{
    // Minimum: 1 character between brackets
    EXPECT_NO_THROW(MessageId("<a>"));
}

TEST(TestMessageIdConstruct, messageIdWithMaxLength)
{
    // Maximum: 248 characters between brackets
    std::string content(248, 'a');
    std::string msg_id = "<" + content + ">";
    EXPECT_NO_THROW(MessageId{msg_id});
}

TEST(TestMessageIdConstruct, messageIdWithAllAllowedCharacters)
{
    // A-NOTGT: %x21-3D / %x3F-7E (excludes >)
    EXPECT_NO_THROW(MessageId("<abc123@domain.com>"));
    EXPECT_NO_THROW(MessageId("<user+tag@example.org>"));
    EXPECT_NO_THROW(MessageId("<message-id_123.456@host.example>"));
    EXPECT_NO_THROW(MessageId("<!#$%&'*+-./@[\\]^_`{|}~>"));
}

TEST(TestMessageIdConstruct, constructValidMessageIdTypicalFormat)
{
    EXPECT_NO_THROW(MessageId("<20231215143022.12345@news.example.com>"));
}

TEST(TestMessageIdInvalid, missingOpeningBracket)
{
    EXPECT_THROW(MessageId("abc@example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, missingClosingBracket)
{
    EXPECT_THROW(MessageId("<abc@example.com"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, missingBothBrackets)
{
    EXPECT_THROW(MessageId("abc@example.com"), std::invalid_argument);
}

// Invalid message-id construction tests - length violations

TEST(TestMessageIdInvalid, emptyContent)
{
    EXPECT_THROW(MessageId("<>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, emptyString)
{
    EXPECT_THROW(MessageId(""), std::invalid_argument);
}

TEST(TestMessageIdInvalid, tooLongContent)
{
    // 249 characters between brackets (exceeds 248 limit)
    std::string content(249, 'a');
    std::string msg_id = "<" + content + ">";
    EXPECT_THROW(MessageId{msg_id}, std::invalid_argument);
}

// Invalid message-id construction tests - forbidden characters

TEST(TestMessageIdInvalid, greaterThanInContent)
{
    EXPECT_THROW(MessageId("<abc>def@example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, spaceCharacter)
{
    EXPECT_THROW(MessageId("<abc @example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc@ example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, controlCharacters)
{
    EXPECT_THROW(MessageId("<abc\x01@example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc\x0D@example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc\x0A@example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc\t@example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, nonASCIICharacters)
{
    // UTF-8 encoded characters
    EXPECT_THROW(MessageId("<abc\xC3\xA9@example.com>"), std::invalid_argument);     // é
    EXPECT_THROW(MessageId("<abc\xE2\x82\xAC@example.com>"), std::invalid_argument); // €
}

TEST(TestMessageIdInvalid, highByteCharacters)
{
    EXPECT_THROW(MessageId("<abc\xFF@example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc\x80@example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, leadingWhitespace)
{
    EXPECT_THROW(MessageId(" <abc@example.com>"), std::invalid_argument);
    EXPECT_THROW(MessageId("\t<abc@example.com>"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, trailingWhitespace)
{
    EXPECT_THROW(MessageId("<abc@example.com> "), std::invalid_argument);
    EXPECT_THROW(MessageId("<abc@example.com>\t"), std::invalid_argument);
}

TEST(TestMessageIdInvalid, surroundingWhitespace)
{
    EXPECT_THROW(MessageId(" <abc@example.com> "), std::invalid_argument);
}

TEST(TestMessageIdValue, returnsCorrectValue)
{
    MessageId msg_id("<abc@example.com>");
    EXPECT_EQ(msg_id.value(), "<abc@example.com>");
}

TEST(TestMessageIdValue, minLength)
{
    MessageId msg_id("<a>");
    EXPECT_EQ(msg_id.value(), "<a>");
}

TEST(TestMessageIdValue, maxLength)
{
    std::string content(248, 'a');
    std::string msg_id_str = "<" + content + ">";
    MessageId msg_id(msg_id_str);
    EXPECT_EQ(msg_id.value(), msg_id_str);
}

TEST(TestMessageIdCompare, sameValue)
{
    MessageId msg_id1("<abc@example.com>");
    MessageId msg_id2("<abc@example.com>");
    EXPECT_EQ(msg_id1, msg_id2);
}

TEST(TestMessageIdCompare, differentValue)
{
    MessageId msg_id1("<abc@example.com>");
    MessageId msg_id2("<xyz@example.com>");
    EXPECT_NE(msg_id1, msg_id2);
}

TEST(TestMessageIdOrdering, lessThan)
{
    MessageId msg_id1("<aaa@example.com>");
    MessageId msg_id2("<bbb@example.com>");
    EXPECT_LT(msg_id1, msg_id2);
}

TEST(TestMessageIdOrdering, lessThanOrEqual)
{
    MessageId msg_id1("<aaa@example.com>");
    MessageId msg_id2("<bbb@example.com>");
    MessageId msg_id3("<aaa@example.com>");
    EXPECT_LE(msg_id1, msg_id2);
    EXPECT_LE(msg_id1, msg_id3);
}

TEST(TestMessageIdOrdering, greaterThan)
{
    MessageId msg_id1("<bbb@example.com>");
    MessageId msg_id2("<aaa@example.com>");
    EXPECT_GT(msg_id1, msg_id2);
}

TEST(TestMessageIdOrdering, greaterThanOrEqual)
{
    MessageId msg_id1("<bbb@example.com>");
    MessageId msg_id2("<aaa@example.com>");
    MessageId msg_id3("<bbb@example.com>");
    EXPECT_GE(msg_id1, msg_id2);
    EXPECT_GE(msg_id1, msg_id3);
}

TEST(TestMessageIdEdgeCase, acceptsAllPrintableASCIIExceptGreaterThan)
{
    // Test all valid A-NOTGT characters
    for (char c = 0x21; c <= 0x7E; ++c)
    {
        if (c == '>')
        {
            continue; // Skip '>'
        }
        std::string msg_id = "<a" + std::string(1, c) + "b>";
        EXPECT_NO_THROW(MessageId{msg_id}) << "Failed for character: " << static_cast<int>(c);
    }
}

TEST(TestMessageIdEdgeCase, throwsOnNullCharacter)
{
    std::string msg_id_with_null = "<abc";
    msg_id_with_null += '\0';
    msg_id_with_null += "def>";
    EXPECT_THROW(MessageId{msg_id_with_null}, std::invalid_argument);
}
