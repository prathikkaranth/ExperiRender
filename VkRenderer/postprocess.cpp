#include "postprocess.h"
#include <spdlog/spdlog.h>
#include <vk_images.h>
#include <vk_pipelines.h>

void PostProcessor::init(VulkanEngine* engine) {

	// init compositor data
	_compositorData.useRayTracer = 0;
	_compositorData.exposure = 4.0f;
	_compositorData.sigma = 2.305f;
	_compositorData.kSigma = 0.529f;
	_compositorData.threshold = 0.143f;
	_compositorData.useDenoiser = 0;

	_fullscreenImage = vkutil::create_image(engine, VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	vmaSetAllocationName(engine->_allocator, _fullscreenImage.allocation, "Post Process Image");

	//Create a sampler for the image
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = nullptr;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	
	vkCreateSampler(engine->_device, &samplerInfo, nullptr, &_fullscreenImageSampler);

	// POSTPROCESS
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_postProcessDescriptorSetLayout = builder.build(engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	//allocate a descriptor set for our post process input
	_postProcessDescriptorSet = engine->globalDescriptorAllocator.allocate(engine->_device, _postProcessDescriptorSetLayout);

	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyDescriptorSetLayout(engine->_device, _postProcessDescriptorSetLayout, nullptr);
	});

	VkPipelineLayoutCreateInfo post_process_layout_info{};
	post_process_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	post_process_layout_info.pNext = nullptr;
	post_process_layout_info.setLayoutCount = 1;
	post_process_layout_info.pSetLayouts = &_postProcessDescriptorSetLayout;

	VK_CHECK(vkCreatePipelineLayout(engine->_device, &post_process_layout_info, nullptr, &_postProcessPipelineLayout));

	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyPipelineLayout(engine->_device, _postProcessPipelineLayout, nullptr);
	});

	// layout code
	VkShaderModule fullscreenDrawVertShader;
	if (!vkutil::load_shader_module("Fullscreen.vert.spv", engine->_device, &fullscreenDrawVertShader))
	{
		spdlog::error("Error when building the Fullscreen Vertex shader");
	}
	VkShaderModule fullscreenDrawFragShader;
	if (!vkutil::load_shader_module("Fullscreen.frag.spv", engine->_device, &fullscreenDrawFragShader))
	{
		spdlog::error("Error when building the Fullscreen Fragment shader");
	}

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(fullscreenDrawVertShader, fullscreenDrawFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.add_color_attachment(_fullscreenImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);

	// use the triangle layout we created
	pipelineBuilder._pipelineLayout = _postProcessPipelineLayout;

	// create the pipeline
	_postProcessPipeline = pipelineBuilder.build_pipeline(engine->_device);

	vkDestroyShaderModule(engine->_device, fullscreenDrawFragShader, nullptr);
	vkDestroyShaderModule(engine->_device, fullscreenDrawVertShader, nullptr);

	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyPipeline(engine->_device, _postProcessPipeline, nullptr);
		vkDestroySampler(engine->_device, _fullscreenImageSampler, nullptr);
		vkutil::destroy_image(engine, _fullscreenImage);

	});
}

void PostProcessor::draw(VulkanEngine* engine, VkCommandBuffer cmd) {
	VkClearValue clearVal = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };

	std::array<VkRenderingAttachmentInfo, 1> colorAttachments = {
		vkinit::attachment_info(_fullscreenImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
	};

	// Pass the color attachments directly to rendering_info
	VkRenderingInfo renderInfo = vkinit::rendering_info(
		engine->_windowExtent,
		colorAttachments.data(),      // Pass your color attachments directly
		nullptr                       // No depth attachment
	);

	vkCmdBeginRendering(cmd, &renderInfo);

	_postProcessDescriptorSet = engine->get_current_frame()._frameDescriptors.allocate(
		engine->_device,
		_postProcessDescriptorSetLayout
	);

	// Allocate a new uniform buffer for the scene data
	AllocatedBuffer compositorSceneDataBuffer = engine->create_buffer(
		sizeof(CompositorData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vmaSetAllocationName(engine->_allocator, compositorSceneDataBuffer.allocation, "Compositor Scene Data Buffer");

	// Add it to the deletion queue
	engine->get_current_frame()._deletionQueue.push_function([=] {
		engine->destroy_buffer(compositorSceneDataBuffer);
	});

	// Write the scene data
	CompositorData* compositorUniformBuffer;
	VK_CHECK(vmaMapMemory(engine->_allocator, compositorSceneDataBuffer.allocation, reinterpret_cast<void**>(&compositorUniformBuffer)));
	*compositorUniformBuffer = _compositorData;
	vmaUnmapMemory(engine->_allocator, compositorSceneDataBuffer.allocation);

	{
		DescriptorWriter writer;
		writer.write_image(0, engine->raytracerPipeline._rtOutputImage.imageView, engine->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(1, engine->_drawImage.imageView, engine->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_buffer(2, compositorSceneDataBuffer.buffer, sizeof(CompositorData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.update_set(engine->_device, _postProcessDescriptorSet);
	}

	// Bind pipeline and descriptors
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _postProcessPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _postProcessPipelineLayout, 0, 1, &_postProcessDescriptorSet, 0, nullptr);

	// Set viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(engine->_windowExtent.width);
	viewport.height = static_cast<float>(engine->_windowExtent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = engine->_windowExtent;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdDraw(cmd, 3, 1, 0, 0); // 1 triangle, 3 vertices

	vkCmdEndRendering(cmd);
}
