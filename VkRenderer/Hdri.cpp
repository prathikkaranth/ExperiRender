#include "Hdri.h"

#include <stb_image.h>
#include "vk_images.h"
#include <spdlog/spdlog.h>

void HDRI::load_hdri_to_buffer(VulkanEngine* engine) {

	// Create an array of the six cubemap face filenames
	std::vector<std::string> filenames = {
		"..\\assets\\HDRI\\skybox\\right.jpg", // Positive X
		"..\\assets\\HDRI\\skybox\\left.jpg", // Negative X
		"..\\assets\\HDRI\\skybox\\top.jpg", // Positive Y
		"..\\assets\\HDRI\\skybox\\bottom.jpg", // Negative Y
		"..\\assets\\HDRI\\skybox\\front.jpg", // Positive Z
		"..\\assets\\HDRI\\skybox\\back.jpg"  // Negative Z
	};

	// Prepare a vector to hold the image data for each face
	std::vector<unsigned char*> cubemapData(6, nullptr);
	int width, height, nrComponents;

	// Load each face of the cubemap
	for (int i = 0; i < 6; i++) {
		// You might want to set flip to false for cubemaps
		stbi_set_flip_vertically_on_load(false);

		// Load the image
		cubemapData[i] = stbi_load(filenames[i].c_str(), &width, &height, &nrComponents, 0);

		if (!cubemapData[i]) {
			spdlog::error("Failed to load cubemap face: {}", filenames[i]);
			// Free any already loaded faces
			for (int j = 0; j < i; j++) {
				stbi_image_free(cubemapData[j]);
			}
			return; // Or handle the error as appropriate
		}

		spdlog::info("Loaded cubemap face {}: {}x{} with {} components",
			i, width, height, nrComponents);
	}

	// Use the loaded data...
	_hdriMap = vkutil::create_hdri_image(engine, cubemapData, width, height, nrComponents);

	if (_hdriMap.image == VK_NULL_HANDLE) {
		spdlog::error("Failed to initialize cubemap!");
	}
	else {
		spdlog::info("Cubemap initialized successfully");
	}
	
	// Sampler configs
	VkSamplerCreateInfo sampl2 = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampl2.magFilter = VK_FILTER_LINEAR;
	sampl2.minFilter = VK_FILTER_LINEAR;
	sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampl2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	vkCreateSampler(engine->_device, &sampl2, nullptr, &_hdriMapSampler);

	engine->_mainDeletionQueue.push_function([=]() {
		
		vkDestroySampler(engine->_device, _hdriMapSampler, nullptr);
		vkutil::destroy_image(engine, _hdriMap);
		});
}

void HDRI::init_hdriMap(VulkanEngine* engine) {

	_hdriOutImage = vkutil::create_image(engine, VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	vmaSetAllocationName(engine->_allocator, _hdriOutImage.allocation, "HDRI Output Image");

	// HDRI 
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		hdriMapDescriptorSetLayout = builder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	VkDescriptorSetLayout layouts[] = { hdriMapDescriptorSetLayout, engine->_gpuSceneDataDescriptorLayout };

	VkPipelineLayoutCreateInfo hdri_pipeline_layout_info = vkinit::pipeline_layout_create_info();
	hdri_pipeline_layout_info.setLayoutCount = 2;
	hdri_pipeline_layout_info.pSetLayouts = layouts;
	hdri_pipeline_layout_info.flags = 0;
	hdri_pipeline_layout_info.pushConstantRangeCount = 0;
	hdri_pipeline_layout_info.pPushConstantRanges = nullptr;

	VK_CHECK(vkCreatePipelineLayout(engine->_device, &hdri_pipeline_layout_info, nullptr, &_hdriMapPipelineLayout));

	VkShaderModule skyboxVertShader;
	if (!vkutil::load_shader_module("Skybox.vert.spv", engine->_device, &skyboxVertShader)) {
		spdlog::error("Error when building the Gbuffer fragment shader module");
	}

	VkShaderModule skyboxFragShader;
	if (!vkutil::load_shader_module("Skybox.frag.spv", engine->_device, &skyboxFragShader)) {
		spdlog::error("Error when building the Gbuffer vertex shader module");
	}

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(skyboxVertShader, skyboxFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_ALWAYS);

	//render format
	pipelineBuilder.add_color_attachment(_hdriOutImage.imageFormat, PipelineBuilder::BlendMode::ALPHA_BLEND);

	pipelineBuilder.set_depth_format(VK_FORMAT_D32_SFLOAT);

	// use the triangle layout we created
	pipelineBuilder._pipelineLayout = _hdriMapPipelineLayout;

	// create the pipeline
	_hdriMapPipeline = pipelineBuilder.build_pipeline(engine->_device);

	// destruction
	vkDestroyShaderModule(engine->_device, skyboxVertShader, nullptr);
	vkDestroyShaderModule(engine->_device, skyboxFragShader, nullptr);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, _hdriMapPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(engine->_device, hdriMapDescriptorSetLayout, nullptr);
		vkDestroyPipeline(engine->_device, _hdriMapPipeline, nullptr);
		vkutil::destroy_image(engine, _hdriOutImage);
		});
}

