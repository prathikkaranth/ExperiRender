/*
 * Copyright (c) 2023, Karthik Karanth
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "RenderObject.h"
#include "raytraceKHR_vk.h"
#include "vk_loader.h"

namespace experirender::vk {
    nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometryKHR(const RenderObject &mesh);
}