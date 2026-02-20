#include <nntp/Article.h>

#include <gtest/gtest.h>

using namespace nntp;

TEST(TestArticle, constuctFromZeroThrowsException)
{
    EXPECT_THROW(Article{0}, std::invalid_argument);
}

TEST(TestArticle, defaultConstructIsArticleNumberOne)
{
    Article art;

    EXPECT_EQ(1U, art.value());
}
