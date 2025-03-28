#pragma once

#include "vk_descriptors.h"

class VulkanEngine;

class Gbuffer {
public:

	void init_gbuffer(VulkanEngine* engine);
	void draw_gbuffer(VulkanEngine* engine, VkCommandBuffer cmd);

	AllocatedImage getGbufferPosInfo() {
		return _gbufferPosition;
	}

	AllocatedImage getGbufferNormInfo() {
		return _gbufferPosition;
	}

	VkDescriptorSet* getInputDescriptorSet() {
		return &_gbufferInputDescriptors;
	}

	VkDescriptorSetLayout getInputDescriptorSetLayout() {
		return _gbufferInputDescriptorLayout;
	}

	VkSampler getGbufferSampler() {
		return _gbufferSampler;
	}

	// For UI Debug feature, need to keep a seperate Pos Outpute Desc Layout public. 
	// TODO: get it working on old desc sets.
	VkDescriptorSet _gbufferPosOutputDescriptor{};

private:
	VkDescriptorSet _gbufferInputDescriptors{};
	VkDescriptorSetLayout _gbufferInputDescriptorLayout{};

	VkPipelineLayout _gbufferPipelineLayout;
	VkPipeline _gbufferPipeline;

	VkSampler _gbufferSampler;

	AllocatedImage _gbufferPosition;
	AllocatedImage _gbufferNormal;
};