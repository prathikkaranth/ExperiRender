#include <spdlog/spdlog.h>
#include <ui.h>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "vk_engine.h"

void ui::set_mainpanel_theme() {
    ImGuiStyle *style = &ImGui::GetStyle();
    ImVec4 *colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style->ChildRounding = 4.0f;
    style->FrameBorderSize = 1.0f;
    style->FrameRounding = 2.0f;
    style->GrabMinSize = 7.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize = 13.0f;
    style->TabBorderSize = 1.0f;
    style->TabRounding = 0.0f;
    style->WindowRounding = 4.0f;
}

void ui::set_font() {
    const ImGuiIO &io = ImGui::GetIO();
    // const char* fontPath = "..\\assets\\fonts\\Roboto_Mono\\RobotoMono-VariableFont_wght.ttf";
    const std::string assetsDir = "../assets";
    std::string fontPath =
        (std::filesystem::path(assetsDir) / "fonts" / "Roboto_Mono" / "RobotoMono-VariableFont_wght.ttf").string();

    // Check if the font file exists
    if (std::filesystem::exists(fontPath)) {
        constexpr float fontSize = 15.0f;
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        spdlog::info("Loaded font: {}", fontPath);
    } else {
        spdlog::info("Font file not found! Using default font instead.");
        io.Fonts->AddFontDefault();
    }

    // Rebuild fonts to apply changes
    io.Fonts->Build();
}

