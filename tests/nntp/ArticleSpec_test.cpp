#include <nntp/ArticleSpec.h>

#include <gtest/gtest.h>

using namespace nntp;

TEST(TestArticleSpecConstruct, fromArticle)
{
    Article article(42);

    EXPECT_NO_THROW(ArticleSpec{article});
}

TEST(TestArticleSpecConstruct, fromMessageId)
{
    MessageId msg_id("<abc@example.com>");

    EXPECT_NO_THROW(ArticleSpec{msg_id});
}

TEST(TestArticleSpecType, isArticleReturnsTrueForArticle)
{
    ArticleSpec spec{Article(42)};

    EXPECT_TRUE(spec.is_article());
    EXPECT_FALSE(spec.is_message_id());
}

TEST(TestArticleSpecType, isMessageIdReturnsTrueForMessageId)
{
    ArticleSpec spec{MessageId("<abc@example.com>")};

    EXPECT_FALSE(spec.is_article());
    EXPECT_TRUE(spec.is_message_id());
}

TEST(TestArticleSpecAccess, asArticleReturnsArticle)
{
    Article article(42);
    ArticleSpec spec{article};

    EXPECT_EQ(spec.as_article(), article);
}

TEST(TestArticleSpecAccess, asMessageIdReturnsMessageId)
{
    MessageId msg_id("<abc@example.com>");
    ArticleSpec spec{msg_id};

    EXPECT_EQ(spec.as_message_id(), msg_id);
}

TEST(TestArticleSpecAccess, asArticleThrowsForMessageId)
{
    ArticleSpec spec{MessageId("<abc@example.com>")};

    EXPECT_THROW(spec.as_article(), std::bad_variant_access);
}

TEST(TestArticleSpecAccess, asMessageIdThrowsForArticle)
{
    ArticleSpec spec{Article(42)};

    EXPECT_THROW(spec.as_message_id(), std::bad_variant_access);
}

TEST(TestArticleSpecValue, articleValueReturnsNumber)
{
    ArticleSpec spec{Article(42)};

    EXPECT_EQ(spec.value(), "42");
}

TEST(TestArticleSpecValue, messageIdValueReturnsMessageId)
{
    ArticleSpec spec{MessageId("<abc@example.com>")};

    EXPECT_EQ(spec.value(), "<abc@example.com>");
}

TEST(TestArticleSpecValue, largeArticleNumber)
{
    ArticleSpec spec{Article(9999999999)};

    EXPECT_EQ(spec.value(), "9999999999");
}

TEST(TestArticleSpecCompare, equalArticles)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{Article(42)};

    EXPECT_EQ(spec1, spec2);
}

TEST(TestArticleSpecCompare, unequalArticles)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{Article(100)};

    EXPECT_NE(spec1, spec2);
}

TEST(TestArticleSpecCompare, equalMessageIds)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};
    ArticleSpec spec2{MessageId("<abc@example.com>")};

    EXPECT_EQ(spec1, spec2);
}

TEST(TestArticleSpecCompare, unequalMessageIds)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};
    ArticleSpec spec2{MessageId("<xyz@example.com>")};

    EXPECT_NE(spec1, spec2);
}

TEST(TestArticleSpecCompare, articleNotEqualMessageId)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{MessageId("<abc@example.com>")};

    EXPECT_NE(spec1, spec2);
}

TEST(TestArticleSpecCompare, articleLessThanMessageId)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{MessageId("<abc@example.com>")};

    EXPECT_LT(spec1, spec2);
}

TEST(TestArticleSpecCompare, messageIdGreaterThanArticle)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};
    ArticleSpec spec2{Article(42)};

    EXPECT_GT(spec1, spec2);
}

TEST(TestArticleSpecOrdering, articlesInOrder)
{
    ArticleSpec spec1{Article(10)};
    ArticleSpec spec2{Article(20)};
    ArticleSpec spec3{Article(30)};

    EXPECT_LT(spec1, spec2);
    EXPECT_LT(spec2, spec3);
    EXPECT_LE(spec1, spec2);
    EXPECT_LE(spec1, spec1);
    EXPECT_GT(spec3, spec2);
    EXPECT_GT(spec2, spec1);
    EXPECT_GE(spec3, spec2);
    EXPECT_GE(spec1, spec1);
}

