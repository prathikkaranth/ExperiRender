
#include "vk_mem_alloc.h"
#include <spdlog/spdlog.h>
#include <vk_images.h>

#include "ssao.h"
#include "vk_engine.h"

void ssao::init_ssao(VulkanEngine* engine) {

	_depthMapExtent = { engine->_windowExtent.width, engine->_windowExtent.height };
	_ssaoImage = vkutil::create_image(engine, VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	vmaSetAllocationName(engine->_allocator, _ssaoImage.allocation, "SSAO Image");
	_depthMap = vkutil::create_image(engine, VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vmaSetAllocationName(engine->_allocator, _depthMap.allocation, "Depth Map");

	// SSAO 
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_ssaoInputDescriptorLayout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our SSAO input
	_ssaoInputDescriptors = engine->globalDescriptorAllocator.allocate(engine->_device, _ssaoInputDescriptorLayout);

	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyDescriptorSetLayout(engine->_device, _ssaoInputDescriptorLayout, nullptr);
		});

	VkPipelineLayoutCreateInfo ssao_layout_info{};
	ssao_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ssao_layout_info.pNext = nullptr;
	ssao_layout_info.pSetLayouts = &_ssaoInputDescriptorLayout;
	ssao_layout_info.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(engine->_device, &ssao_layout_info, nullptr, &_ssaoPipelineLayout));

	// layout code
	VkShaderModule ssaoDrawShader;
	if (!vkutil::load_shader_module("SSAO.comp.spv", engine->_device, &ssaoDrawShader))
	{
		spdlog::error ("Error when building the ssao compute shader");

	}

	VkPipelineShaderStageCreateInfo ssaoStageInfo{};
	ssaoStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ssaoStageInfo.pNext = nullptr;
	ssaoStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	ssaoStageInfo.module = ssaoDrawShader;
	ssaoStageInfo.pName = "main";

	VkComputePipelineCreateInfo ssaoPipelineInfo{};
	ssaoPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	ssaoPipelineInfo.pNext = nullptr;
	ssaoPipelineInfo.layout = _ssaoPipelineLayout;
	ssaoPipelineInfo.stage = ssaoStageInfo;

	VK_CHECK(vkCreateComputePipelines(engine->_device, VK_NULL_HANDLE, 1, &ssaoPipelineInfo, nullptr, &_ssaoPipeline));

	vkDestroyShaderModule(engine->_device, ssaoDrawShader, nullptr);
	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyPipelineLayout(engine->_device, _ssaoPipelineLayout, nullptr);
		vkDestroyPipeline(engine->_device, _ssaoPipeline, nullptr);
		vkutil::destroy_image(engine, _ssaoImage);
		vkutil::destroy_image(engine, _depthMap);
	});
}

void ssao::init_ssao_blur(VulkanEngine* engine) {

	_ssaoImageBlurred = vkutil::create_image(engine, VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	vmaSetAllocationName(engine->_allocator, _ssaoImageBlurred.allocation, "SSAO Blurred Image");

	// SSAO Blur
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_ssaoBlurInputDescriptorLayout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our SSAO Blur input
	_ssaoBlurInputDescriptors = engine->globalDescriptorAllocator.allocate(engine->_device, _ssaoBlurInputDescriptorLayout);

	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyDescriptorSetLayout(engine->_device, _ssaoBlurInputDescriptorLayout, nullptr);
		});

	VkPipelineLayoutCreateInfo ssao_blur_layout_info{};
	ssao_blur_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ssao_blur_layout_info.pNext = nullptr;
	ssao_blur_layout_info.pSetLayouts = &_ssaoBlurInputDescriptorLayout;
	ssao_blur_layout_info.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(engine->_device, &ssao_blur_layout_info, nullptr, &_ssaoBlurPipelineLayout));

	// layout code
	VkShaderModule ssaoBlurDrawShader;
	if (!vkutil::load_shader_module("SSAO_BLUR.comp.spv", engine->_device, &ssaoBlurDrawShader))
	{
		spdlog::error("Error when building the ssao blur compute shader");

	}

	VkPipelineShaderStageCreateInfo ssaoBlurStageInfo{};
	ssaoBlurStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ssaoBlurStageInfo.pNext = nullptr;
	ssaoBlurStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	ssaoBlurStageInfo.module = ssaoBlurDrawShader;
	ssaoBlurStageInfo.pName = "main";

	VkComputePipelineCreateInfo ssaoBlurPipelineInfo{};
	ssaoBlurPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	ssaoBlurPipelineInfo.pNext = nullptr;
	ssaoBlurPipelineInfo.layout = _ssaoBlurPipelineLayout;
	ssaoBlurPipelineInfo.stage = ssaoBlurStageInfo;

	VK_CHECK(vkCreateComputePipelines(engine->_device, VK_NULL_HANDLE, 1, &ssaoBlurPipelineInfo, nullptr, &_ssaoBlurPipeline));

	VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;

	vkCreateSampler(engine->_device, &sampl, nullptr, &_ssaoSampler);

	vkDestroyShaderModule(engine->_device, ssaoBlurDrawShader, nullptr);
	engine->_mainDeletionQueue.push_function([=] {
		vkDestroyPipelineLayout(engine->_device, _ssaoBlurPipelineLayout, nullptr);
		vkDestroyPipeline(engine->_device, _ssaoBlurPipeline, nullptr);
		vkDestroySampler(engine->_device, _ssaoSampler, nullptr);
		vkutil::destroy_image(engine, _ssaoImageBlurred);
		});
}

