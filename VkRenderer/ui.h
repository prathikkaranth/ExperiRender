#pragma once

#include <SDL.h>
#include <functional>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <vk_utils.h>

#include "VkBootstrap.h"

class VulkanEngine;

class ui {
public:
    static void set_mainpanel_theme();
    static void set_font();
    static void init_imgui(VulkanEngine *engine);

    static void setup_imgui_panel(VulkanEngine *engine);
    static void draw_imgui(const VulkanEngine *engine, VkCommandBuffer cmd, VkImageView targetImageView);
    static void handle_sdl_event(const SDL_Event *event);
};