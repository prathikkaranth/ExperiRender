#include "Scene/camera.h"
#include <SDL_events.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

glm::mat4 Camera::getViewMatrix() const {
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() const {
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix

    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{0.f, -1.f, 0.f});

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void Camera::processSDLEvent(SDL_Event &e) {

    // Inside your event loop
    if (e.type == SDL_KEYDOWN) {
        // Toggle FPS camera on/off when 'F' key is pressed
        if (e.key.keysym.sym == SDLK_f) {
            fpsCameraEnabled = !fpsCameraEnabled;
        }
    }


    if (e.type == SDL_KEYDOWN && fpsCameraEnabled) {
        if (e.key.keysym.sym == SDLK_w) {
            velocity.z = -moveSensitivity;
        }
        if (e.key.keysym.sym == SDLK_s) {
            velocity.z = moveSensitivity;
        }
        if (e.key.keysym.sym == SDLK_a) {
            velocity.x = -moveSensitivity;
        }
        if (e.key.keysym.sym == SDLK_d) {
            velocity.x = moveSensitivity;
        }
    }

    if (e.type == SDL_KEYUP && fpsCameraEnabled) {
        if (e.key.keysym.sym == SDLK_w) {
            velocity.z = 0.f;
        }
        if (e.key.keysym.sym == SDLK_s) {
            velocity.z = 0.f;
        }
        if (e.key.keysym.sym == SDLK_a) {
            velocity.x = 0.f;
        }
        if (e.key.keysym.sym == SDLK_d) {
            velocity.x = 0.f;
        }
    }

    if (e.type == SDL_MOUSEMOTION && fpsCameraEnabled) {
        yaw += (float) e.motion.xrel / 600.f;
        pitch -= (float) e.motion.yrel / 600.f;

        // Clamp the pitch to prevent flipping
        pitch = glm::clamp(pitch, -glm::half_pi<float>(), glm::half_pi<float>());

        if (e.motion.xrel != 0.0f || e.motion.yrel != 0) {
            isRotating = true;
        }
    }
}

void Camera::update() {
    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));

    // Set isMoving based on position or rotation changes
    isMoving = glm::length(velocity) > 0.0f || isRotating;
}