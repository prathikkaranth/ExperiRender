#pragma once

#include <RenderObject.h>
#include <functional>
#include <raytraceKHR_vk.h>
#include <vk_descriptors.h>
#include <vk_loader.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <vk_utils.h>

#include "VkBootstrap.h"

class VulkanEngine;

// Push constant structure for the ray tracer
struct PushConstantRay {
    std::uint32_t seed;
    std::uint32_t samples_done;
    std::uint32_t depth;
    std::uint32_t useMicrofacetSampling;
};

struct MaterialRTData {
    glm::vec4 albedo;
    uint32_t albedoTexIndex;
    uint32_t padding[3];
    glm::vec4 metal_rough_factors;
    float transmissionFactor;
    uint32_t hasTransmissionTex;
    float ior;
    uint32_t hasEmissiveTex;
    glm::vec4 emissiveFactor;
};

class Raytracer {
public:
    void init_ray_tracing(VulkanEngine *engine);
    void createBottomLevelAS(const VulkanEngine *engine) const;
    void createTopLevelAS(const VulkanEngine *engine) const;
    void createRtDescriptorSet(VulkanEngine *engine);
    void createRtOutputImageOnly(VulkanEngine *engine);
    void updateRtDescriptorSet(const VulkanEngine *engine) const;
    void createRtPipeline(VulkanEngine *engine);
    void createRtShaderBindingTable(VulkanEngine *engine);
    void resetSamples();
    void raytrace(VulkanEngine *engine, const VkCommandBuffer &cmdBuf, const glm::vec4 &clearColor);
    void rtSampleUpdates(const VulkanEngine *engine);
    void setRTDefaultData();

    // Ray tracing features
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

    // Ray tracing accel struct + variables
    bool m_is_raytracing_supported{false};
    std::unique_ptr<nvvk::RaytracingBuilderKHR> m_rt_builder;

    // Ray tracing descriptors
    VkDescriptorSetLayout m_rtDescSetLayout;
    VkDescriptorSet m_rtDescSet;
    VkDescriptorSetLayout m_objDescSetLayout;
    VkDescriptorSet m_objDescSet;
    VkDescriptorSetLayout m_texSetLayout;
    VkDescriptorSet m_texDescSet;
    VkDescriptorSetLayout m_matDescSetLayout;
    VkDescriptorSet m_matDescSet;

    // Ray tracing resources
    AllocatedImage _rtOutputImage;
    // Put all textures in loadScenes to a vector
    std::vector<AllocatedImage> loadedTextures;
    std::vector<AllocatedImage> loadedNormTextures;
    std::vector<AllocatedImage> loadedMetalRoughTextures;
    std::vector<AllocatedImage> loadedEmissiveTextures;

    AllocatedBuffer m_rtSBTBuffer{};
    VkStridedDeviceAddressRegionKHR m_rgenRegion{};
    VkStridedDeviceAddressRegionKHR m_missRegion{};
    VkStridedDeviceAddressRegionKHR m_hitRegion{};
    VkStridedDeviceAddressRegionKHR m_callRegion{};

    // Ray Tracing Pipeline
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
    VkPipelineLayout m_rtPipelineLayout;
    VkPipeline m_rtPipeline;

    // Push constant for ray tracer
    PushConstantRay m_pcRay{};
    std::uint32_t max_samples;
    std::uint32_t prevMaxSamples;
    std::uint32_t prevMaxDepth;
    int prevLightType;
    glm::vec4 prevSunDir;
    bool prevUseMicrofacetSampling;

    // Cleanup tracking
    bool m_cleanupRegistered = false;

    // Rendering options
    bool useMicrofacetSampling = true;
};