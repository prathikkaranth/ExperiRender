#pragma once

#include <glm/glm.hpp>
#include <vk_types.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

struct CompositorData {
    int useRayTracer;
    float exposure;
    int showGrid;
    int useFXAA;
};

struct FXAAData {
    alignas(16) glm::vec3 R_inverseFilterTextureSize;
    float R_fxaaSpanMax;
    float R_fxaaReduceMin;
    float R_fxaaReduceMul;
};

class VulkanEngine;

class PostProcessor {
public:
    void init(VulkanEngine *engine);
    void draw(VulkanEngine *engine, VkCommandBuffer cmd);
    void draw_fxaa(VulkanEngine *engine, VkCommandBuffer cmd);
    void draw_grid_only(VulkanEngine *engine, VkCommandBuffer cmd);
    void draw_grid_geometry(VulkanEngine *engine, VkCommandBuffer cmd);

    // Fullscreen resources
    AllocatedImage _fullscreenImage{};
    AllocatedImage _fxaaImage{};
    CompositorData _compositorData{};
    FXAAData _fxaaData{};

    // Getter for the sampler
    VkSampler getFullscreenImageSampler() const { return _fullscreenImageSampler; }

    // Get the final processed image (FXAA output if enabled, otherwise fullscreen image)
    const AllocatedImage &getFinalImage() const { return _compositorData.useFXAA ? _fxaaImage : _fullscreenImage; }

    // Get the final image view for UI display
    VkImageView getFinalImageView() const {
        return _compositorData.useFXAA ? _fxaaImage.imageView : _fullscreenImage.imageView;
    }

private:
    VkPipelineLayout _postProcessPipelineLayout = nullptr;
    VkPipeline _postProcessPipeline = nullptr;

    VkDescriptorSet _postProcessDescriptorSet = nullptr;
    VkDescriptorSetLayout _postProcessDescriptorSetLayout = nullptr;
    VkSampler _fullscreenImageSampler = nullptr;

    // FXAA pipeline
    VkPipelineLayout _fxaaPipelineLayout = nullptr;
    VkPipeline _fxaaPipeline = nullptr;
    VkDescriptorSet _fxaaDescriptorSet = nullptr;
    VkDescriptorSetLayout _fxaaDescriptorSetLayout = nullptr;

    VkPipelineLayout _gridPipelineLayout = nullptr;
    VkPipeline _gridPipeline = nullptr;
    VkDescriptorSetLayout _gridDescriptorSetLayout = nullptr;

    VkPipelineLayout _gridGeometryPipelineLayout = nullptr;
    VkPipeline _gridGeometryPipeline = nullptr;
};