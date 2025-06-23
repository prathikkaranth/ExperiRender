#include <glm/glm.hpp>
#include<gtest/gtest.h>
#include <vector>
#include "ssao.h"

class SSAOTest : public ::testing::Test {
public:
SSAOTest() = default;

~SSAOTest() = default;

    void SetUp() override { ssaoCaller.ssaoData.kernelSize = 64; }

    void TearDown() override {}

protected:
    ssao ssaoCaller;
};

TEST_F(SSAOTest, SsaoLerp) {
    float a = 0.0f;
    float b = 1.0f;
    float f = 0.5f;
    float result = ssaoCaller.ssaoLerp(a, b, f);
    EXPECT_FLOAT_EQ(result, 0.5f) << "Lerp result is not as expected: " << result;
}

TEST_F(SSAOTest, KernelSizeTest) {
    auto kernel = ssaoCaller.generate_ssao_kernels();
    EXPECT_EQ(kernel.size(), 64);
}

TEST_F(SSAOTest, KernelZPositive) {
    auto kernel = ssaoCaller.generate_ssao_kernels();
    for (const auto &sample: kernel) {
        EXPECT_GE(sample.z, 0.0f) << "Sample z < 0: " << sample.z;
    }
}

TEST_F(SSAOTest, KernelLengthWithinBounds) {
    auto kernel = ssaoCaller.generate_ssao_kernels();
    for (const auto &sample: kernel) {
        float length = glm::length(sample);
        EXPECT_LE(length, 1.0f) << "Sample length > 1: " << length;
    }
}

TEST_F(SSAOTest, ReproducibilityTest) {
    auto kernel1 = ssaoCaller.generate_ssao_kernels();
    auto kernel2 = ssaoCaller.generate_ssao_kernels();
    ASSERT_EQ(kernel1.size(), kernel2.size());
    for (size_t i = 0; i < kernel1.size(); ++i) {
        EXPECT_FLOAT_EQ(kernel1[i].x, kernel2[i].x);
        EXPECT_FLOAT_EQ(kernel1[i].y, kernel2[i].y);
        EXPECT_FLOAT_EQ(kernel1[i].z, kernel2[i].z);
    }
}

