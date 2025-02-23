/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include "raytraceKHR_vk.h"
#include "vk_engine.h"
#include <cinttypes>
#include <numeric>

namespace
{

	PFN_vkCmdCopyAccelerationStructureKHR pfn_vkCmdCopyAccelerationStructureKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pfn_vkCmdWriteAccelerationStructuresPropertiesKHR;
	PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR;
	PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR;
	PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR;

} // namespace

namespace experirender::vk
{
	void load_raytracing_functions(VkInstance instance)
	{
#define VK_GET_INSTANCE_PROC_ADDR(name)                                                                                \
    pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);                                                   \
    assert(pfn_##name);

		VK_GET_INSTANCE_PROC_ADDR(vkCmdCopyAccelerationStructureKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkCmdBuildAccelerationStructuresKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkCmdWriteAccelerationStructuresPropertiesKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkDestroyAccelerationStructureKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkCreateAccelerationStructureKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkGetAccelerationStructureDeviceAddressKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkGetAccelerationStructureBuildSizesKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkCreateRayTracingPipelinesKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkCmdTraceRaysKHR);
		VK_GET_INSTANCE_PROC_ADDR(vkGetRayTracingShaderGroupHandlesKHR);

	}
} // namespace infinirack::vk

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer,
	const VkCopyAccelerationStructureInfoKHR* pInfo)
{
	return pfn_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer,
	uint32_t infoCount,
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
	const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
	return pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer,
	uint32_t accelerationStructureCount,
	const VkAccelerationStructureKHR* pAccelerationStructures,
	VkQueryType queryType,
	VkQueryPool queryPool,
	uint32_t firstQuery)
{
	return pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(
		commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(VkDevice device,
	VkAccelerationStructureKHR accelerationStructure,
	const VkAllocationCallbacks* pAllocator)
{
	return pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(VkDevice device,
	const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkAccelerationStructureKHR* pAccelerationStructure)
{
	return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL
vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
{
	return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}

VKAPI_ATTR void VKAPI_CALL
vkGetAccelerationStructureBuildSizesKHR(VkDevice device,
	VkAccelerationStructureBuildTypeKHR buildType,
	const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
	const uint32_t* pMaxPrimitiveCounts,
	VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
	return pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(VkDevice device,
	VkDeferredOperationKHR deferredOperation,
	VkPipelineCache pipelineCache,
	uint32_t createInfoCount,
	const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines)
{
	return pfn_vkCreateRayTracingPipelinesKHR(
		device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer,
	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
	uint32_t width,
	uint32_t height,
	uint32_t depth)
{
	return pfn_vkCmdTraceRaysKHR(commandBuffer,
		pRaygenShaderBindingTable,
		pMissShaderBindingTable,
		pHitShaderBindingTable,
		pCallableShaderBindingTable,
		width,
		height,
		depth);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
	VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
{
	return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

namespace
{
nvvk::AccelKHR create_acceleration(VulkanEngine* engine,
                                   VkAccelerationStructureCreateInfoKHR& accel_create_info) {

    nvvk::AccelKHR accel;
    accel.buffer = engine->create_buffer(accel_create_info.size,
                                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY);
    vmaSetAllocationName(engine->_allocator, accel.buffer.allocation, "Accel Buffer");
    accel_create_info.buffer = accel.buffer.buffer;
    VkAccelerationStructureKHR accelStructure;
    VK_CHECK(vkCreateAccelerationStructureKHR(engine->_device, &accel_create_info, nullptr, &accelStructure));

    accel.accel = accelStructure;
    return accel;
}
} // namespace

//--------------------------------------------------------------------------------------------------
// Initializing the allocator and querying the raytracing properties
//
nvvk::RaytracingBuilderKHR::RaytracingBuilderKHR(VulkanEngine* engine, uint32_t queueIndex)
{
    m_engine_ptr = engine;
    m_queueIndex = queueIndex;
    {
        
        auto command_pool_info = vkinit::command_pool_create_info(
            engine->_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VkCommandPool graphics_pool;
        VK_CHECK(vkCreateCommandPool(engine->_device, &command_pool_info, nullptr, &graphics_pool));
        m_cmd_pool = graphics_pool;
    }
}

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void nvvk::RaytracingBuilderKHR::destroy()
{

	m_engine_ptr->destroy_buffer(m_tlas.buffer);
    vkDestroyAccelerationStructureKHR(m_engine_ptr->_device, m_tlas.accel, nullptr);
    for (auto& blas : m_blas)
    {
		m_engine_ptr->destroy_buffer(blas.buffer);
		vkDestroyAccelerationStructureKHR(m_engine_ptr->_device, blas.accel, nullptr);
	}
	vkDestroyCommandPool(m_engine_ptr->_device, m_cmd_pool, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Returning the constructed top-level acceleration structure
//
VkAccelerationStructureKHR nvvk::RaytracingBuilderKHR::getAccelerationStructure() const
{
    return m_tlas.accel;
}

//--------------------------------------------------------------------------------------------------
// Return the device address of a Blas previously created.
//
VkDeviceAddress nvvk::RaytracingBuilderKHR::getBlasDeviceAddress(uint32_t blasId)
{
    assert(size_t(blasId) < m_blas.size());
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = m_blas[blasId].accel;
    auto vk_engine = m_engine_ptr;
    return vkGetAccelerationStructureDeviceAddressKHR(vk_engine->_device, &addressInfo);
}

//--------------------------------------------------------------------------------------------------
// Create all the BLAS from the vector of BlasInput
// - There will be one BLAS per input-vector entry
// - There will be as many BLAS as input.size()
// - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
//   and can be referenced by index.
// - if flag has the 'Compact' flag, the BLAS will be compacted
//
void nvvk::RaytracingBuilderKHR::buildBlas(const std::vector<BlasInput>& input,
                                           VkBuildAccelerationStructureFlagsKHR flags)
{
    auto vk_engine = m_engine_ptr;
    const VkDevice raw_device = vk_engine->_device;
    auto nbBlas = static_cast<uint32_t>(input.size());
    VkDeviceSize asTotalSize{0};    // Memory size of all allocated BLAS
    uint32_t nbCompactions{0};      // Nb of BLAS requesting compaction
    VkDeviceSize maxScratchSize{0}; // Largest scratch size

    // Preparing the information for the acceleration build commands.
    std::vector<BuildAccelerationStructure> buildAs(nbBlas);
    for (uint32_t idx = 0; idx < nbBlas; idx++) {
        // Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
        // Other information will be filled in the createBlas (see #2)
        buildAs[idx].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildAs[idx].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildAs[idx].buildInfo.flags = input[idx].flags | flags;
        buildAs[idx].buildInfo.geometryCount = static_cast<uint32_t>(input[idx].asGeometry.size());
        buildAs[idx].buildInfo.pGeometries = input[idx].asGeometry.data();

        // Build range information
        buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo.data();

        // Finding sizes to create acceleration structures and scratch
        std::vector<uint32_t> maxPrimCount(input[idx].asBuildOffsetInfo.size());
        for (size_t tt = 0; tt < input[idx].asBuildOffsetInfo.size(); tt++)
            maxPrimCount[tt] = input[idx].asBuildOffsetInfo[tt].primitiveCount; // Number of primitives/triangles
        vkGetAccelerationStructureBuildSizesKHR(raw_device,
                                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &buildAs[idx].buildInfo,
                                                maxPrimCount.data(),
                                                &buildAs[idx].sizeInfo);

        // Extra info
        asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
        maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
        nbCompactions +=
            hasFlag(buildAs[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    const auto scratchBuffer =
        vk_engine->create_buffer(maxScratchSize,
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(vk_engine->_allocator, scratchBuffer.allocation, "RT BLAS Scratch Buffer");

    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer.buffer };
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(raw_device, &bufferInfo);

    // Allocate a query pool for storing the needed size for every BLAS compaction.
    VkQueryPool queryPool{VK_NULL_HANDLE};
    if (nbCompactions > 0) // Is compaction requested?
    {
        assert(nbCompactions == nbBlas); // Don't allow mix of on/off compaction
        VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        qpci.queryCount = nbBlas;
        qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        vkCreateQueryPool(raw_device, &qpci, nullptr, &queryPool);
    }

    // Batching creation/compaction of BLAS to allow staying in restricted amount of memory
    std::vector<uint32_t> indices; // Indices of the BLAS to create
    VkDeviceSize batchSize{0};
    VkDeviceSize batchLimit{256'000'000}; // 256 MB
    for (uint32_t idx = 0; idx < nbBlas; idx++) {
        indices.push_back(idx);
        batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
        // Over the limit or last BLAS element
        if (batchSize >= batchLimit || idx == nbBlas - 1) {
            {
                VkCommandBuffer cmdBuf;
                auto command_buffer_info = vkinit::command_buffer_allocate_info(m_cmd_pool);
                VK_CHECK(vkAllocateCommandBuffers(raw_device, &command_buffer_info, &cmdBuf));
                auto command_buffer_begin_info =
                    vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                VK_CHECK(vkBeginCommandBuffer(cmdBuf, &command_buffer_begin_info));
                cmdCreateBlas(cmdBuf, indices, buildAs, scratchAddress, queryPool);
                VK_CHECK(vkEndCommandBuffer(cmdBuf));
                vkinit::submit_command_buffer_and_wait(vk_engine->_graphicsQueue, cmdBuf);
            }

            if (queryPool) {
                VkCommandBuffer cmdBuf;
                auto command_buffer_info = vkinit::command_buffer_allocate_info(m_cmd_pool);
                VK_CHECK(vkAllocateCommandBuffers(raw_device, &command_buffer_info, &cmdBuf));
                cmdCompactBlas(cmdBuf, indices, buildAs, queryPool);
                vkinit::submit_command_buffer_and_wait(vk_engine->_graphicsQueue, cmdBuf);

                // Destroy the non-compacted version
                destroyNonCompacted(indices, buildAs);
            }
            // Reset

            batchSize = 0;
            indices.clear();
        }
    }

    // Logging reduction
    if (queryPool) {
        VkDeviceSize compactSize =
            std::accumulate(buildAs.begin(), buildAs.end(), 0ULL, [](const auto& a, const auto& b) {
                return a + b.sizeInfo.accelerationStructureSize;
            });
        const float fractionSmaller = (asTotalSize == 0) ? 0 : (asTotalSize - compactSize) / float(asTotalSize);
		std::cout << "RT BLAS: reducing from: " << asTotalSize
			<< " to: " << compactSize
			<< " = " << (asTotalSize - compactSize)
			<< " (" << std::fixed << std::setprecision(2) << (fractionSmaller * 100.f) << "% smaller)"
			<< std::endl;
    }

    // Keeping all the created acceleration structures
    for (auto& b : buildAs) {
        m_blas.push_back(std::move(b.as));
    }

    // Clean up
    vkDestroyQueryPool(raw_device, queryPool, nullptr);
    // m_alloc->finalizeAndReleaseStaging();
    vk_engine->destroy_buffer(scratchBuffer);
    // m_cmdPool.deinit();
}

//--------------------------------------------------------------------------------------------------
// Creating the bottom level acceleration structure for all indices of `buildAs` vector.
// The array of BuildAccelerationStructure was created in buildBlas and the vector of
// indices limits the number of BLAS to create at once. This limits the amount of
// memory needed when compacting the BLAS.
void nvvk::RaytracingBuilderKHR::cmdCreateBlas(VkCommandBuffer cmdBuf,
                                               std::vector<uint32_t> indices,
                                               std::vector<BuildAccelerationStructure>& buildAs,
                                               VkDeviceAddress scratchAddress,
                                               VkQueryPool queryPool)
{
    auto vk_engine = m_engine_ptr;
    auto raw_device = vk_engine->_device;
    if (queryPool) // For querying the compaction size
        vkResetQueryPool(raw_device, queryPool, 0, static_cast<uint32_t>(indices.size()));
    uint32_t queryCnt{0};

    for (const auto& idx : indices) {
        // Actual allocation of buffer and acceleration structure.
        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize; // Will be used to allocate memory.
        buildAs[idx].as = create_acceleration(vk_engine, createInfo);
        // NAME_IDX_VK(buildAs[idx].as.accel, idx);
        // NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

        // BuildInfo #2 part
        buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.accel; // Setting where the build lands
        buildAs[idx].buildInfo.scratchData.deviceAddress =
            scratchAddress; // All build are using the same scratch buffer

        // Building the bottom-level-acceleration-structure
        vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildAs[idx].buildInfo, &buildAs[idx].rangeInfo);

        // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
        // is finished before starting the next one.
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuf,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0,
                             1,
                             &barrier,
                             0,
                             nullptr,
                             0,
                             nullptr);

        if (queryPool) {
            // Add a query to find the 'real' amount of memory needed, use for compaction
            vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf,
                                                          1,
                                                          &buildAs[idx].buildInfo.dstAccelerationStructure,
                                                          VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                                                          queryPool,
                                                          queryCnt++);
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Create and replace a new acceleration structure and buffer based on the size retrieved by the
// Query.
void nvvk::RaytracingBuilderKHR::cmdCompactBlas(VkCommandBuffer cmdBuf,
                                                std::vector<uint32_t> indices,
                                                std::vector<BuildAccelerationStructure>& buildAs,
                                                VkQueryPool queryPool)
{
    auto vk_engine = m_engine_ptr;
    auto raw_device = vk_engine->_device;
    uint32_t queryCtn{0};

    // Get the compacted size result back
    std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
    vkGetQueryPoolResults(raw_device,
                          queryPool,
                          0,
                          (uint32_t)compactSizes.size(),
                          compactSizes.size() * sizeof(VkDeviceSize),
                          compactSizes.data(),
                          sizeof(VkDeviceSize),
                          VK_QUERY_RESULT_WAIT_BIT);

    for (auto idx : indices) {
        buildAs[idx].cleanupAS = std::move(buildAs[idx].as);                        // previous AS to destroy
        buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++]; // new reduced size

        // Creating a compact version of the AS
        VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
        asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildAs[idx].as = create_acceleration(vk_engine, asCreateInfo);
        // NAME_IDX_VK(buildAs[idx].as.accel, idx);
        // NAME_IDX_VK(buildAs[idx].as.buffer.buffer, idx);

        // Copy the original BLAS to a compact version
        VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
        copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
        copyInfo.dst = buildAs[idx].as.accel;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
    }
}

//--------------------------------------------------------------------------------------------------
// Destroy all the non-compacted acceleration structures
//
void nvvk::RaytracingBuilderKHR::destroyNonCompacted(std::vector<uint32_t> indices,
                                                     std::vector<BuildAccelerationStructure>& buildAs)
{
	auto vk_engine = m_engine_ptr;
	auto raw_device = vk_engine->_device;
    for (auto& i : indices) {
        /*buildAs[i].cleanupAS.accel.reset();
        buildAs[i].cleanupAS.buffer.reset();*/
        vkDestroyAccelerationStructureKHR(raw_device, buildAs[i].cleanupAS.accel, nullptr);
        vkDestroyBuffer(raw_device, buildAs[i].cleanupAS.buffer.buffer, nullptr);
    }
}

void nvvk::RaytracingBuilderKHR::buildTlas(
    const std::vector<VkAccelerationStructureInstanceKHR>& instances,
    VkBuildAccelerationStructureFlagsKHR flags /*= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR*/,
    bool update /*= false*/)
{
    const auto num_instances = static_cast<uint32_t>(instances.size());
    const auto instance_data = reinterpret_cast<const void*>(instances.data());
    const auto sizeof_instance = sizeof(VkAccelerationStructureInstanceKHR);
    buildTlas(num_instances, const_cast<void*>(instance_data), sizeof_instance, flags, update, false);
}

#ifdef VK_NV_ray_tracing_motion_blur
void nvvk::RaytracingBuilderKHR::buildTlas(
    const std::vector<VkAccelerationStructureMotionInstanceNV>& instances,
    VkBuildAccelerationStructureFlagsKHR flags /*= VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV*/,
    bool update /*= false*/)
{
    const auto num_instances = static_cast<uint32_t>(instances.size());
    const auto instance_data = reinterpret_cast<const void*>(instances.data());
    const auto sizeof_instance = sizeof(VkAccelerationStructureMotionInstanceNV);
    buildTlas(num_instances, const_cast<void*>(instance_data), sizeof_instance, flags, update, true);
}
#endif

//--------------------------------------------------------------------------------------------------
// Low level of Tlas creation - see buildTlas
//
void nvvk::RaytracingBuilderKHR::cmdCreateTlas(VkCommandBuffer cmdBuf,
                                               uint32_t countInstance,
                                               VkDeviceAddress instBufferAddr,
                                               AllocatedBuffer& scratchBuffer,
                                               VkBuildAccelerationStructureFlagsKHR flags,
                                               bool update,
                                               bool motion)
{
    auto vk_engine = m_engine_ptr;
    auto raw_device = vk_engine->_device;
    // Wraps a device pointer to the above uploaded instances.
    VkAccelerationStructureGeometryInstancesDataKHR instancesVk{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instancesVk.data.deviceAddress = instBufferAddr;

    // Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
    VkAccelerationStructureGeometryKHR topASGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.geometry.instances = instancesVk;

    // Find sizes
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.flags = flags;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &topASGeometry;
    buildInfo.mode =
        update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(
        raw_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &countInstance, &sizeInfo);

#ifdef VK_NV_ray_tracing_motion_blur
    VkAccelerationStructureMotionInfoNV motionInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV};
    motionInfo.maxInstances = countInstance;
#endif

    // Create TLAS
    if (update == false) {

        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        createInfo.size = sizeInfo.accelerationStructureSize;
#ifdef VK_NV_ray_tracing_motion_blur
        if (motion) {
            createInfo.createFlags = VK_ACCELERATION_STRUCTURE_CREATE_MOTION_BIT_NV;
            createInfo.pNext = &motionInfo;
        }
#endif

        m_tlas = create_acceleration(vk_engine, createInfo);
        // NAME_VK(m_tlas.accel);
        // NAME_VK(m_tlas.buffer.buffer);
    }

    // Allocate the scratch memory
    scratchBuffer =
        vk_engine->create_buffer(sizeInfo.buildScratchSize,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_MEMORY_USAGE_UNKNOWN);
    vmaSetAllocationName(vk_engine->_allocator, scratchBuffer.allocation, "RT TLAS Scratch Buffer");

    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer.buffer};
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(raw_device, &bufferInfo);
    // NAME_VK(scratchBuffer.buffer);

    // Update build information
    buildInfo.srcAccelerationStructure = update ? m_tlas.accel : VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = m_tlas.accel;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    // Build Offsets info: n instances
    VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{countInstance, 0, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS
    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildOffsetInfo);
}

//--------------------------------------------------------------------------------------------------
// Refit BLAS number blasIdx from updated buffer contents.
//
void nvvk::RaytracingBuilderKHR::updateBlas(uint32_t blasIdx,
                                            BlasInput& blas,
                                            VkBuildAccelerationStructureFlagsKHR flags)
{
    assert(size_t(blasIdx) < m_blas.size());

    auto vk_engine = m_engine_ptr;
    auto raw_device = vk_engine->_device;

    // Preparing all build information, acceleration is filled later
    VkAccelerationStructureBuildGeometryInfoKHR buildInfos{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfos.flags = flags;
    buildInfos.geometryCount = (uint32_t)blas.asGeometry.size();
    buildInfos.pGeometries = blas.asGeometry.data();
    buildInfos.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR; // UPDATE
    buildInfos.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfos.srcAccelerationStructure = m_blas[blasIdx].accel; // UPDATE
    buildInfos.dstAccelerationStructure = m_blas[blasIdx].accel;

    // Find size to build on the device
    std::vector<uint32_t> maxPrimCount(blas.asBuildOffsetInfo.size());
    for (size_t tt = 0; tt < blas.asBuildOffsetInfo.size(); tt++)
        maxPrimCount[tt] = blas.asBuildOffsetInfo[tt].primitiveCount; // Number of primitives/triangles
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(
        raw_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfos, maxPrimCount.data(), &sizeInfo);

    // Allocate the scratch buffer and setting the scratch info
    const auto scratchBuffer =
        vk_engine->create_buffer(sizeInfo.buildScratchSize,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                 VMA_MEMORY_USAGE_UNKNOWN);
    vmaSetAllocationName(vk_engine->_allocator, scratchBuffer.allocation, "RT BLAS Scratch Buffer");
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer = scratchBuffer.buffer;
    buildInfos.scratchData.deviceAddress = vkGetBufferDeviceAddress(raw_device, &bufferInfo);
    // NAME_VK(scratchBuffer.buffer);

    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
    for (size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
        pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

    // Update the instance buffer on the device side and build the TLAS
    VkCommandBuffer cmdBuf;
    {
        auto command_buffer_info = vkinit::command_buffer_allocate_info(m_cmd_pool);
        VK_CHECK(vkAllocateCommandBuffers(raw_device, &command_buffer_info, &cmdBuf));
    }

    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing BLAS being passed and updated in place
    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfos, pBuildOffset.data());

    vkinit::submit_command_buffer_and_wait(vk_engine->_graphicsQueue, cmdBuf);
}
// Build TLAS from an array of VkAccelerationStructureInstanceKHR
// - Use motion=true with VkAccelerationStructureMotionInstanceNV
// - The resulting TLAS will be stored in m_tlas
// - update is to rebuild the TLAS with updated matrices, flag must have the 'allow_update'
void nvvk::RaytracingBuilderKHR::buildTlas(size_t num_instances,
                                           void* instance_data,
                                           size_t sizeof_instance,
                                           VkBuildAccelerationStructureFlagsKHR flags,
                                           bool update,
                                           bool motion)
{
    auto vk_engine = m_engine_ptr;
    auto raw_device = vk_engine->_device;

    // Cannot call buildTlas twice except to update.
    assert(m_tlas.accel == VK_NULL_HANDLE || update);
    uint32_t countInstance = static_cast<uint32_t>(num_instances);

    // Command buffer to create the TLAS
    VkCommandBuffer cmdBuf;
    {
        auto command_buffer_info = vkinit::command_buffer_allocate_info(m_cmd_pool);
        VK_CHECK(vkAllocateCommandBuffers(raw_device, &command_buffer_info, &cmdBuf));
        auto command_buffer_begin_info =
            vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmdBuf, &command_buffer_begin_info));
    }

    // Create a buffer holding the actual instance data (matrices++) for use by the AS builder
    size_t instancesBufferSize = num_instances * sizeof_instance;
    const auto instancesBuffer =
        vk_engine->create_buffer(instancesBufferSize,
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                 VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(vk_engine->_allocator, instancesBuffer.allocation, "RT TLAS Instance Buffer");
    // Copy the instance data into the buffer
    vk_engine->upload_to_vma_allocation(instance_data, instancesBufferSize, instancesBuffer);

    // NAME_VK(instancesBuffer.buffer);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, instancesBuffer.buffer };
    VkDeviceAddress instBufferAddr = vkGetBufferDeviceAddress(raw_device, &bufferInfo);

    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0,
                         1,
                         &barrier,
                         0,
                         nullptr,
                         0,
                         nullptr);

    // Creating the TLAS
    AllocatedBuffer scratchBuffer;
    cmdCreateTlas(cmdBuf, countInstance, instBufferAddr, scratchBuffer, flags, update, motion);

    // Finalizing and destroying temporary data
    VK_CHECK(vkEndCommandBuffer(cmdBuf));
    vkinit::submit_command_buffer_and_wait(vk_engine->_graphicsQueue, cmdBuf);
    vk_engine->destroy_buffer(scratchBuffer);
    vk_engine->destroy_buffer(instancesBuffer);

}