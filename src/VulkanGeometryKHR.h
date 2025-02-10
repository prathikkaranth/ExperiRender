/*
 * Copyright (c) 2023, Karthik Karanth
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "vk_loader.h"
#include "raytraceKHR_vk.h"

namespace experirender::vk
{
	nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometryKHR(const std::shared_ptr<MeshAsset>& mesh);
}