TEST(TestArticleSpecOrdering, messageIdsInOrder)
{
    ArticleSpec spec1{MessageId("<aaa@example.com>")};
    ArticleSpec spec2{MessageId("<bbb@example.com>")};
    ArticleSpec spec3{MessageId("<ccc@example.com>")};

    EXPECT_LT(spec1, spec2);
    EXPECT_LT(spec2, spec3);
    EXPECT_LE(spec1, spec2);
    EXPECT_LE(spec1, spec1);
    EXPECT_GT(spec3, spec2);
    EXPECT_GT(spec2, spec1);
    EXPECT_GE(spec3, spec2);
    EXPECT_GE(spec1, spec1);
}

TEST(TestArticleSpecCopy, copyArticle)
{
    ArticleSpec spec1{Article(42)};

    ArticleSpec spec2{spec1};

    EXPECT_EQ(spec1, spec2);
    EXPECT_TRUE(spec2.is_article());
    EXPECT_EQ(spec2.as_article(), Article(42));
}

TEST(TestArticleSpecCopy, copyMessageId)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};

    ArticleSpec spec2{spec1};

    EXPECT_EQ(spec1, spec2);
    EXPECT_TRUE(spec2.is_message_id());
    EXPECT_EQ(spec2.as_message_id(), MessageId("<abc@example.com>"));
}

TEST(TestArticleSpecCopy, copyAssignmentArticle)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{Article(100)};

    spec2 = spec1;

    EXPECT_EQ(spec1, spec2);
    EXPECT_EQ(spec2.as_article(), Article(42));
}

TEST(TestArticleSpecCopy, copyAssignmentMessageId)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};
    ArticleSpec spec2{MessageId("<xyz@example.com>")};

    spec2 = spec1;

    EXPECT_EQ(spec1, spec2);
    EXPECT_EQ(spec2.as_message_id(), MessageId("<abc@example.com>"));
}

TEST(TestArticleSpecCopy, copyAssignmentDifferentTypes)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{MessageId("<abc@example.com>")};

    spec2 = spec1;

    EXPECT_EQ(spec1, spec2);
    EXPECT_TRUE(spec2.is_article());
    EXPECT_EQ(spec2.as_article(), Article(42));
}

TEST(TestArticleSpecMove, moveArticle)
{
    ArticleSpec spec1{Article(42)};

    ArticleSpec spec2{std::move(spec1)};

    EXPECT_TRUE(spec2.is_article());
    EXPECT_EQ(spec2.as_article(), Article(42));
}

TEST(TestArticleSpecMove, moveMessageId)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};

    ArticleSpec spec2{std::move(spec1)};

    EXPECT_TRUE(spec2.is_message_id());
    EXPECT_EQ(spec2.as_message_id(), MessageId("<abc@example.com>"));
}

TEST(TestArticleSpecMove, moveAssignmentArticle)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{Article(100)};

    spec2 = std::move(spec1);

    EXPECT_TRUE(spec2.is_article());
    EXPECT_EQ(spec2.as_article(), Article(42));
}

TEST(TestArticleSpecMove, moveAssignmentMessageId)
{
    ArticleSpec spec1{MessageId("<abc@example.com>")};
    ArticleSpec spec2{MessageId("<xyz@example.com>")};

    spec2 = std::move(spec1);

    EXPECT_TRUE(spec2.is_message_id());
    EXPECT_EQ(spec2.as_message_id(), MessageId("<abc@example.com>"));
}

TEST(TestArticleSpecMove, moveAssignmentDifferentTypes)
{
    ArticleSpec spec1{Article(42)};
    ArticleSpec spec2{MessageId("<abc@example.com>")};

    spec2 = std::move(spec1);

    EXPECT_TRUE(spec2.is_article());
    EXPECT_EQ(spec2.as_article(), Article(42));
}
