#pragma once 
#include <optional>
#include "vk_types.h"

class VulkanEngine;

class HDRI {
public:
	void load_hdri_to_buffer(VulkanEngine* engine);
	void init_hdriMap(VulkanEngine* engine);
	void draw_hdriMap(VulkanEngine* engine, VkCommandBuffer cmd);

	AllocatedImage get_hdriOutImage() { return _hdriOutImage; }
	VkSampler get_hdriMapSampler() { return _hdriMapSampler; }
private:

	VkDescriptorSet hdriMapDescriptorSet{};
	VkDescriptorSetLayout hdriMapDescriptorSetLayout;

	VkPipelineLayout _hdriMapPipelineLayout;
	VkPipeline _hdriMapPipeline{};

	AllocatedImage _hdriMap;
	AllocatedImage _hdriOutImage;
	VkSampler _hdriMapSampler;

};