void ssao::draw_ssao(VulkanEngine* engine, VkCommandBuffer cmd) const {
	//allocate a new uniform buffer for the scene data
	AllocatedBuffer ssaoSceneDataBuffer = engine->create_buffer(sizeof(SSAOSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vmaSetAllocationName(engine->_allocator, ssaoSceneDataBuffer.allocation, "SSAO Scene Data Buffer");

	//add it to the deletion queue of this frame so it gets deleted once its been used
	engine->get_current_frame()._deletionQueue.push_function([=] {
		engine->destroy_buffer(ssaoSceneDataBuffer);
		});

	//write the buffer
	SSAOSceneData* sceneUniformData;
	VK_CHECK(vmaMapMemory(engine->_allocator, ssaoSceneDataBuffer.allocation, reinterpret_cast<void**>(& sceneUniformData)));
	*sceneUniformData = ssaoData;
	vmaUnmapMemory(engine->_allocator, ssaoSceneDataBuffer.allocation);

	DescriptorWriter ssao_writer;
	ssao_writer.write_buffer(0, ssaoSceneDataBuffer.buffer, sizeof(SSAOSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	ssao_writer.write_image(1, _depthMap.imageView, engine->_defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	ssao_writer.write_image(2, engine->gbuffer.getGbufferPosInfo().imageView, engine->_defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	ssao_writer.write_image(3, engine->gbuffer.getGbufferNormInfo().imageView, engine->_defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	ssao_writer.write_image(4, engine->_ssaoNoiseImage.imageView, engine->_defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	ssao_writer.write_image(5, _ssaoImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	ssao_writer.update_set(engine->_device, _ssaoInputDescriptors);

	// bind the SSAO pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoPipeline);
	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoPipelineLayout, 0, 1, &_ssaoInputDescriptors, 0, nullptr);
	// execute the compute pipeline dispatch. We are using 32x32 work group size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(engine->_drawExtent.width / 32.0), std::ceil(engine->_drawExtent.height / 32.0), 1);
}

void ssao::draw_ssao_blur(const VulkanEngine* engine, VkCommandBuffer cmd) const {
	DescriptorWriter ssao_blur_writer;
	ssao_blur_writer.write_image(0, _ssaoImage.imageView, engine->_defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	ssao_blur_writer.write_image(1, _ssaoImageBlurred.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	ssao_blur_writer.update_set(engine->_device, _ssaoBlurInputDescriptors);

	// bind the SSAO blur pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoBlurPipeline);
	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoBlurPipelineLayout, 0, 1, &_ssaoBlurInputDescriptors, 0, nullptr);
	// execute the compute pipeline dispatch. We are using 32x32 work group size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(engine->_drawExtent.width / 32.0), std::ceil(engine->_drawExtent.height / 32.0), 1);
}
