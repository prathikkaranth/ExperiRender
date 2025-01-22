#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_loader.h>	
#include <functional>
#include <vk_utils.h>

#include "VkBootstrap.h"

class VulkanEngine;

class ssao {
public:
	VkDescriptorSet _ssaoInputDescriptors{};
	VkDescriptorSetLayout _ssaoInputDescriptorLayout{};

	VkDescriptorSet _ssaoBlurInputDescriptors{};
	VkDescriptorSetLayout _ssaoBlurInputDescriptorLayout{};

	VkPipelineLayout _ssaoPipelineLayout;
	VkPipeline _ssaoPipeline;

	VkPipelineLayout _ssaoBlurPipelineLayout;
	VkPipeline _ssaoBlurPipeline;

	SSAOSceneData ssaoData;

	//SSAO resources
	AllocatedImage _ssaoImage;
	AllocatedImage _ssaoImageBlurred;
	AllocatedImage _depthMap;
	VkExtent2D _ssaoExtent{};
	VkExtent2D _depthMapExtent{};

	// SSAO
	void init_ssao(VulkanEngine* engine);
	void init_ssao_blur(VulkanEngine* engine);

	void draw_ssao(VulkanEngine* engine, VkCommandBuffer cmd);
	void draw_ssao_blur(VulkanEngine* engine, VkCommandBuffer cmd);

};