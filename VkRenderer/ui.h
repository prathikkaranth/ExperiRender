#pragma once

#include <SDL.h>
#include <functional>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_types.h>
#include <vk_utils.h>

#include "VkBootstrap.h"
#include "imgui.h"

class VulkanEngine;

class ui {
public:
    static void set_mainpanel_theme();
    static void set_font();
    static void init_imgui(VulkanEngine *engine);

    static void setup_imgui_panel(VulkanEngine *engine);
    static void draw_imgui(const VulkanEngine *engine, VkCommandBuffer cmd, VkImageView targetImageView);
    static void handle_sdl_event(const SDL_Event *event);

private:
    // Docking system functions
    static void setup_dockspace(VulkanEngine *engine);
    static void setup_default_layout(ImGuiID dockspace_id);
    
    // Panel creation functions
    static void create_settings_panel(VulkanEngine *engine);
    static void create_stats_panel(VulkanEngine *engine);
    static void create_debug_panel(VulkanEngine *engine);
    static void create_viewport_panel(VulkanEngine *engine);
};