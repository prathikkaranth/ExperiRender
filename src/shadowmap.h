#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_loader.h>	
#include <functional>
#include <vk_utils.h>
#include <glm/gtx/transform.hpp>

#include "VkBootstrap.h"

class VulkanEngine;

class shadowMap {

public:
	VkDescriptorSet _depthShadowMapInputDescriptors{};
	VkDescriptorSetLayout _depthShadowMapInputDescriptorLayout{};

	VkDescriptorSet _shadowmapDescriptors{};
	VkDescriptorSetLayout _shadowmapDescriptorLayout{};

	VkPipelineLayout _depthShadowMapPipelineLayout;
	VkPipeline _depthShadowMapPipeline;

	VkPipelineLayout _shadowmapPipelineLayout;
	VkPipeline _shadowmapPipeline;

	//Shadowmap resources
	AllocatedImage _depthShadowMap;
	AllocatedImage _shadowMapImage;
	VkExtent2D _shadowMapExtent{};

	// Shadowmap
	void init_lightSpaceMatrix(VulkanEngine* engine);
	void init_depthShadowMap(VulkanEngine* engine);
	void draw_depthShadowMap(VulkanEngine* engine, VkCommandBuffer cmd);
};