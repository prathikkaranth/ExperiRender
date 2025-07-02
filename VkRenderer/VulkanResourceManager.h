#pragma once

#include <vk_mem_alloc.h>
#include "vk_types.h"

// Forward declarations
class VulkanEngine;

class VulkanResourceManager {
public:
    VulkanResourceManager() = default;
    ~VulkanResourceManager() = default;

    // Initialize with engine reference
    void init(VulkanEngine *engine);
    void cleanup();

    // Default texture resources
    AllocatedImage getWhiteImage() const { return _whiteImage; }
    AllocatedImage getBlackImage() const { return _blackImage; }
    AllocatedImage getGreyImage() const { return _greyImage; }
    AllocatedImage getErrorCheckerboardImage() const { return _errorCheckerboardImage; }

    // Default samplers
    VkSampler getLinearSampler() const { return _defaultSamplerLinear; }
    VkSampler getNearestSampler() const { return _defaultSamplerNearest; }
    VkSampler getShadowDepthSampler() const { return _defaultSamplerShadowDepth; }

private:
    void createDefaultTextures();
    void createDefaultSamplers();

    VulkanEngine *_engine = nullptr;

    // Default texture resources
    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;

    // Default samplers
    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;
    VkSampler _defaultSamplerShadowDepth;
};