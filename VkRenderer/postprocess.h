#pragma once

#include <vk_types.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

struct CompositorData {
    int useRayTracer;
    float exposure;
    int showGrid;
};

class VulkanEngine;

class PostProcessor {
public:
    void init(VulkanEngine *engine);
    void draw(VulkanEngine *engine, VkCommandBuffer cmd);
    void draw_grid_only(VulkanEngine *engine, VkCommandBuffer cmd);
    void draw_grid_geometry(VulkanEngine *engine, VkCommandBuffer cmd);

    // Fullscreen resources
    AllocatedImage _fullscreenImage{};
    CompositorData _compositorData{};

    // Getter for the sampler
    VkSampler getFullscreenImageSampler() const { return _fullscreenImageSampler; }

private:
    VkPipelineLayout _postProcessPipelineLayout = nullptr;
    VkPipeline _postProcessPipeline = nullptr;

    VkDescriptorSet _postProcessDescriptorSet = nullptr;
    VkDescriptorSetLayout _postProcessDescriptorSetLayout = nullptr;
    VkSampler _fullscreenImageSampler = nullptr;

    VkPipelineLayout _gridPipelineLayout = nullptr;
    VkPipeline _gridPipeline = nullptr;
    VkDescriptorSetLayout _gridDescriptorSetLayout = nullptr;

    VkPipelineLayout _gridGeometryPipelineLayout = nullptr;
    VkPipeline _gridGeometryPipeline = nullptr;
};