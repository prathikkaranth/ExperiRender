#pragma once

#include <vk_mem_alloc.h>
#include <vk_types.h>
#include <vk_utils.h>

class VulkanEngine;

namespace vkutil {

    // Memory mapping utility template
    template<typename Fn>
    void run_with_mapped_memory(VmaAllocator allocator, VmaAllocation allocation, Fn &&fn) {
        void *mapped_memory;
        VK_CHECK(vmaMapMemory(allocator, allocation, &mapped_memory));
        fn(mapped_memory);
        vmaUnmapMemory(allocator, allocation);
    }

    // Buffer creation and destruction functions
    AllocatedBuffer create_buffer(const VulkanEngine *engine, size_t allocSize, VkBufferUsageFlags usage,
                                  VmaMemoryUsage memoryUsage, const char *name = nullptr);
    void destroy_buffer(const VulkanEngine *engine, const AllocatedBuffer &buffer);

    // Memory upload utility
    void upload_to_buffer(const VulkanEngine *engine, const void *src, size_t size, const AllocatedBuffer &dst_buffer,
                          size_t dst_offset = 0);

} // namespace vkutil