#include <Scene/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

constexpr float EPSILON = 1e-6f;

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override { camera = Camera{}; }

    Camera camera;
};

TEST_F(CameraTest, RotationMatrixIdentityWhenZeroRotation) {
    // Default camera has zero pitch and yaw
    camera.pitch = 0.0f;
    camera.yaw = 0.0f;

    glm::mat4 rotation = camera.getRotationMatrix();
    glm::mat4 identity = glm::mat4(1.0f);

    // Check if rotation matrix is identity
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(rotation[i][j], identity[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, RotationMatrixPitchOnly) {
    // Test pitch rotation (around X-axis)
    camera.pitch = glm::half_pi<float>(); // 90 degrees
    camera.yaw = 0.0f;

    glm::mat4 rotation = camera.getRotationMatrix();
    glm::mat4 expected = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(rotation[i][j], expected[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, RotationMatrixYawOnly) {
    // Test yaw rotation (around -Y-axis in camera space)
    camera.pitch = 0.0f;
    camera.yaw = glm::half_pi<float>(); // 90 degrees

    glm::mat4 rotation = camera.getRotationMatrix();
    glm::mat4 expected = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(0.0f, -1.0f, 0.0f));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(rotation[i][j], expected[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, RotationMatrixIsOrthogonal) {
    // Test with arbitrary rotation values
    camera.pitch = 0.5f;
    camera.yaw = 1.2f;

    glm::mat4 rotation = camera.getRotationMatrix();
    glm::mat4 transpose = glm::transpose(rotation);
    glm::mat4 product = rotation * transpose;
    glm::mat4 identity = glm::mat4(1.0f);

    // For rotation matrix: R * R^T = I (orthogonal property)
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(product[i][j], identity[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, ViewMatrixOriginIdentity) {
    // Camera at origin with no rotation should give identity view matrix
    camera.position = glm::vec3(0.0f);
    camera.pitch = 0.0f;
    camera.yaw = 0.0f;

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 identity = glm::mat4(1.0f);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(view[i][j], identity[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, ViewMatrixTranslation) {
    // Camera moved back should translate world forward
    camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.pitch = 0.0f;
    camera.yaw = 0.0f;

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(view[i][j], expected[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, ViewMatrixInverseProperty) {
    // Test that view matrix properly inverts camera transform
    camera.position = glm::vec3(1.0f, 2.0f, 3.0f);
    camera.pitch = 0.3f;
    camera.yaw = 0.7f;

    glm::mat4 view = camera.getViewMatrix();

    // Create the camera model matrix manually
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), camera.position);
    glm::mat4 rotation = camera.getRotationMatrix();
    glm::mat4 cameraModel = translation * rotation;

    // View matrix should be inverse of camera model matrix
    glm::mat4 inverse = glm::inverse(cameraModel);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(view[i][j], inverse[i][j], EPSILON);
        }
    }
}

TEST_F(CameraTest, ViewMatrixDeterminant) {
    // View matrix should preserve volume (determinant = ±1)
    camera.position = glm::vec3(2.0f, -1.0f, 4.0f);
    camera.pitch = 0.8f;
    camera.yaw = -0.5f;

    glm::mat4 view = camera.getViewMatrix();
    float det = glm::determinant(view);

    EXPECT_NEAR(std::abs(det), 1.0f, EPSILON);
}

TEST_F(CameraTest, PitchClampingBounds) {
    // Test that extreme pitch values produce valid matrices
    camera.position = glm::vec3(0.0f);
    camera.yaw = 0.0f;

    // Test maximum pitch (near 90 degrees)
    camera.pitch = glm::half_pi<float>() - 0.01f;
    glm::mat4 rotation_max = camera.getRotationMatrix();
    EXPECT_FALSE(glm::any(glm::isnan(rotation_max[0])));

    // Test minimum pitch (near -90 degrees)
    camera.pitch = -glm::half_pi<float>() + 0.01f;
    glm::mat4 rotation_min = camera.getRotationMatrix();
    EXPECT_FALSE(glm::any(glm::isnan(rotation_min[0])));
}