void ui::init_imgui(VulkanEngine *engine) {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    const VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                               {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                               {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(engine->_device, &pool_info, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    // Enable docking and multi-viewport features
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // this initializes imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(engine->_window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = engine->_instance;
    init_info.PhysicalDevice = engine->_chosenGPU;
    init_info.Device = engine->_device;
    init_info.Queue = engine->_graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.CheckVkResultFn = [](const VkResult res) { VK_CHECK(res); };

    // dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &engine->_swapchainImageFormat;


    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    set_font();

    set_mainpanel_theme();

    // For Drawing shadow depth map on ImGui
    engine->_shadowMap.shadowMapDescriptorSet = ImGui_ImplVulkan_AddTexture(
        engine->_shadowMap._shadowDepthMapSampler, engine->_shadowMap._depthShadowMap.imageView,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);

    // For Drawing SSAO on ImGui
    engine->_ssao._ssaoDescriptorSet =
        ImGui_ImplVulkan_AddTexture(engine->_ssao._ssaoSampler, engine->_ssao._ssaoImageBlurred.imageView,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // For Drawing Gbuffer Position on ImGui
    engine->gbuffer._gbufferPosOutputDescriptor =
        ImGui_ImplVulkan_AddTexture(engine->gbuffer.getGbufferSampler(), engine->gbuffer.getGbufferPosInfo().imageView,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // For Drawing final rendered output in viewport
    engine->_viewportTextureDescriptorSet = ImGui_ImplVulkan_AddTexture(
        engine->postProcessor.getFullscreenImageSampler(), engine->postProcessor._fullscreenImage.imageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // add the destroy the imgui created structures
    engine->_mainDeletionQueue.push_function([=] {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(engine->_device, imguiPool, nullptr);
    });
}

void ui::setup_imgui_panel(VulkanEngine *engine) {
    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();

    // Setup main dockspace
    setup_dockspace(engine);

    // Create docked panels
    create_settings_panel(engine);
    create_stats_panel(engine);
    create_debug_panel(engine);
    create_viewport_panel(engine);

    ImGui::Render();

    // Update and Render additional Platform Windows
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ui::setup_dockspace(VulkanEngine *engine) {
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        // Setup default layout on first run
        static bool first_time = true;
        if (first_time) {
            first_time = false;
            setup_default_layout(dockspace_id);
        }
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr, nullptr);
            ImGui::MenuItem("Stats", nullptr, nullptr);
            ImGui::MenuItem("Debug Tools", nullptr, nullptr);
            ImGui::MenuItem("Viewport", nullptr, nullptr);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void ui::setup_default_layout(ImGuiID dockspace_id) {
    // Clear any existing layout
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

    // Split the dockspace into sections
    auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
    auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
    auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

    // Dock windows to sections
    ImGui::DockBuilderDockWindow("Settings", dock_id_left);
    ImGui::DockBuilderDockWindow("Debug Tools", dock_id_right);
    ImGui::DockBuilderDockWindow("Stats", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Viewport", dockspace_id); // Center area

    ImGui::DockBuilderFinish(dockspace_id);
}

void ui::create_settings_panel(VulkanEngine *engine) {
    ImGui::Begin("Settings");

    // Scene Information section
    if (ImGui::CollapsingHeader("Scene Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (engine->loadedScenes.empty()) {
            ImGui::Text("No scene loaded");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drag and drop a .gltf or .glb file to load a scene");
        } else {
            ImGui::Text("Loaded scenes:");
            for (const auto &[name, scene]: engine->loadedScenes) {
                ImGui::BulletText("%s", name.c_str());
            }
        }
    }

    ImGui::Checkbox(
        "Ray Tracer mode",
        reinterpret_cast<bool *>(&engine->postProcessor._compositorData)); // Switch between raster and ray tracing

    if (ImGui::CollapsingHeader("Compositor Settings")) {
        ImGui::SliderFloat("Exposure", &engine->postProcessor._compositorData.exposure, 0.1f, 10.0f);
        ImGui::Checkbox("Show Grid Helper", reinterpret_cast<bool *>(&engine->postProcessor._compositorData.showGrid));
    }

    if (ImGui::CollapsingHeader("Lighting Settings")) {
        ImGui::ColorEdit3("Ambient Color", &engine->sceneData.ambientColor.x);
        ImGui::ColorEdit3("Sunlight Color", &engine->sceneData.sunlightColor.x);
        ImGui::SliderFloat3("Sunlight Direction", &engine->sceneData.sunlightDirection.x, -10, 10);
        ImGui::SliderFloat("Sunlight Intensity", &engine->sceneData.sunlightDirection.w, 0, 10);

        ImGui::Separator();
        ImGui::Text("Point Light");
        ImGui::ColorEdit3("Point Light Color", &engine->sceneData.pointLightColor.x);
        ImGui::SliderFloat3("Point Light Position", &engine->sceneData.pointLightPosition.x, -10, 10);
        ImGui::SliderFloat("Point Light Intensity", &engine->sceneData.pointLightColor.w, 0, 10);
        ImGui::SliderFloat("Point Light Range", &engine->sceneData.pointLightPosition.w, 0.1f, 20.0f);

        ImGui::Separator();
        ImGui::Checkbox("Shadow Maps", reinterpret_cast<bool *>(&engine->sceneData.enableShadows));
        ImGui::Checkbox("SSAO", reinterpret_cast<bool *>(&engine->sceneData.enableSSAO));
        ImGui::Checkbox("PBR", reinterpret_cast<bool *>(&engine->sceneData.enablePBR));
    }
    if (ImGui::CollapsingHeader("Ray Tracer Settings")) {
        ImGui::SliderInt("Max Samples", reinterpret_cast<int *>(&engine->raytracerPipeline.max_samples), 1, 20000);
        ImGui::SliderInt("Ray Depth", reinterpret_cast<int *>(&engine->raytracerPipeline.m_pcRay.depth), 1, 32);

        // Microfacet BRDF sampling toggle
        bool previousMicrofacet = engine->raytracerPipeline.useMicrofacetSampling;
        if (ImGui::Checkbox("Use Microfacet BRDF Sampling", &engine->raytracerPipeline.useMicrofacetSampling)) {
            // Setting changed, samples will be reset automatically in rtSampleUpdates
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable GGX importance sampling for more accurate BRDF evaluation.\nDisabling uses "
                              "cosine-weighted hemisphere sampling.");
        }
    }

    if (ImGui::CollapsingHeader("SSAO Settings")) {
        ImGui::SliderInt("SSAO Kernel Size", &engine->_ssao.ssaoData.kernelSize, 1, 256);
        ImGui::SliderFloat("SSAO Radius", &engine->_ssao.ssaoData.radius, 0.0001f, 10.f);
        ImGui::SliderFloat("SSAO Bias", &engine->_ssao.ssaoData.bias, 0.001f, 1.055f);
        ImGui::SliderFloat("SSAO Strength", &engine->_ssao.ssaoData.intensity, 0.0f, 10.f);
    }

    if (ImGui::CollapsingHeader("Camera Settings")) {
        ImGui::Text("Press F to toggle FPS camera movement");
        ImGui::SliderFloat("Move Sensitivity", &engine->mainCamera.moveSensitivity, 0.001f, 5.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("ShadowMap Settings")) {
        ImGui::SliderFloat("Near Plane", &engine->_shadowMap.near_plane, 0.f, 1.f);
        ImGui::SliderFloat("Far Plane", &engine->_shadowMap.far_plane, 2.f, 650.f);
        ImGui::SliderFloat("Left", &engine->_shadowMap.left, -100.f, -1.f);
        ImGui::SliderFloat("Right", &engine->_shadowMap.right, 100.f, 1.f);
        ImGui::SliderFloat("Top", &engine->_shadowMap.top, 100.f, 1.f);
        ImGui::SliderFloat("Bottom", &engine->_shadowMap.bottom, -100.f, -1.f);
    }

    ImGui::End();
}

void ui::create_stats_panel(VulkanEngine *engine) {
    ImGui::Begin("Stats");

    int fps = static_cast<int>(std::round(1000.f / engine->stats.frametime));

    ImGui::Text("FPS: %i", fps);
    ImGui::Text("Frame time: %.2f ms", engine->stats.frametime);
    ImGui::Text("Ray Tracing Samples: %i", engine->raytracerPipeline.m_pcRay.samples_done);

    // Camera position display
    ImGui::Separator();
    ImGui::Text("Camera Position:");
    ImGui::Text("  X: %.3f", engine->mainCamera.position.x);
    ImGui::Text("  Y: %.3f", engine->mainCamera.position.y);
    ImGui::Text("  Z: %.3f", engine->mainCamera.position.z);
    ImGui::Text("Pitch: %.3f", engine->mainCamera.pitch);
    ImGui::Text("Yaw: %.3f", engine->mainCamera.yaw);

    if (ImGui::CollapsingHeader("Detailed Stats")) {
        ImGui::Text("Triangles: %i", engine->stats.triangle_count);
        ImGui::Text("Draw calls: %i", engine->stats.drawcall_count);
        ImGui::Text("Scene update time: %.2f ms", engine->stats.scene_update_time);
        ImGui::Text("Mesh draw time: %.2f ms", engine->stats.mesh_draw_time);
    }

    ImGui::End();
}

void ui::create_debug_panel(VulkanEngine *engine) {
    ImGui::Begin("Debug Tools");

    // Dropdown for selecting the visual for debugging
    const char *visuals[] = {"--------", "Shadow Map", "SSAO Map", "GBuffer Position"};
    static const char *current_item = visuals[0];
    if (ImGui::BeginCombo("Image Buffers", current_item)) {
        for (auto &visual: visuals) {
            const bool is_selected = current_item == visual;
            if (ImGui::Selectable(visual, is_selected))
                current_item = visual;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Display selected debug image
    if (strcmp(current_item, "Shadow Map") == 0) {
        ImGui::Text("Shadow Map:");
        ImGui::Image(reinterpret_cast<ImTextureID>(engine->_shadowMap.shadowMapDescriptorSet), ImVec2(256, 256));
    } else if (strcmp(current_item, "SSAO Map") == 0) {
        ImGui::Text("SSAO Map:");
        ImGui::Image(reinterpret_cast<ImTextureID>(engine->_ssao._ssaoDescriptorSet), ImVec2(256, 256));
    } else if (strcmp(current_item, "GBuffer Position") == 0) {
        ImGui::Text("GBuffer Position:");
        ImGui::Image(reinterpret_cast<ImTextureID>(engine->gbuffer._gbufferPosOutputDescriptor), ImVec2(256, 256));
    }

    ImGui::End();
}

void ui::create_viewport_panel(VulkanEngine *engine) {
    ImGui::Begin("Viewport");

    // Reserve space for info text at the bottom
    float infoHeight = ImGui::GetTextLineHeightWithSpacing();

    // Get the available content region minus info space
    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    availableRegion.y -= infoHeight;

    if (engine->_viewportTextureDescriptorSet != VK_NULL_HANDLE && availableRegion.x > 0 && availableRegion.y > 0) {
        // Calculate the actual rendered image size
        float imageWidth = static_cast<float>(engine->_drawExtent.width);
        float imageHeight = static_cast<float>(engine->_drawExtent.height);

        // Calculate aspect ratios
        float imageAspect = imageWidth / imageHeight;
        float panelAspect = availableRegion.x / availableRegion.y;

        // Calculate display size maintaining aspect ratio
        ImVec2 displaySize;
        if (imageAspect > panelAspect) {
            // Image is wider than panel - fit to width
            displaySize.x = availableRegion.x;
            displaySize.y = availableRegion.x / imageAspect;
        } else {
            // Image is taller than panel - fit to height
            displaySize.y = availableRegion.y;
            displaySize.x = availableRegion.y * imageAspect;
        }

        // Center the image horizontally, top-align vertically
        float centerOffsetX = (availableRegion.x - displaySize.x) * 0.5f;

        if (centerOffsetX > 0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerOffsetX);
        }

        // Display the rendered scene texture with proper scaling
        ImGui::Image(reinterpret_cast<ImTextureID>(engine->_viewportTextureDescriptorSet), displaySize);

        // Display info below the image
        ImGui::Text("Render: %dx%d (%.1fx scale)", engine->_drawExtent.width, engine->_drawExtent.height,
                    engine->renderScale);
    } else {
        // Fallback display when texture isn't ready
        ImGui::Text("Main Render Viewport");
        ImGui::Text("Resolution: %dx%d", engine->_windowExtent.width, engine->_windowExtent.height);
        ImGui::Text("Viewport size: %.0fx%.0f", availableRegion.x, availableRegion.y);
    }

    ImGui::End();
}

void ui::draw_imgui(const VulkanEngine *engine, VkCommandBuffer cmd, VkImageView targetImageView) {
    VkRenderingAttachmentInfo colorAttachment =
        vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void ui::handle_sdl_event(const SDL_Event *event) { ImGui_ImplSDL2_ProcessEvent(event); }
