#include "vk_buffers.h"
#include "vk_engine.h"

namespace vkutil {

    AllocatedBuffer create_buffer(const VulkanEngine *engine, size_t allocSize, VkBufferUsageFlags usage,
                                  VmaMemoryUsage memoryUsage) {
        // allocate buffer
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer{};

        // allocate the buffer
        VK_CHECK(vmaCreateBuffer(engine->_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer,
                                 &newBuffer.allocation, &newBuffer.info));

        return newBuffer;
    }

    void destroy_buffer(const VulkanEngine *engine, const AllocatedBuffer &buffer) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(engine->_allocator, buffer.buffer, buffer.allocation);
        }
    }

    void upload_to_buffer(const VulkanEngine *engine, const void *src, size_t size, const AllocatedBuffer &dst_buffer,
                          size_t dst_offset) {
        run_with_mapped_memory(engine->_allocator, dst_buffer.allocation,
                               [&](void *dst) { memcpy(static_cast<std::uint8_t *>(dst) + dst_offset, src, size); });
    }

} // namespace vkutil