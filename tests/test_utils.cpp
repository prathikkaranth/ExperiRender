#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vk_utils.h>

TEST(UtilTests, AlignUp) {
    // Basic alignment tests
    EXPECT_EQ(align_up(1, 4), 4);
    EXPECT_EQ(align_up(4, 4), 4);
    EXPECT_EQ(align_up(5, 8), 8);
    EXPECT_EQ(align_up(7, 8), 8);
    EXPECT_EQ(align_up(8, 8), 8);

    // Edge case: zero
    EXPECT_EQ(align_up(0, 4), 0);
    EXPECT_EQ(align_up(0, 16), 0);

    // Power of 2 alignments
    EXPECT_EQ(align_up(1, 1), 1);
    EXPECT_EQ(align_up(1, 2), 2);
    EXPECT_EQ(align_up(3, 2), 4);
    EXPECT_EQ(align_up(15, 16), 16);
    EXPECT_EQ(align_up(16, 16), 16);
    EXPECT_EQ(align_up(17, 16), 32);
    EXPECT_EQ(align_up(31, 32), 32);
    EXPECT_EQ(align_up(33, 32), 64);

    // Large values
    EXPECT_EQ(align_up(1000, 256), 1024);
    EXPECT_EQ(align_up(1024, 256), 1024);
    EXPECT_EQ(align_up(1025, 256), 1280);

    // Different integral types
    EXPECT_EQ(align_up(static_cast<uint32_t>(5), 4), 8u);
    EXPECT_EQ(align_up(static_cast<uint64_t>(9), 8), 16ull);
    EXPECT_EQ(align_up(static_cast<size_t>(3), 4), static_cast<size_t>(4));
}