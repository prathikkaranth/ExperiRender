#include <RenderConfig.h>
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

constexpr float EPSILON = 1e-6f;

TEST(DisplayTests, AspectRatioCalculation) {
    constexpr float aspectRatio = RenderConfig::getDefaultAspectRatio();
    float expectedRatio = static_cast<float>(DEFAULT_WINDOW_WIDTH) / static_cast<float>(DEFAULT_WINDOW_HEIGHT);

    EXPECT_NEAR(aspectRatio, expectedRatio, EPSILON);
    EXPECT_NEAR(aspectRatio, 1600.0f / 900.0f, EPSILON);
    EXPECT_NEAR(aspectRatio, 16.0f / 9.0f, EPSILON);
}

TEST(DisplayTests, AspectRatioIsValid) {
    constexpr float aspectRatio = RenderConfig::getDefaultAspectRatio();

    // Aspect ratio should be positive and reasonable for displays
    EXPECT_GT(aspectRatio, 0.0f);
    EXPECT_LT(aspectRatio, 10.0f);
    EXPECT_GT(aspectRatio, 0.1f);

    EXPECT_FALSE(std::isnan(aspectRatio));
    EXPECT_FALSE(std::isinf(aspectRatio));
}

TEST(DisplayTests, CommonAspectRatios) {
    constexpr float aspectRatio = RenderConfig::getDefaultAspectRatio();

    // Test that the default is a common 16:9 aspect ratio
    EXPECT_NEAR(aspectRatio, 16.0f / 9.0f, EPSILON);

    // Verify it's not other common ratios
    EXPECT_NE(aspectRatio, 4.0f / 3.0f);
    EXPECT_NE(aspectRatio, 3.0f / 2.0f);
    EXPECT_NE(aspectRatio, 21.0f / 9.0f);
}

TEST(DisplayTests, ResolutionValidation) {
    VkExtent2D extent = RenderConfig::getDefaultWindowExtent();

    EXPECT_GT(extent.width, 0u);
    EXPECT_GT(extent.height, 0u);

    EXPECT_GE(extent.width, 640u);
    EXPECT_GE(extent.height, 480u);
    EXPECT_LE(extent.width, 7680u);
    EXPECT_LE(extent.height, 4320u);
}

TEST(DisplayTests, DimensionsAreDivisible) {
    VkExtent2D extent = RenderConfig::getDefaultWindowExtent();

    // Test for divisibility
    EXPECT_EQ(extent.width % 8, 0u);
    EXPECT_EQ(extent.height % 4, 0u);

    EXPECT_EQ(1600u % 8u, 0u);
    EXPECT_EQ(900u % 4u, 0u);
}

TEST(DisplayTests, FOVAndPlanesValidation) {
    // Test the other constants are reasonable
    EXPECT_GT(DEFAULT_FOV_DEGREES, 30.0f);
    EXPECT_LT(DEFAULT_FOV_DEGREES, 120.0f);

    EXPECT_GT(NEAR_PLANE, 0.0f);
    EXPECT_LT(NEAR_PLANE, 1.0f);

    EXPECT_GT(FAR_PLANE, NEAR_PLANE);
    EXPECT_GT(FAR_PLANE, 100.0f);
}