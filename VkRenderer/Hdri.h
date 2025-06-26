#pragma once
#include <optional>
#include <string>
#include "vk_types.h"

class VulkanEngine;

class HDRI {
public:
    void load_hdri_to_buffer(VulkanEngine *engine, const std::string &jsonFilePath);
    void load_hdri_to_buffer_fallback(VulkanEngine *engine);
    void load_hdri_from_file(VulkanEngine *engine, const std::string &hdriFilePath);
    void init_hdriMap(VulkanEngine *engine);
    void draw_hdriMap(VulkanEngine *engine, VkCommandBuffer cmd);
    void cleanup(VulkanEngine *engine);

    [[nodiscard]] AllocatedImage get_hdriMap() const { return _hdriMap; }
    [[nodiscard]] AllocatedImage get_hdriOutImage() const { return _hdriOutImage; }
    [[nodiscard]] VkSampler get_hdriMapSampler() const { return _hdriMapSampler; }

private:
    VkDescriptorSet hdriMapDescriptorSet{};
    VkDescriptorSetLayout hdriMapDescriptorSetLayout{};

    VkPipelineLayout _hdriMapPipelineLayout{};
    VkPipeline _hdriMapPipeline{};

    AllocatedImage _hdriMap{};
    AllocatedImage _hdriOutImage{};
    VkSampler _hdriMapSampler{};
};