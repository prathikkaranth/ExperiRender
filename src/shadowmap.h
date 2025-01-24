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

	ShadowmapSceneData shadowmapData;

	//Shadowmap resources
	AllocatedImage _depthShadowMap;
	AllocatedImage _shadowMapImage;
	VkExtent2D _shadowMapExtent{};

	// Shadowmap defaults
	float near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;

	// Shadowmap
	void init_depthShadowMap(VulkanEngine* engine);
	void init_shadowMap(VulkanEngine* engine);
	void draw_depthShadowMap(VulkanEngine* engine, VkCommandBuffer cmd);
	void draw_shadowMap(VulkanEngine* engine, VkCommandBuffer cmd);
};