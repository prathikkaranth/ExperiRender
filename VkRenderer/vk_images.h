#pragma once

#include <vk_engine.h>
#include <vulkan/vulkan.h>

namespace vkutil {

    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
                          VkImageAspectFlags mask);
    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
                             VkExtent2D dstSize, VkFilter filter, VkImageAspectFlags mask);
    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
    AllocatedImage create_image(const VulkanEngine *engine, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
                                bool mipmapped = false, const char *name = nullptr);
    AllocatedImage create_image(const VulkanEngine *engine, void *data, VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped = false, const char *name = nullptr);
    AllocatedImage create_hdri_image(const VulkanEngine *engine, float *data, int width, int height, int nrComponents,
                                     const char *name = nullptr);
    void destroy_image(const VulkanEngine *engine, const AllocatedImage &image);
} // namespace vkutil