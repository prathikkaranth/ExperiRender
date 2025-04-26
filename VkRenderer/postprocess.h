#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vk_types.h>

struct CompositorData {
	int useRayTracer;
	int useDenoiser;

	// Denoising parameters
	float sigma; // Standard deviation for Gaussian filter
	float kSigma; // Standard deviation for kernel size
	float threshold; // Threshold for denoising

	float exposure;
};

class VulkanEngine;

class PostProcessor {
public:
	void init(VulkanEngine* engine);
	void draw(VulkanEngine* engine, VkCommandBuffer cmd);

	//Fullscreen resources
	AllocatedImage _fullscreenImage{};
	CompositorData _compositorData{};

private:
	VkPipelineLayout _postProcessPipelineLayout = nullptr;
	VkPipeline _postProcessPipeline = nullptr;

	VkDescriptorSet _postProcessDescriptorSet = nullptr;
	VkDescriptorSetLayout _postProcessDescriptorSetLayout = nullptr;
	VkSampler _fullscreenImageSampler = nullptr;
};