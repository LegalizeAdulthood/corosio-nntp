#include <nntp/WildcardMatchPattern.h>

#include <nntp/Newsgroup.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using namespace nntp;
using namespace testing;

namespace
{

class PatternConstructValid : public TestWithParam<std::string_view>
{
};

class PatternConstructInvalid : public TestWithParam<std::string_view>
{
};

class PatternMatchExact : public TestWithParam<std::string_view>
{
};

class PatternMatchFail : public TestWithParam<std::pair<std::string_view, std::string_view>>
{
};

} // namespace

TEST_P(PatternConstructValid, pattern)
{
    EXPECT_NO_THROW(WildcardMatchPattern{GetParam()});
}

INSTANTIATE_TEST_SUITE_P(TestWildcardMatchPatternValidWithParam, PatternConstructValid,
    Values("*",                 //
        "?",                    //
        "comp.lang.c",          //
        "comp.lang.*",          //
        "comp.*.c",             //
        "?omp.lang.c",          //
        "*b",                   //
        "*a*",                  //
        "a*b*c",                //
        "comp.lang.\xC3\xA9")); // UTF-8 pattern

TEST(TestWildcardMatchPatternConstruct, defaultConstructor)
{
    WildcardMatchPattern pattern;

    EXPECT_EQ(pattern.value(), "*");
}

TEST_P(PatternConstructInvalid, pattern)
{
    const std::string_view value = GetParam();

    EXPECT_THROW(WildcardMatchPattern{value}, std::invalid_argument);
}

static std::string_view s_invalid_patterns[]{
    "",             // empty
    "comp,lang",    // comma not allowed in single pattern
    "comp[lang",    // bracket not allowed
    "comp]lang",    // bracket not allowed
    "comp\\lang",   // backslash not allowed
    "comp!lang",    // exclamation not allowed at start
    "comp lang",    // space not allowed
    "comp\x01lang", // control character
    "\xC0\xAF"      // invalid UTF-8
};

INSTANTIATE_TEST_SUITE_P(
    TestWildcardMatchPatternInvalidWithParam, PatternConstructInvalid, ValuesIn(s_invalid_patterns));

TEST(TestWildcardMatchPatternValue, returnsCorrectValue)
{
    WildcardMatchPattern pattern("comp.lang.*");

    EXPECT_EQ(pattern.value(), "comp.lang.*");
}

TEST(TestWildcardMatchPatternCompare, sameValue)
{
    WildcardMatchPattern pattern1("comp.lang.*");
    WildcardMatchPattern pattern2("comp.lang.*");

    EXPECT_EQ(pattern1, pattern2);
}

TEST(TestWildcardMatchPatternCompare, differentValue)
{
    WildcardMatchPattern pattern1("comp.lang.*");
    WildcardMatchPattern pattern2("comp.lang.?");

    EXPECT_NE(pattern1, pattern2);
}

TEST_P(PatternMatchExact, pattern)
{
    WildcardMatchPattern pattern{GetParam()};
    Newsgroup group{GetParam()};

    EXPECT_TRUE(pattern.match(group));
}

INSTANTIATE_TEST_SUITE_P(TestWildcardMatchPatternExactMatch, PatternMatchExact,
    Values("abc",               //
        "comp.lang.c",          //
        "a",                    //
        "comp.lang.\xC3\xA9")); // UTF-8

TEST_P(PatternMatchFail, pattern)
{
    const auto [pattern_str, newsgroup_str] = GetParam();
    WildcardMatchPattern pattern{pattern_str};
    Newsgroup group{newsgroup_str};

    EXPECT_FALSE(pattern.match(group));
}

INSTANTIATE_TEST_SUITE_P(TestWildcardMatchPatternNoMatch, PatternMatchFail,
    Values(std::make_pair("abc", "abd"),                //
        std::make_pair("comp.lang.c", "comp.lang.cpp"), //
        std::make_pair("abc", "xyzabc"),                //
        std::make_pair("a", "aa")));                    //

TEST(TestWildcardMatchPatternWildcard, asteriskZeroCharacters)
{
    WildcardMatchPattern pattern("a*b");
    Newsgroup group("ab");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, asteriskMultipleCharacters)
{
    WildcardMatchPattern pattern("a*b");
    Newsgroup group("axxxb");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, asteriskAtStart)
{
    WildcardMatchPattern pattern("*abc");
    Newsgroup group("xyzabc");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, asteriskAtEnd)
{
    WildcardMatchPattern pattern("abc*");
    Newsgroup group("abcxyz");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, questionMarkSingleCharacter)
{
    WildcardMatchPattern pattern("a?c");
    Newsgroup group("abc");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, questionMarkMultiple)
{
    WildcardMatchPattern pattern("??a*");
    Newsgroup group("xya");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, questionMarkDoesNotMatchZero)
{
    WildcardMatchPattern pattern("a?c");
    Newsgroup group("ac");

    EXPECT_FALSE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, asteriskDoesNotMatchZeroInMiddle)
{
    WildcardMatchPattern pattern("a*b");
    Newsgroup group("acb");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, defaultAsteriskMatchesAnything)
{
    WildcardMatchPattern pattern;
    Newsgroup group("comp.lang.c");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternWildcard, defaultAsteriskMatchesEmpty)
{
    WildcardMatchPattern pattern;
    Newsgroup group("a");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternUTF8, asteriskWithUTF8Newsgroup)
{
    WildcardMatchPattern pattern("comp.*");
    Newsgroup group("comp.\xC3\xA9");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternUTF8, questionMarkWithUTF8Character)
{
    WildcardMatchPattern pattern("a?z");
    Newsgroup group("a\xC3\xA9z");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternUTF8, exactMatchWithUTF8)
{
    WildcardMatchPattern pattern("test.\xC3\xA9");
    Newsgroup group("test.\xC3\xA9");

    EXPECT_TRUE(pattern.match(group));
}

TEST(TestWildcardMatchPatternUTF8, mismatchWithDifferentUTF8)
{
    WildcardMatchPattern pattern("test.\xC3\xA9");
    Newsgroup group("test.\xE2\x82\xAC");

    EXPECT_FALSE(pattern.match(group));
}
