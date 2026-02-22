#include <nntp/Newsgroup.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using namespace nntp;
using namespace testing;

namespace
{

class GroupConstructValid : public TestWithParam<std::string_view>
{
};

class GroupConstructInvalid : public TestWithParam<std::string_view>
{
};

} // namespace

TEST_P(GroupConstructValid, newsgroup)
{
    EXPECT_NO_THROW(Newsgroup{GetParam()});
}

INSTANTIATE_TEST_SUITE_P(TestNewsgroupWithParam, GroupConstructValid,
    Values("comp.lang.c",     //
        "a",                  //
        "comp.lang.c++",      //
        "a.b.c",              //
        "comp.lang.\xC3\xA9", //
        "comp.\xE2\x82\xAC.news"));

TEST(TestNewsgroupConstruct, allowedAsciiCharacters)
{
    for (char c = 0x22; c <= 0x7E; ++c)
    {
        if (c == '*' || c == ',' || c == '?' || c == '[' || c == '\\' || c == ']')
        {
            continue;
        }

        std::string group = "a" + std::string(1, c) + "b";

        EXPECT_NO_THROW(Newsgroup{group}) << "Failed for character: " << static_cast<int>(c);
    }
}

TEST_P(GroupConstructInvalid, newsgroup)
{
    const std::string_view value = GetParam();

    EXPECT_THROW(Newsgroup{value}, std::invalid_argument);
}

static std::string_view s_invalid_newsgroups[]{
    "",                 //
    "comp lang.c",      //
    " comp.lang.c",     //
    "comp.lang.c ",     //
    "comp\x01.lang",    //
    "comp\tlang",       //
    "comp\nlang",       //
    "comp\x7Flang",     //
    "comp!lang",        //
    "comp*lang",        //
    "comp,lang",        //
    "comp?lang",        //
    "comp[lang",        //
    "comp\\lang",       //
    "comp]lang",        //
    "\xC0\xAF",         // invalid UTF-8
    "\xE2\x28\xA1",     // invalid UTF-8
    "\xF0\x9F",         // invalid UTF-8
    {"comp\x00lang", 9} // embedded NUL
};

INSTANTIATE_TEST_SUITE_P(TestNewsgroupInvalidWithParam, GroupConstructInvalid, ValuesIn(s_invalid_newsgroups));

TEST(TestNewsgroupValue, returnsCorrectValue)
{
    Newsgroup group("comp.lang.c");

    EXPECT_EQ(group.value(), "comp.lang.c");
}

TEST(TestNewsgroupCompare, sameValue)
{
    Newsgroup group1("comp.lang.c");
    Newsgroup group2("comp.lang.c");

    EXPECT_EQ(group1, group2);
}

TEST(TestNewsgroupCompare, differentValue)
{
    Newsgroup group1("comp.lang.c");
    Newsgroup group2("comp.lang.cpp");

    EXPECT_NE(group1, group2);
}

TEST(TestNewsgroupOrdering, lessThan)
{
    Newsgroup group1("a.group");
    Newsgroup group2("b.group");

    EXPECT_LT(group1, group2);
}

TEST(TestNewsgroupOrdering, lessThanOrEqual)
{
    Newsgroup group1("a.group");
    Newsgroup group2("b.group");
    Newsgroup group3("a.group");

    EXPECT_LE(group1, group2);
    EXPECT_LE(group1, group3);
}

TEST(TestNewsgroupOrdering, greaterThan)
{
    Newsgroup group1("b.group");
    Newsgroup group2("a.group");

    EXPECT_GT(group1, group2);
}

TEST(TestNewsgroupOrdering, greaterThanOrEqual)
{
    Newsgroup group1("b.group");
    Newsgroup group2("a.group");
    Newsgroup group3("b.group");

    EXPECT_GE(group1, group2);
    EXPECT_GE(group1, group3);
}
