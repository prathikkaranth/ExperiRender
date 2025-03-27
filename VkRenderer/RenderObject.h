#pragma once

#include <vk_types.h>
#include <vk_loader.h>	
#include "VkBootstrap.h"

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	uint32_t vertexCount;
	VkBuffer indexBuffer;

	MaterialInstance* material;
	Bounds bounds;
	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
	VkDeviceAddress indexBufferAddress;
};

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
	std::vector<RenderObject> TransparentSurfaces;

};