#pragma once

#include <functional>
#include <vk_descriptors.h>
#include <vk_loader.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <vk_utils.h>

#include "VkBootstrap.h"

class VulkanEngine;

class ssao {
public:
    VkDescriptorSet _ssaoInputDescriptors{};
    VkDescriptorSetLayout _ssaoInputDescriptorLayout{};

    VkDescriptorSet _ssaoBlurInputDescriptors{};
    VkDescriptorSetLayout _ssaoBlurInputDescriptorLayout{};

    // Descriptor set for the ImGui SSAO pass
    VkDescriptorSet _ssaoDescriptorSet{};

    VkPipelineLayout _ssaoPipelineLayout{};
    VkPipeline _ssaoPipeline{};

    VkPipelineLayout _ssaoBlurPipelineLayout{};
    VkPipeline _ssaoBlurPipeline{};

    SSAOSceneData ssaoData{};

    // SSAO resources
    AllocatedImage _ssaoImage{};
    AllocatedImage _ssaoImageBlurred{};
    VkSampler _ssaoSampler{};
    AllocatedImage _depthMap{};
    VkExtent2D _ssaoExtent{};
    VkExtent2D _depthMapExtent{};
    AllocatedImage _ssaoNoiseImage{};

    // SSAO
    std::vector<glm::vec3> generate_ssao_kernels() const;
    static float ssaoLerp(float a, float b, float f);
    void init_ssao_data(VulkanEngine *engine);
    void init_ssao(VulkanEngine *engine);
    void init_ssao_blur(VulkanEngine *engine);

    void draw_ssao(VulkanEngine *engine, VkCommandBuffer cmd) const;
    void draw_ssao_blur(const VulkanEngine *engine, VkCommandBuffer cmd) const;
};