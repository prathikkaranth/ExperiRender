/*
 * Copyright (c) 2023, Karthik Karanth
 *
 * SPDX-License-Identifier: MIT
 */

#include "VulkanGeometryKHR.h"
#include "raytraceKHR_vk.h"

namespace experirender::vk
{
	nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometryKHR(const RenderObject& mesh) {
		const VkDeviceAddress vertex_address = mesh.vertexBufferAddress;
		const VkDeviceAddress index_address = mesh.indexBufferAddress;

		const uint32_t max_primitive_count = mesh.indexCount / 3;

		// Describe buffer as array of VertexObj
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
			.pNext = nullptr,
			.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
			.vertexData = {.deviceAddress = vertex_address},
			.vertexStride = sizeof(Vertex),
			.maxVertex = mesh.vertexCount,
			.indexType = VK_INDEX_TYPE_UINT32,
			.indexData = {.deviceAddress = index_address},
			.transformData = {}, // null implies identity transform
		};

		// Identify the above data as containing opaque triangles
		VkAccelerationStructureGeometryKHR as_geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		as_geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		as_geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		as_geom.geometry.triangles = triangles;

		// The entire array will be used to build the BLAS
		VkAccelerationStructureBuildRangeInfoKHR offset{
			.primitiveCount = max_primitive_count,
			.primitiveOffset = mesh.firstIndex * sizeof(std::uint32_t),
			.firstVertex = 0,
			.transformOffset = 0,
		};

		// Our blas is made from only one geometry, but could be made of many geometries
		nvvk::RaytracingBuilderKHR::BlasInput blas_input{
			.asGeometry = {as_geom},
			.asBuildOffsetInfo = {offset},
		};

		return blas_input;
	}
}