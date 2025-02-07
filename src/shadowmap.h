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
	VkDescriptorSet shadowMapDescriptorSet;

	VkPipelineLayout _depthShadowMapPipelineLayout;
	VkPipeline _depthShadowMapPipeline;

	VkPipelineLayout _shadowmapPipelineLayout;
	VkPipeline _shadowmapPipeline;

	//Shadowmap resources
	AllocatedImage _depthShadowMap;
	VkExtent2D _shadowMapExtent{ 4096, 4096 };
	VkSampler _shadowDepthMapSampler;

	float near_plane, far_plane;
	float left, right, bottom, top;
	glm::mat4 lightProjection;

	// Shadowmap
	void init_lightSpaceMatrix(VulkanEngine* engine);
	void update_lightSpaceMatrix(VulkanEngine* engine);
	void init_depthShadowMap(VulkanEngine* engine);
	void draw_depthShadowMap(VulkanEngine* engine, VkCommandBuffer cmd);
};