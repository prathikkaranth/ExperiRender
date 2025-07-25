﻿#pragma once

#include <array>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include <AllocatedBuffer.h>
#include <AllocatedImage.h>

// holds the resources needed for a mesh
struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
    VkDeviceAddress indexBufferAddress;
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
    glm::mat4 lightSpaceMatrix;
    alignas(16) glm::vec4 ambientColor;
    alignas(16) glm::vec4 sunlightDirection; // w for sun power
    alignas(16) glm::vec4 sunlightColor;
    alignas(16) glm::vec4 pointLightPosition; // w for range/radius
    alignas(16) glm::vec4 pointLightColor; // w for intensity
    alignas(16) glm::vec3 cameraPosition;
    int enableShadows;
    int enableSSAO;
    int enablePBR;
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

// Information of a obj model when referenced in a shader
struct alignas(8) ObjDesc {
    uint64_t vertexAddress; // Address of the Vertex buffer
    uint64_t indexAddress; // Address of the index buffer
    uint32_t firstIndex; // First index of the mesh
    uint32_t padding;
};


enum class MaterialPass : uint8_t { MainColor, Transparent, Other };
struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline *pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;

    AllocatedImage colImage;
    VkSampler colSampler;

    AllocatedImage normImage;
    VkSampler normSampler;

    AllocatedImage metalRoughImage;
    VkSampler metalRoughSampler;

    AllocatedImage emissiveImage;
    VkSampler emissiveSampler;

    uint32_t matIndex;

    glm::vec4 albedo;
    uint32_t albedoTexIndex;
    glm::vec4 metalRoughFactors;
    float transmissionFactor;
    float ior;
    glm::vec4 emissiveFactor;
    uint32_t emissiveTexIndex;
};

struct DrawContext;

// base class for a renderable dynamic object
class IRenderable {
public:
    virtual ~IRenderable() = default;

private:
    virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) = 0;
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

    void refreshTransform(const glm::mat4 &parentMatrix) {
        worldTransform = parentMatrix * localTransform;
        for (auto &child: children) {
            child->refreshTransform(worldTransform);
        }
    }

    virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {

        // draw children
        for (auto &child: children) {
            child->Draw(topMatrix, ctx);
        }
    }
};