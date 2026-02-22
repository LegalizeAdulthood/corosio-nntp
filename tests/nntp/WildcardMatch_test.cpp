#include <nntp/WildcardMatch.h>

#include <nntp/Newsgroup.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using namespace nntp;
using namespace testing;

namespace
{

class ConstructValidWildMat : public TestWithParam<std::string_view>
{
};

class ConstructInvalidWildMat : public TestWithParam<std::string_view>
{
};

} // namespace

TEST_P(ConstructValidWildMat, pattern)
{
    WildcardMatch wildmat(GetParam());

    EXPECT_EQ(wildmat.value(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(TestWildcardMatchConstructWithParam, ConstructValidWildMat,
    Values("comp.lang.*",         //
        "comp.lang.*,sci.math.*", //
        "a*,!*b",                 //
        "a*,!*b,*c*"));           //

TEST(TestWildcardMatchConstruct, defaultConstructor)
{
    WildcardMatch wildmat;

    EXPECT_EQ(wildmat.value(), "*");
}

TEST_P(ConstructInvalidWildMat, pattern)
{
    EXPECT_THROW(WildcardMatch{GetParam()}, std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(TestWildcardMatchConstructWithParam, ConstructInvalidWildMat,
    Values("",                // empty string
        "comp.*,",            // empty pattern after comma
        "comp.[lang",         // invalid pattern
        "comp.*,sci.[math")); // invalid pattern after comma

TEST(TestWildcardMatchValue, returnsCorrectValue)
{
    WildcardMatch wildmat("a*,!*b");

    EXPECT_EQ(wildmat.value(), "a*,!*b");
}

TEST(TestWildcardMatchCompare, sameValue)
{
    WildcardMatch wildmat1("a*,!*b");
    WildcardMatch wildmat2("a*,!*b");

    EXPECT_EQ(wildmat1, wildmat2);
}

TEST(TestWildcardMatchCompare, differentValue)
{
    WildcardMatch wildmat1("a*,!*b");
    WildcardMatch wildmat2("a*,*b");

    EXPECT_NE(wildmat1, wildmat2);
}

TEST(TestWildcardMatchMatchingSemantics, rightmostPositivePatternMatches)
{
    WildcardMatch wildmat("a*,!*b");
    Newsgroup group("aaa");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rightmostNegativePatternMatches)
{
    WildcardMatch wildmat("a*,!*b");
    Newsgroup group("abb");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rightmostPatternDecides)
{
    WildcardMatch wildmat("a*,c*,!*b");
    Newsgroup group("ccb");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, noPatternMatches)
{
    WildcardMatch wildmat("a*,!*b");
    Newsgroup group("xxx");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, singlePositivePatternMatches)
{
    WildcardMatch wildmat("comp.lang.*");
    Newsgroup group("comp.lang.c");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, singlePositivePatternNoMatch)
{
    WildcardMatch wildmat("comp.lang.*");
    Newsgroup group("sci.math.topology");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, singleNegativePatternNoMatch)
{
    WildcardMatch wildmat("!*bot");
    Newsgroup group("sci.robot");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, multiplePositivePatternsFirstMatches)
{
    WildcardMatch wildmat("comp.*,sci.*");
    Newsgroup group("comp.lang.c");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, multiplePositivePatternsLastMatches)
{
    WildcardMatch wildmat("comp.*,sci.*");
    Newsgroup group("sci.math.topology");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, multiplePositivePatternsNoneMatch)
{
    WildcardMatch wildmat("comp.*,sci.*");
    Newsgroup group("rec.arts.books");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rfc3977ExampleAaa)
{
    WildcardMatch wildmat("a*,!*b,*c*");
    Newsgroup group("aaa");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rfc3977ExampleAbb)
{
    WildcardMatch wildmat("a*,!*b,*c*");
    Newsgroup group("abb");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rfc3977ExampleCcb)
{
    WildcardMatch wildmat("a*,!*b,*c*");
    Newsgroup group("ccb");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, rfc3977ExampleXxx)
{
    WildcardMatch wildmat("a*,!*b,*c*");
    Newsgroup group("xxx");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, negationAfterAsterisk)
{
    WildcardMatch wildmat("*,!*test");
    Newsgroup group("mytest");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, negationAfterAsteriskNoMatch)
{
    WildcardMatch wildmat("*,!*test");
    Newsgroup group("mygroup");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, defaultMatchesAnything)
{
    WildcardMatch wildmat;
    Newsgroup group("any.group.name");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, utf8InPatterns)
{
    WildcardMatch wildmat("comp.*,!*\xC3\xA9");
    Newsgroup group("comp.lang.c");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, utf8InPatternMatched)
{
    WildcardMatch wildmat("comp.*,!*\xC3\xA9");
    Newsgroup group("comp.\xC3\xA9");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, complexNegativePattern)
{
    WildcardMatch wildmat("comp.*,!comp.test.*");
    Newsgroup group("comp.lang.c");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, complexNegativePatternMatched)
{
    WildcardMatch wildmat("comp.*,!comp.test.*");
    Newsgroup group("comp.test.java");

    EXPECT_FALSE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, alternatingNegations)
{
    WildcardMatch wildmat("a*,!ab*,abc*");
    Newsgroup group("abc");

    EXPECT_TRUE(wildmat.match(group));
}

TEST(TestWildcardMatchMatchingSemantics, alternatingNegationsAbNoMatch)
{
    WildcardMatch wildmat("a*,!ab*,abc*");
    Newsgroup group("ab");

    EXPECT_FALSE(wildmat.match(group));
}
