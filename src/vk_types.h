#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"


struct AllocatedImage {

	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;

};

struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

struct Vertex {

	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	alignas(16) glm::vec4 color;
	alignas(16) glm::vec3 tangent;
	alignas(16) glm::vec3 bitangent;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {

	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
	glm::mat4 worldMatrix;
	VkDeviceAddress vertexBuffer;
};

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; // w for sun power
	glm::vec4 sunlightColor;
	glm::vec3 cameraPosition;
	int hasSpecular;
	int viewGbufferPos;
};

struct SSAOSceneData {
	alignas(16) glm::vec4 samples[128];
	alignas(16) glm::mat4 viewproj;
	alignas(16) glm::mat4 view;
	int kernelSize;
	float radius;
	float bias;
	float intensity;
};

enum class MaterialPass :uint8_t {
	MainColor,
	Transparent,
	Other
};
struct MaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance {
	MaterialPipeline* pipeline;
	VkDescriptorSet materialSet;
	MaterialPass passType;
};

struct DrawContext;

// base class for a renderable dynamic object
class IRenderable {
	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// implementation of a drawable scene node
// the scene node can hold children and will also keep a transform to propogate
// to them

struct Node : public IRenderable {
	
	// parent pointer must be a weak pointer to avoid circular references
	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children;

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	void refreshTransform(const glm::mat4& parentMatrix) {
		worldTransform = parentMatrix * localTransform;
		for (auto& child : children) {
			child->refreshTransform(worldTransform);
		}
	}

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx){
		
		// draw children
		for (auto& child : children) {
			child->Draw(topMatrix, ctx);
		}
	}
};