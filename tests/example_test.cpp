#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <span>

#include "vk_engine.h"

// Simple test to verify test framework is working
TEST(VulkanEngineTest, BasicSetupTest) {
    // This just tests that we can compile and run tests
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
}