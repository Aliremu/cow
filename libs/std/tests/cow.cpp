#include <gtest/gtest.h>
#include <memory>

#include "cow.h"

struct MockStruct
{
    int value;
};

TEST(Cow, Construct)
{

    auto a = cow<MockStruct>(1);
    auto b = a;

    EXPECT_EQ(a._data.get(), b._data.get());
    b->value = 10;
    EXPECT_EQ(a->value, 1);
    EXPECT_EQ(b->value, 10);
}

TEST(Cow, ConstructFromSharedPtr)
{
    auto a = std::make_shared<MockStruct>(1);
    {
        auto b = cow{std::move(a)};
        EXPECT_EQ(a.use_count(), 1);
        EXPECT_EQ(b->value, 1);
        b->value = 10;
        EXPECT_EQ(b->value, 1);
    }
    EXPECT_EQ(a.use_count(), 0);
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
