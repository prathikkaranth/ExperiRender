/*
 * Copyright (c) 2023, Karthik Karanth
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "vk_loader.h"
#include "raytraceKHR_vk.h"
#include "RenderObject.h"

namespace experirender::vk
{
	nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometryKHR(const RenderObject& mesh);
}