void HDRI::draw_hdriMap(VulkanEngine* engine, VkCommandBuffer cmd) {

	VkClearValue clearVal = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info( _hdriOutImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkClearValue depthClear = { .depthStencil = {1.0f, 0} };
	VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(engine->_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	depthAttachment.clearValue = depthClear;
	VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_windowExtent, &colorAttachment, &depthAttachment);

	vkCmdBeginRendering(cmd, &renderInfo);

	// Allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = engine->create_buffer(
		sizeof(GPUSceneData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vmaSetAllocationName(engine->_allocator, gpuSceneDataBuffer.allocation, "Skybox Scene Data Buffer");

	// Add it to the deletion queue
	engine->get_current_frame()._deletionQueue.push_function([=]() {
		engine->destroy_buffer(gpuSceneDataBuffer);
		});

	// Write the scene data
	GPUSceneData* sceneUniformData;
	VK_CHECK(vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void**>(&sceneUniformData)));
	*sceneUniformData = engine->sceneData;
	vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

	// Create and update descriptor set
	VkDescriptorSet globalDescriptor = engine->get_current_frame()._frameDescriptors.allocate(
		engine->_device,
		engine->_gpuSceneDataDescriptorLayout
	);

	{
		DescriptorWriter writer;
		writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.update_set(engine->_device, globalDescriptor);
	}
	hdriMapDescriptorSet = engine->get_current_frame()._frameDescriptors.allocate(
		engine->_device,
		hdriMapDescriptorSetLayout
	);
	{
		DescriptorWriter writer;
		writer.write_image(0, _hdriMap.imageView, _hdriMapSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.update_set(engine->_device, hdriMapDescriptorSet);
	}

	// Bind pipeline and descriptors
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriMapPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriMapPipelineLayout, 0, 1, &hdriMapDescriptorSet, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,_hdriMapPipelineLayout, 1, 1, &globalDescriptor, 0, nullptr);

	// Set viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)engine->_windowExtent.width;
	viewport.height = (float)engine->_windowExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = engine->_windowExtent;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// For a skybox cube, we often don't need vertex/index buffers
	// as we can generate the cube vertices in the vertex shader
	// However, if you have a cube mesh:

	// Draw a cube without vertex/index buffers (if your shader supports it)
	vkCmdDraw(cmd, 36, 1, 0, 0); // 36 vertices for a cube (6 faces * 2 triangles * 3 vertices)

	// Or if you're using a mesh:
	// vkCmdBindIndexBuffer(cmd, cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	// vkCmdDrawIndexed(cmd, cubeIndexCount, 1, 0, 0, 0);

	vkCmdEndRendering(cmd);
}
