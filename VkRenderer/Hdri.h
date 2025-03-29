#pragma once 
#include <optional>
#include "vk_types.h"

class VulkanEngine;

class HDRI {
public:
	void init_hdriMap(VulkanEngine* engine);
private:

	VkDescriptorSet hdriMapDescriptorSet;

	VkPipelineLayout _hdriMapPipelineLayout;
	VkPipeline _hdriMapPipeline;

	AllocatedImage _hdriMap;
	VkSampler _hdriMapSampler;

};