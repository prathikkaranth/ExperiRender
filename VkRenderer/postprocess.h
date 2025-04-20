#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vk_types.h>

struct CompositorData {
	int useRayTracer;
	float exposure;
};

class VulkanEngine;

class PostProcessor {
public:
	void init(VulkanEngine* engine);
	void draw(VulkanEngine* engine, VkCommandBuffer cmd);
	void cleanup(VulkanEngine* engine);

	//Fullscreen resources
	AllocatedImage _fullscreenImage;
	CompositorData _compositorData;

private:
	VkPipelineLayout _postProcessPipelineLayout;
	VkPipeline _postProcessPipeline;

	VkDescriptorSet _postProcessDescriptorSet;
	VkDescriptorSetLayout _postProcessDescriptorSetLayout;
	VkSampler _fullscreenImageSampler;
};