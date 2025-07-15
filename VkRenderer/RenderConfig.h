#pragma once

// Shared constants that work in both C++ and GLSL
#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080
#define DEFAULT_FOV_DEGREES 75.0f
#define NEAR_PLANE 0.01f
#define FAR_PLANE 10000.0f
#define FRAME_OVERLAP 1

// C++ specific code
#ifdef __cplusplus
#include <vulkan/vulkan.h>

/**
 * Centralized render configuration constants.
 * These values are used across the entire renderer including shaders.
 * The preprocessor defines above can be used directly in GLSL shaders.
 */
struct RenderConfig {
    // Helper functions (C++ only) - directly use the preprocessor constants
    static constexpr VkExtent2D getDefaultWindowExtent() { return {DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT}; }

    static constexpr float getDefaultAspectRatio() {
        return static_cast<float>(DEFAULT_WINDOW_WIDTH) / static_cast<float>(DEFAULT_WINDOW_HEIGHT);
    }
};

#endif // __cplusplus