#pragma once

#include "vk_descriptors.h"

class VulkanEngine;

class Gbuffer {
public:
    void init_gbuffer(VulkanEngine *engine);
    void draw_gbuffer(VulkanEngine *engine, VkCommandBuffer cmd);

    [[nodiscard]] AllocatedImage getGbufferPosInfo() const { return _gbufferPosition; }

    [[nodiscard]] AllocatedImage getGbufferNormInfo() const { return _gbufferPosition; }

    VkDescriptorSet *getInputDescriptorSet() { return &_gbufferInputDescriptors; }

    [[nodiscard]] VkDescriptorSetLayout getInputDescriptorSetLayout() const { return _gbufferInputDescriptorLayout; }

    [[nodiscard]] VkSampler getGbufferSampler() const { return _gbufferSampler; }

    // For UI Debug feature, need to keep a seperate Pos Outpute Desc Layout public.
    // TODO: get it working on old desc sets.
    VkDescriptorSet _gbufferPosOutputDescriptor{};

private:
    VkDescriptorSet _gbufferInputDescriptors{};
    VkDescriptorSetLayout _gbufferInputDescriptorLayout{};

    VkPipelineLayout _gbufferPipelineLayout{};
    VkPipeline _gbufferPipeline{};

    VkSampler _gbufferSampler{};

    AllocatedImage _gbufferPosition{};
    AllocatedImage _gbufferNormal{};
};