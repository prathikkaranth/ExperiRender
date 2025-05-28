#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <vector>
#include "ssao.h"

TEST(SSAOTest, KernelSizeTest) {
    ssao ssaoCaller;
    ssaoCaller.ssaoData.kernelSize = 64; // Set kernel size
    auto kernel = ssaoCaller.generate_ssao_kernels();
    EXPECT_EQ(kernel.size(), 64);
}

TEST(SSAOTest, KernelZPositive) {
    ssao ssaoCaller;
    ssaoCaller.ssaoData.kernelSize = 64;
    auto kernel = ssaoCaller.generate_ssao_kernels();
    for (const auto& sample : kernel) {
        EXPECT_GE(sample.z, 0.0f) << "Sample z < 0: " << sample.z;
    }
}

TEST(SSAOTest, KernelLengthWithinBounds) {
    ssao ssaoCaller;
    ssaoCaller.ssaoData.kernelSize = 64;
    auto kernel = ssaoCaller.generate_ssao_kernels();
    for (const auto& sample : kernel) {
        float length = glm::length(sample);
        EXPECT_LE(length, 1.0f) << "Sample length > 1: " << length;
    }
}

TEST(SSAOTest, ReproducibilityTest) {
    ssao ssaoCaller;
    ssaoCaller.ssaoData.kernelSize = 64;
    auto kernel1 = ssaoCaller.generate_ssao_kernels();
    auto kernel2 = ssaoCaller.generate_ssao_kernels();
    ASSERT_EQ(kernel1.size(), kernel2.size());

    for (size_t i = 0; i < kernel1.size(); ++i) {
        EXPECT_FLOAT_EQ(kernel1[i].x, kernel2[i].x);
        EXPECT_FLOAT_EQ(kernel1[i].y, kernel2[i].y);
        EXPECT_FLOAT_EQ(kernel1[i].z, kernel2[i].z);
    }
}