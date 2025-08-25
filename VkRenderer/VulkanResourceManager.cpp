#include "VulkanResourceManager.h"
#include <glm/gtc/packing.hpp>
#include "vk_engine.h"
#include "vk_images.h"

void VulkanResourceManager::init(VulkanEngine *engine) {
    _engine = engine;
    createDefaultTextures();
    createDefaultSamplers();
}

void VulkanResourceManager::cleanup() const {
    if (!_engine)
        return;

    // Cleanup samplers
    vkDestroySampler(_engine->_device, _defaultSamplerNearest, nullptr);
    vkDestroySampler(_engine->_device, _defaultSamplerLinear, nullptr);
    vkDestroySampler(_engine->_device, _defaultSamplerShadowDepth, nullptr);

    // Cleanup default textures
    vkutil::destroy_image(_engine, _whiteImage);
    vkutil::destroy_image(_engine, _greyImage);
    vkutil::destroy_image(_engine, _blackImage);
    vkutil::destroy_image(_engine, _errorCheckerboardImage);
}

void VulkanResourceManager::createDefaultTextures() {
    // 3 default textures, white, grey and black. 1 pixel each.
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    _whiteImage = vkutil::create_image(_engine, (void *) &white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT, false, "whiteImage");

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    _greyImage = vkutil::create_image(_engine, (void *) &grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                      VK_IMAGE_USAGE_SAMPLED_BIT, false, "greyImage");

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    _blackImage = vkutil::create_image(_engine, (void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT, false, "blackImage");

    // checkerboard texture
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 * 16> pixels{};
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[x + y * 16] = x % 2 ^ y % 2 ? magenta : black;
        }
    }
    _errorCheckerboardImage =
        vkutil::create_image(_engine, pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_USAGE_SAMPLED_BIT, false, "errorCheckerboardImage");
}

void VulkanResourceManager::createDefaultSamplers() {
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    // Common settings for all samplers
    sampl.maxLod = VK_LOD_CLAMP_NONE;
    sampl.minLod = 0;
    sampl.mipLodBias = 0.0f;
    sampl.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampl.anisotropyEnable = VK_TRUE;
    sampl.maxAnisotropy = 16.0f;
    sampl.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_engine->_device, &sampl, nullptr, &_defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_engine->_device, &sampl, nullptr, &_defaultSamplerLinear);

    // Shadow depth sampler - disable anisotropic filtering for depth
    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    sampl.anisotropyEnable = VK_FALSE;
    sampl.maxAnisotropy = 1.0f;
    sampl.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampl.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(_engine->_device, &sampl, nullptr, &_defaultSamplerShadowDepth);
}