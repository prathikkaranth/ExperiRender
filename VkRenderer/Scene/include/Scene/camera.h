#pragma once
#include <SDL_events.h>
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    bool fpsCameraEnabled = false;
    // vertical rotation
    float pitch{0.f};
    // horizontal rotation
    float yaw{0.f};
    bool isRotating = false;
    bool isMoving = false;
    // To track rotation change
    float prevPitch;
    float prevYaw;
    // Movement sensitivity
    float moveSensitivity = 0.03f;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getRotationMatrix() const;

    void processSDLEvent(SDL_Event &e);

    void update();
};