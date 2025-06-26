#pragma once
#include "vk_types.h"

class VulkanEngine;

class CubePipeline {
public:
    void init(VulkanEngine *engine);
    void draw(VulkanEngine *engine, VkCommandBuffer cmd);
    void destroy();

    bool isInitialized() const { return hasPipeline; }

private:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    bool hasPipeline = false;
    
    VulkanEngine* enginePtr = nullptr;
};