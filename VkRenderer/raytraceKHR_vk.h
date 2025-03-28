/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vulkan/vulkan_core.h>
#include <cassert>


namespace experirender::vk
{

	void load_raytracing_functions(VkInstance instance);

} 


 /**

 \class nvvk::RaytracingBuilderKHR

 \brief nvvk::RaytracingBuilderKHR is a base functionality of raytracing

 This class acts as an owning container for a single top-level acceleration
 structure referencing any number of bottom-level acceleration structures.
 We provide functions for building (on the device) an array of BLASs and a
 single TLAS from vectors of BlasInput and Instance, respectively, and
 a destroy function for cleaning up the created acceleration structures.

 Generally, we reference BLASs by their index in the stored BLAS array,
 rather than using raw device pointers as the pure Vulkan acceleration
 structure API uses.

 This class does not support replacing acceleration structures once
 built, but you can update the acceleration structures. For educational
 purposes, this class prioritizes (relative) understandability over
 performance, so vkQueueWaitIdle is implicitly used everywhere.

 # Setup and Usage
 \code{.cpp}
 // Borrow a VkDevice and memory allocator pointer (must remain
 // valid throughout our use of the ray trace builder), and
 // instantiate an unspecified queue of the given family for use.
 m_rtBuilder.setup(device, memoryAllocator, queueIndex);

 // You create a vector of RayTracingBuilderKHR::BlasInput then
 // pass it to buildBlas.
 std::vector<RayTracingBuilderKHR::BlasInput> inputs = // ...
 m_rtBuilder.buildBlas(inputs);

 // You create a vector of RaytracingBuilder::Instance and pass to
 // buildTlas. The blasId member of each instance must be below
 // inputs.size() (above).
 std::vector<VkAccelerationStructureInstanceKHR> instances = // ...
 m_rtBuilder.buildTlas(instances);

 // Retrieve the handle to the acceleration structure.
 const VkAccelerationStructureKHR tlas = m.rtBuilder.getAccelerationStructure()
 \endcode
 */

#include <mutex>

#if VK_KHR_acceleration_structure

 // #include "resourceallocator_vk.hpp"
 // #include "debug_util_vk.hpp"
 // #include "nvmath/nvmath.h"
#include <type_traits>
#include <glm/glm.hpp>
#include <cstring>
#include <vector>

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_loader.h>	
#include <functional>
#include <vk_utils.h>

#include "VkBootstrap.h"

class VulkanEngine;

namespace nvvk
{

    struct AccelKHR {
        VkAccelerationStructureKHR accel{ nullptr };
        AllocatedBuffer buffer;
    };

    // Convert a Mat4x4 to the matrix required by acceleration structures
    inline VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix)
    {
        // VkTransformMatrixKHR uses a row-major memory layout, while nvmath::mat4f
        // uses a column-major memory layout. We transpose the matrix so we can
        // memcpy the matrix's data directly.
        glm::mat4 temp = glm::transpose(matrix);
        VkTransformMatrixKHR out_matrix;
        std::memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
        return out_matrix;
    }

    // Ray tracing BLAS and TLAS builder
    class RaytracingBuilderKHR
    {
    public:
        RaytracingBuilderKHR(VulkanEngine* engine, uint32_t queueIndex);

        // Inputs used to build Bottom-level acceleration structure.
        // You manage the lifetime of the buffer(s) referenced by the VkAccelerationStructureGeometryKHRs within.
        // In particular, you must make sure they are still valid and not being modified when the BLAS is built or updated.
        struct BlasInput {
            // Data used to build acceleration structure geometry
            std::vector<VkAccelerationStructureGeometryKHR> asGeometry;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
            VkBuildAccelerationStructureFlagsKHR flags{ 0 };
        };

        // Destroying all allocations
        void destroy();

        // Returning the constructed top-level acceleration structure
        VkAccelerationStructureKHR getAccelerationStructure() const;

        // Return the Acceleration Structure Device Address of a BLAS Id
        VkDeviceAddress getBlasDeviceAddress(uint32_t blasId);

        // Create all the BLAS from the vector of BlasInput
        void
            buildBlas(const std::vector<BlasInput>& input,
                VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        // Refit BLAS number blasIdx from updated buffer contents.
        void updateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);

        // Build TLAS for static acceleration structures
        void
            buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
                VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                bool update = false);

#ifdef VK_NV_ray_tracing_motion_blur
        // Build TLAS for mix of motion and static acceleration structures
        void buildTlas(const std::vector<VkAccelerationStructureMotionInstanceNV>& instances,
            VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV,
            bool update = false);
#endif

        // Build TLAS from an array of VkAccelerationStructureInstanceKHR
        // - Use motion=true with VkAccelerationStructureMotionInstanceNV
        // - The resulting TLAS will be stored in m_tlas
        // - update is to rebuild the Tlas with updated matrices, flag must have the 'allow_update'
        void
            buildTlas(size_t num_instances,
                void* instance_data,
                size_t sizeof_instance,
                VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                bool update = false,
                bool motion = false);

        // Creating the TLAS, called by buildTlas
        void cmdCreateTlas(VkCommandBuffer cmdBuf,          // Command buffer
            uint32_t countInstance,                         // number of instances
            VkDeviceAddress instBufferAddr,                 // Buffer address of instances
            AllocatedBuffer& scratchBuffer,                 // Scratch buffer for construction
            VkBuildAccelerationStructureFlagsKHR flags,     // Build creation flag
            bool update,                                    // Update == animation
            bool motion                                     // Motion Blur
        );

#ifdef VULKAN_HPP
    public:
        void buildBlas(const std::vector<RaytracingBuilderKHR::BlasInput>& blas_,
            vk::BuildAccelerationStructureFlagsKHR flags)
        {
            buildBlas(blas_, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags));
        }

        void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
            vk::BuildAccelerationStructureFlagsKHR flags,
            bool update = false)
        {
            buildTlas(instances, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags), update);
        }

#endif

    protected:
        std::vector<nvvk::AccelKHR> m_blas; // Bottom-level acceleration structure
        nvvk::AccelKHR m_tlas;              // Top-level acceleration structure

        // Setup
        uint32_t m_queueIndex{ 0 };
        VulkanEngine* m_engine_ptr;
        VkCommandPool m_cmd_pool;

        struct BuildAccelerationStructure {
            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
            nvvk::AccelKHR as; // result acceleration structure
            nvvk::AccelKHR cleanupAS;
        };

        void cmdCreateBlas(VkCommandBuffer cmdBuf,
            std::vector<uint32_t> indices,
            std::vector<BuildAccelerationStructure>& buildAs,
            VkDeviceAddress scratchAddress,
            VkQueryPool queryPool);
        void cmdCompactBlas(VkCommandBuffer cmdBuf,
            std::vector<uint32_t> indices,
            std::vector<BuildAccelerationStructure>& buildAs,
            VkQueryPool queryPool);
        void destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs);
        bool hasFlag(VkFlags item, VkFlags flag)
        {
            return (item & flag) == flag;
        }
    };

} // namespace nvvk

#else
#error This include requires VK_KHR_acceleration_structure support in the Vulkan SDK.
#endif
