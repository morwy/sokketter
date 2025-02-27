#include <gtest/gtest.h>

using namespace testing;

TEST(HelloTest, BasicAssertions)
{
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}
