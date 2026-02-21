#include <nntp/ArticleRange.h>

#include <gtest/gtest.h>

using namespace nntp;

TEST(TestArticleRangeConstruct, defaultConstructor)
{
    EXPECT_NO_THROW(ArticleRange());
}

TEST(TestArticleRangeConstruct, unboundedFromArticle)
{
    EXPECT_NO_THROW(ArticleRange(Article(42)));
}

TEST(TestArticleRangeConstruct, boundedFromTwoArticles)
{
    EXPECT_NO_THROW(ArticleRange(Article(1), Article(100)));
}

TEST(TestArticleRangeConstruct, boundedThrowsWhenBeginGreaterThanEnd)
{
    EXPECT_THROW(ArticleRange(Article(100), Article(50)), std::invalid_argument);
}

TEST(TestArticleRangeQuery, defaultIsBounded)
{
    ArticleRange range;

    EXPECT_TRUE(range.is_bounded());
}

TEST(TestArticleRangeQuery, unboundedIsNotBounded)
{
    ArticleRange range(Article(42));

    EXPECT_FALSE(range.is_bounded());
}

TEST(TestArticleRangeQuery, boundedIsBounded)
{
    ArticleRange range(Article(1), Article(100));

    EXPECT_TRUE(range.is_bounded());
}

TEST(TestArticleRangeQuery, defaultBeginIsOne)
{
    ArticleRange range;

    EXPECT_EQ(range.begin().value(), 1);
}

TEST(TestArticleRangeQuery, defaultEndIsOne)
{
    ArticleRange range;

    EXPECT_EQ(range.end().value(), 1);
}

TEST(TestArticleRangeQuery, boundedBeginReturnsStart)
{
    ArticleRange range(Article(10), Article(50));

    EXPECT_EQ(range.begin().value(), 10);
}

TEST(TestArticleRangeQuery, boundedEndReturnsEnd)
{
    ArticleRange range(Article(10), Article(50));

    EXPECT_EQ(range.end().value(), 50);
}

TEST(TestArticleRangeParse, boundedRangeValid)
{
    auto result = ArticleRange::parse("1-100");

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->is_bounded());
    EXPECT_EQ(result->begin().value(), 1);
    EXPECT_EQ(result->end().value(), 100);
}

TEST(TestArticleRangeParse, unboundedRangeValid)
{
    auto result = ArticleRange::parse("500-");

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->is_bounded());
    EXPECT_EQ(result->begin().value(), 500);
}

TEST(TestArticleRangeParse, singleElementRangeValid)
{
    auto result = ArticleRange::parse("42-42");

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->is_bounded());
    EXPECT_EQ(result->begin().value(), 42);
    EXPECT_EQ(result->end().value(), 42);
}

TEST(TestArticleRangeParse, largeNumbersValid)
{
    auto result = ArticleRange::parse("999999-1000000");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin().value(), 999999);
    EXPECT_EQ(result->end().value(), 1000000);
}

TEST(TestArticleRangeParse, emptyStringInvalid)
{
    auto result = ArticleRange::parse("");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, missingHyphenInvalid)
{
    auto result = ArticleRange::parse("100");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, invalidStartNumberInvalid)
{
    auto result = ArticleRange::parse("abc-100");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, invalidEndNumberInvalid)
{
    auto result = ArticleRange::parse("1-xyz");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, zeroStartInvalid)
{
    auto result = ArticleRange::parse("0-100");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, beginGreaterThanEndInvalid)
{
    auto result = ArticleRange::parse("100-50");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeParse, whitespaceInvalid)
{
    auto result = ArticleRange::parse("1 - 100");

    EXPECT_FALSE(result);
}

TEST(TestArticleRangeComparison, defaultRangesEqual)
{
    ArticleRange range1;
    ArticleRange range2;

    EXPECT_EQ(range1, range2);
}

TEST(TestArticleRangeComparison, boundedRangesEqual)
{
    ArticleRange range1(Article(1), Article(100));
    ArticleRange range2(Article(1), Article(100));

    EXPECT_EQ(range1, range2);
}

TEST(TestArticleRangeComparison, unboundedRangesEqual)
{
    ArticleRange range1(Article(50));
    ArticleRange range2(Article(50));

    EXPECT_EQ(range1, range2);
}

TEST(TestArticleRangeComparison, differentBeginNotEqual)
{
    ArticleRange range1(Article(1), Article(100));
    ArticleRange range2(Article(2), Article(100));

    EXPECT_NE(range1, range2);
}

TEST(TestArticleRangeComparison, differentEndNotEqual)
{
    ArticleRange range1(Article(1), Article(100));
    ArticleRange range2(Article(1), Article(99));

    EXPECT_NE(range1, range2);
}

TEST(TestArticleRangeComparison, boundedVsUnboundedNotEqual)
{
    ArticleRange bounded(Article(1), Article(100));
    ArticleRange unbounded(Article(1));

    EXPECT_NE(bounded, unbounded);
}

TEST(TestArticleRangeComparison, defaultVsBoundedNotEqual)
{
    ArticleRange defaultRange;
    ArticleRange bounded(Article(1), Article(2));

    EXPECT_NE(defaultRange, bounded);
}

TEST(TestArticleRangeConversion, boundedToString)
{
    ArticleRange range(Article(1), Article(100));

    EXPECT_EQ(to_string(range), "1-100");
}

TEST(TestArticleRangeConversion, unboundedToString)
{
    ArticleRange range(Article(500));

    EXPECT_EQ(to_string(range), "500-");
}

TEST(TestArticleRangeConversion, defaultToString)
{
    ArticleRange range;

    EXPECT_EQ(to_string(range), "1-1");
}

TEST(TestArticleRangeConversion, singleElementToString)
{
    ArticleRange range(Article(42), Article(42));

    EXPECT_EQ(to_string(range), "42-42");
}

TEST(TestArticleRangeRoundTrip, boundedRangeParseAndConvert)
{
    std::string original = "42-100";
    auto parsed = ArticleRange::parse(original);

    ASSERT_TRUE(parsed);
    EXPECT_EQ(to_string(*parsed), original);
}

TEST(TestArticleRangeRoundTrip, unboundedRangeParseAndConvert)
{
    std::string original = "500-";
    auto parsed = ArticleRange::parse(original);

    ASSERT_TRUE(parsed);
    EXPECT_EQ(to_string(*parsed), original);
}

TEST(TestArticleRangeRoundTrip, constructorAndParseEquivalent)
{
    ArticleRange constructed(Article(10), Article(50));
    auto parsed = ArticleRange::parse("10-50");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(constructed, *parsed);
}
