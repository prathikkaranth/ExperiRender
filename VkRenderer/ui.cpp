#include <spdlog/spdlog.h>
#include <ui.h>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
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

    ImGui_ImplVulkan_CreateFontsTexture();

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

    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Settings", nullptr, window_flags);

    // Scene Information section
    if (ImGui::CollapsingHeader("Scene Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (engine->loadedScenes.empty()) {
            ImGui::Text("No scene loaded");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drag and drop a .gltf or .glb file to load a scene");
        } else {
            ImGui::Text("Loaded scenes:");
            for (const auto& [name, scene] : engine->loadedScenes) {
                ImGui::BulletText("%s", name.c_str());
            }
        }
    }

    ImGui::Checkbox(
        "Ray Tracer mode",
        reinterpret_cast<bool *>(&engine->postProcessor._compositorData)); // Switch between raster and ray tracing

    if (ImGui::CollapsingHeader("Compositor Settings")) {
        ImGui::SliderFloat("Exposure", &engine->postProcessor._compositorData.exposure, 0.1f, 10.0f);

        ImGui::Checkbox("Denoiser", reinterpret_cast<bool *>(&engine->postProcessor._compositorData.useDenoiser));

        if (ImGui::TreeNode("Denoiser Settings")) {
            ImGui::SliderFloat("Sigma", &engine->postProcessor._compositorData.sigma, 0.1f, 10.0f);
            ImGui::SliderFloat("kSigma", &engine->postProcessor._compositorData.kSigma, 0.1f, 10.0f);
            ImGui::SliderFloat("Threshold", &engine->postProcessor._compositorData.threshold, 0.1f, 10.0f);

            ImGui::TreePop();
            ImGui::Spacing();
        }
    }

    if (ImGui::CollapsingHeader("Lighting Settings")) {
        ImGui::ColorEdit3("Ambient Color", &engine->sceneData.ambientColor.x);
        ImGui::ColorEdit3("Sunlight Color", &engine->sceneData.sunlightColor.x);
        ImGui::SliderFloat3("Sunlight Direction", &engine->sceneData.sunlightDirection.x, -10, 10);
        ImGui::SliderFloat("Sunlight Intensity", &engine->sceneData.sunlightDirection.w, 0, 10);

        ImGui::Checkbox("Shadow Maps", reinterpret_cast<bool *>(&engine->sceneData.enableShadows));
        ImGui::Checkbox("SSAO", reinterpret_cast<bool *>(&engine->sceneData.enableSSAO));
        ImGui::Checkbox("PBR", reinterpret_cast<bool *>(&engine->sceneData.enablePBR));
    }
    if (ImGui::CollapsingHeader("Ray Tracer Settings")) {
        ImGui::SliderInt("Max Samples", reinterpret_cast<int *>(&engine->raytracerPipeline.max_samples), 1, 5000);
        ImGui::SliderInt("Ray Depth", reinterpret_cast<int *>(&engine->raytracerPipeline.m_pcRay.depth), 1, 32);
    }

    if (ImGui::CollapsingHeader("SSAO Settings")) {
        ImGui::SliderInt("SSAO Kernel Size", &engine->_ssao.ssaoData.kernelSize, 1, 256);
        ImGui::SliderFloat("SSAO Radius", &engine->_ssao.ssaoData.radius, 0.0001f, 10.f);
        ImGui::SliderFloat("SSAO Bias", &engine->_ssao.ssaoData.bias, 0.001f, 1.055f);
        ImGui::SliderFloat("SSAO Strength", &engine->_ssao.ssaoData.intensity, 0.0f, 10.f);
    }

    if (ImGui::CollapsingHeader("ShadowMap Settings")) {
        ImGui::SliderFloat("Near Plane", &engine->_shadowMap.near_plane, 0.f, 1.f);
        ImGui::SliderFloat("Far Plane", &engine->_shadowMap.far_plane, 2.f, 150.f);
        ImGui::SliderFloat("Left", &engine->_shadowMap.left, -100.f, -1.f);
        ImGui::SliderFloat("Right", &engine->_shadowMap.right, 100.f, 1.f);
        ImGui::SliderFloat("Top", &engine->_shadowMap.top, 100.f, 1.f);
        ImGui::SliderFloat("Bottom", &engine->_shadowMap.bottom, -100.f, -1.f);
    }

    if (ImGui::CollapsingHeader("Debug Tools")) {
        // Dropdown for selecting the visual for debugging
        const char *visuals[] = {"--------", "Shadow Map", "SSAO Map", "GBuffer Position"};
        static const char *current_item = visuals[0];
        if (ImGui::BeginCombo("Image Buffers",
                              current_item)) // The second parameter is the label previewed before opening the combo.
        {
            for (auto &visual: visuals) {
                const bool is_selected = current_item == visual;
                // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(visual, is_selected))
                    current_item = visual;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }

        if (strcmp(current_item, "--------") == 0) {

        } else if (strcmp(current_item, "Shadow Map") == 0) {
            ImGui::Begin("Shadow Map");
            ImGui::Image(reinterpret_cast<ImTextureID>(engine->_shadowMap.shadowMapDescriptorSet), ImVec2(256, 256),
                         ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
            ImGui::End();
        } else if (strcmp(current_item, "SSAO Map") == 0) {
            ImGui::Begin("SSAO Map");
            ImGui::Image(reinterpret_cast<ImTextureID>(engine->_ssao._ssaoDescriptorSet), ImVec2(256, 256),
                         ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
            ImGui::End();
        } else if (strcmp(current_item, "GBuffer Position") == 0) {
            ImGui::Begin("GBuffer Position");
            ImGui::Image(reinterpret_cast<ImTextureID>(engine->gbuffer._gbufferPosOutputDescriptor), ImVec2(256, 256),
                         ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
            ImGui::End();
        }
    }

    ImGui::End();

    constexpr ImGuiWindowFlags transparent_window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("Stats", nullptr, transparent_window_flags);

    int fps = static_cast<int>(std::round(1000.f / engine->stats.frametime));

    ImGui::Text("FPS %i", fps);
    /*ImGui::Text("draw time %f ms", stats.mesh_draw_time);
    ImGui::Text("update time %f ms", stats.scene_update_time);
    ImGui::Text("triangles %i", stats.triangle_count);
    ImGui::Text("draws %i", stats.drawcall_count);*/
    ImGui::Text("Ray Tracing Samples done: %i", engine->raytracerPipeline.m_pcRay.samples_done);

    ImGui::End();

    ImGui::Render();
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
