#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <functional>
#include <vk_utils.h>
#include <SDL.h>

#include "VkBootstrap.h"

class VulkanEngine;

class ui {
public:
	void init_imgui(VulkanEngine* engine);
	void setup_imgui_panel(VulkanEngine* engine);
	void draw_imgui(VulkanEngine* engine, VkCommandBuffer cmd, VkImageView targetImageView);
	void handle_sdl_event(SDL_Event* event);
};