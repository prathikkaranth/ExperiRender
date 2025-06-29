#pragma once

#include <vk_types.h>

class VulkanEngine;

namespace ExpResource {

    template<typename T>
    class Managed {
    public:
        Managed() = default;
        explicit Managed(T item);

        ~Managed();

        Managed(const Managed<T> &) = delete;
        Managed<T> &operator=(const Managed<T> &) = delete;

        Managed(Managed<T> &&other) noexcept;
        Managed<T> &operator=(Managed<T> &&other) noexcept;

        T get() const { return item_; }
        bool is_valid() const;

    private:
        T item_{};
        VulkanEngine *engine_ = nullptr;
    };

    // Type aliases for common Vulkan objects
    using ManagedBuffer = Managed<AllocatedBuffer>;
    using ManagedImage = Managed<AllocatedImage>;
    using ManagedPipeline = Managed<VkPipeline>;
    using ManagedPipelineLayout = Managed<VkPipelineLayout>;
    using ManagedDescriptorSetLayout = Managed<VkDescriptorSetLayout>;
    using ManagedSampler = Managed<VkSampler>;
    using ManagedAccelerationStructure = Managed<VkAccelerationStructureKHR>;

} // namespace ExpResource