TEST_F(SSAOTest, TestKernelValues) {
    auto kernel = ssaoCaller.generate_ssao_kernels();

    constexpr std::array<glm::vec3, 64> expectedKernel = {
        glm::vec3(0.0144768f, 0.00952489f, 0.00601507f),      glm::vec3(0.0125274f, 0.0355144f, 0.0464733f),
        glm::vec3(-0.00972168f, -0.00153078f, 0.00220456f),   glm::vec3(0.0337174f, -0.00375235f, 0.00267416f),
        glm::vec3(0.0274324f, -0.0470846f, 0.0396256f),       glm::vec3(0.0553793f, -0.0522773f, 0.00121295f),
        glm::vec3(-3.61041e-05f, 5.50424e-05f, 5.22396e-05f), glm::vec3(-0.0365617f, 0.0568425f, 0.0104989f),
        glm::vec3(0.000104845f, 0.000472859f, 0.000644243f),  glm::vec3(-0.0228967f, -0.0523075f, 0.0236865f),
        glm::vec3(-0.00424572f, -0.00117935f, 0.00360293f),   glm::vec3(-0.00766907f, 0.0271873f, 0.00838263f),
        glm::vec3(0.0423186f, -0.0607518f, 0.0338392f),       glm::vec3(0.0123928f, -0.102331f, 0.0869199f),
        glm::vec3(-0.102814f, -0.00753441f, 0.0671462f),      glm::vec3(-0.045918f, 0.0251289f, 0.0423358f),
        glm::vec3(0.0996865f, -0.108092f, 0.00722318f),       glm::vec3(0.0337167f, 0.00691922f, 0.0527856f),
        glm::vec3(-0.0245575f, -0.0295447f, 0.00929659f),     glm::vec3(-0.0169255f, -0.0732408f, 0.0967542f),
        glm::vec3(-0.00599817f, 0.136796f, 0.0758857f),       glm::vec3(0.060153f, -0.0480018f, 0.00252684f),
        glm::vec3(0.0667254f, -0.130462f, 0.0531226f),        glm::vec3(0.00518425f, -0.0193347f, 0.0402627f),
        glm::vec3(-0.00529004f, 0.00113642f, 0.00458855f),    glm::vec3(0.044896f, 0.0558538f, 0.079108f),
        glm::vec3(0.146067f, -0.0387894f, 0.173785f),         glm::vec3(0.0634599f, 0.0341871f, 0.0449691f),
        glm::vec3(-0.136777f, 0.0316928f, 0.0199064f),        glm::vec3(-0.0851003f, 0.224686f, 0.0110175f),
        glm::vec3(-0.0944882f, 0.102203f, 0.0803089f),        glm::vec3(-0.0962646f, 0.0582985f, 0.278464f),
        glm::vec3(0.0210792f, 0.0528432f, 0.0693492f),        glm::vec3(0.0456483f, -0.0307739f, 0.0106436f),
        glm::vec3(0.106104f, -0.105554f, 0.00812317f),        glm::vec3(-0.0651668f, -0.0227368f, 0.083517f),
        glm::vec3(0.0416594f, -0.06417f, 0.000364622f),       glm::vec3(0.153925f, 0.142051f, 0.237554f),
        glm::vec3(-0.281793f, 0.0700983f, 0.255119f),         glm::vec3(-0.339123f, 0.133376f, 0.158232f),
        glm::vec3(0.0439649f, 0.124815f, 0.15388f),           glm::vec3(-0.122976f, -0.114001f, 0.0466184f),
        glm::vec3(-0.196737f, 0.190013f, 0.174989f),          glm::vec3(0.0477798f, 0.0317118f, 0.126712f),
        glm::vec3(-0.0124588f, 0.0274616f, 0.198913f),        glm::vec3(0.189305f, 0.418755f, 0.053084f),
        glm::vec3(0.018377f, 0.0664965f, 0.114079f),          glm::vec3(-0.000321885f, -0.0126552f, 0.0199989f),
        glm::vec3(-0.0141842f, 0.0412384f, 0.0511639f),       glm::vec3(-0.125039f, -0.0193449f, 0.00405294f),
        glm::vec3(0.100512f, 0.291612f, 0.0115791f),          glm::vec3(0.0235946f, 0.174194f, 0.432702f),
        glm::vec3(-0.16655f, -0.239599f, 0.301457f),          glm::vec3(0.112731f, 0.0175751f, 0.0905161f),
        glm::vec3(-0.301193f, 0.315314f, 0.0814529f),         glm::vec3(-0.547273f, 0.314661f, 0.234036f),
        glm::vec3(0.125792f, 0.0507683f, 0.189777f),          glm::vec3(0.219913f, -0.233556f, 0.187494f),
        glm::vec3(-0.188157f, -0.169024f, 0.241228f),         glm::vec3(0.0187183f, 0.182357f, 0.212331f),
        glm::vec3(0.184861f, -0.176377f, 0.188422f),          glm::vec3(-0.166299f, 0.173498f, 0.0678085f),
        glm::vec3(-0.0001789f, 0.000362477f, 0.000279712f),   glm::vec3(0.189183f, -0.0773195f, 0.214502f),
    };

    ASSERT_EQ(kernel.size(), expectedKernel.size())
        << "Generated kernel size (" << kernel.size() << ") doesn't match expected size (" << expectedKernel.size()
        << ")";

    auto vectorsEqual = [](const glm::vec3 &a, const glm::vec3 &b, float tolerance = 1e-6f) {
        return std::abs(a.x - b.x) < tolerance && std::abs(a.y - b.y) < tolerance && std::abs(a.z - b.z) < tolerance;
    };

    for (size_t i = 0; i < expectedKernel.size(); ++i) {
        EXPECT_TRUE(vectorsEqual(kernel[i], expectedKernel[i]))
            << "Kernel sample " << i << " doesn't match expected values."
            << "\nExpected: (" << expectedKernel[i].x << ", " << expectedKernel[i].y << ", " << expectedKernel[i].z
            << ")"
            << "\nActual:   (" << kernel[i].x << ", " << kernel[i].y << ", " << kernel[i].z << ")";
    }
}