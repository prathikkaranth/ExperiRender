#include<ui.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"
#include "vk_engine.h"

void ui::set_mainpanel_theme() {
	ImGuiStyle* style = &ImGui::GetStyle();
	style->WindowRounding = 5.3f;
	style->GrabRounding = style->FrameRounding = 2.3f;
	style->ScrollbarRounding = 5.0f;
	style->FrameBorderSize = 1.0f;
	style->ItemSpacing.y = 6.5f;

	style->Colors[ImGuiCol_Text] = { 0.73333335f, 0.73333335f, 0.73333335f, 1.00f };
	style->Colors[ImGuiCol_TextDisabled] = { 0.34509805f, 0.34509805f, 0.34509805f, 1.00f };
	style->Colors[ImGuiCol_WindowBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.94f };
	style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };
	style->Colors[ImGuiCol_PopupBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.94f };
	style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
	style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };
	style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.54f };
	style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
	style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
	style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 0.51f };
	style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.80f };
	style->Colors[ImGuiCol_ScrollbarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.60f };
	style->Colors[ImGuiCol_ScrollbarGrab] = { 0.21960786f, 0.30980393f, 0.41960788f, 0.51f };
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
	// style->Colors[ImGuiCol_ComboBg]               = {0.1f, 0.1f, 0.1f, 0.99f};
	style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
	style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
	style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
	style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
	style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
	style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
	style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
	style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
	style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
	style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
	style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
	style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
	style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
	style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
	style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
	style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };
}

void ui::init_imgui(VulkanEngine* engine) {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { 
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
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
	init_info.CheckVkResultFn = [](VkResult res) { VK_CHECK(res); };

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &engine->_swapchainImageFormat;


	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	auto& imgui_io = ImGui::GetIO();
	imgui_io.Fonts->AddFontFromFileTTF("..\\assets\\fonts\\Lexend\\Lexend-VariableFont_wght.ttf", 15.0f);
	ImGui_ImplVulkan_CreateFontsTexture();

	set_mainpanel_theme();

	// For Drawing shadow depth map on ImGui
	engine->_shadowMap.shadowMapDescriptorSet = ImGui_ImplVulkan_AddTexture(engine->_shadowMap._shadowDepthMapSampler, engine->_shadowMap._depthShadowMap.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);

	// For Drawing SSAO on ImGui
	engine->_ssao._ssaoDescriptorSet = ImGui_ImplVulkan_AddTexture(engine->_ssao._ssaoSampler, engine->_ssao._ssaoImageBlurred.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// For Drawing Gbuffer Position on ImGui
	engine->_gbufferPosOutputDescriptor = ImGui_ImplVulkan_AddTexture(engine->_gbufferSampler, engine->_gbufferPosition.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// add the destroy the imgui created structures
	engine->_mainDeletionQueue.push_function([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(engine->_device, imguiPool, nullptr);
		});
}

void ui::setup_imgui_panel(VulkanEngine* engine) {
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("Settings", nullptr, window_flags);

	ImGui::Checkbox("Ray Tracer mode", &engine->useRaytracer); // Switch between raster and ray tracing

	if (ImGui::CollapsingHeader("Lighting Settings")) {
		ImGui::ColorEdit3("Ambient Color", &engine->sceneData.ambientColor.x);
		ImGui::ColorEdit3("Sunlight Color", &engine->sceneData.sunlightColor.x);
		ImGui::SliderFloat3("Sunlight Direction", &engine->sceneData.sunlightDirection.x, -50, 50);
		ImGui::SliderFloat("Sunlight Intensity", &engine->sceneData.sunlightDirection.w, 0, 10);

		ImGui::Checkbox("Shadow Maps", reinterpret_cast<bool*>(&engine->sceneData.enableShadows));
		ImGui::Checkbox("SSAO", reinterpret_cast<bool*>(&engine->sceneData.enableSSAO));
	}

	if (ImGui::CollapsingHeader("Ray Tracer Settings")) {
		ImGui::SliderInt("Max Samples", reinterpret_cast<int*>(&engine->raytracerPipeline.max_samples), 100, 5000);
		ImGui::SliderInt("Ray Step Size", reinterpret_cast<int*>(&engine->raytracerPipeline.m_pcRay.depth), 2, 32);

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
		const char* visuals[] = { "--------", "Shadow Map", "SSAO Map", "GBuffer Position" };
		static const char* current_item = visuals[0];
		if (ImGui::BeginCombo("Image Buffers", current_item)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(visuals); n++)
			{
				bool is_selected = (current_item == visuals[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(visuals[n], is_selected))
					current_item = visuals[n];
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		if (strcmp(current_item, "--------") == 0)
		{

		}
		else if (strcmp(current_item, "Shadow Map") == 0)
		{
			ImGui::Begin("Shadow Map");
			ImGui::Image((ImTextureID)engine->_shadowMap.shadowMapDescriptorSet, ImVec2(256, 256), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::End();
		}
		else if (strcmp(current_item, "SSAO Map") == 0)
		{
			ImGui::Begin("SSAO Map");
			ImGui::Image((ImTextureID)engine->_ssao._ssaoDescriptorSet, ImVec2(256, 256), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::End();
		}
		else if (strcmp(current_item, "GBuffer Position") == 0)
		{
			ImGui::Begin("GBuffer Position");
			ImGui::Image((ImTextureID)engine->_gbufferPosOutputDescriptor, ImVec2(256, 256), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::End();
		}
	}

	ImGui::End();

	constexpr ImGuiWindowFlags transparent_window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar;
	ImGui::Begin("Stats", nullptr, transparent_window_flags);

	float fps = 1.f / engine->stats.frametime;
	fps = std::round(fps * 1000);

	ImGui::Text("FPS %f", fps);
	/*ImGui::Text("draw time %f ms", stats.mesh_draw_time);
	ImGui::Text("update time %f ms", stats.scene_update_time);
	ImGui::Text("triangles %i", stats.triangle_count);
	ImGui::Text("draws %i", stats.drawcall_count);*/
	ImGui::Text("Ray Tracing Samples done: %i", engine->raytracerPipeline.m_pcRay.samples_done);

	ImGui::End();

	ImGui::Render();
}

void ui::draw_imgui(VulkanEngine* engine, VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void ui::handle_sdl_event(SDL_Event* event) {
	ImGui_ImplSDL2_ProcessEvent(event);
}
