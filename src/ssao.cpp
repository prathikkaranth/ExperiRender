#include "ssao.h"
#include "vk_engine.h"

void ssao::init_ssao(VulkanEngine &engine) {

	/*_depthMapExtent = { _windowExtent.width, _windowExtent.height };*/
	_ssaoImage = engine.create_image(VkExtent3D{ engine._windowExtent.width, engine._windowExtent.height, 1 }, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	_depthMap = engine.create_image(VkExtent3D{ engine._windowExtent.width, engine._windowExtent.height, 1 }, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	// SSAO 
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_ssaoInputDescriptorLayout = builder.build(engine._device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our SSAO input
	_ssaoInputDescriptors = engine.globalDescriptorAllocator.allocate(engine._device, _ssaoInputDescriptorLayout);

	engine._mainDeletionQueue.push_function([&]() {
		vkDestroyDescriptorSetLayout(engine._device, _ssaoInputDescriptorLayout, nullptr);
		});

	VkPipelineLayoutCreateInfo ssao_layout_info{};
	ssao_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ssao_layout_info.pNext = nullptr;
	ssao_layout_info.pSetLayouts = &_ssaoInputDescriptorLayout;
	ssao_layout_info.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(engine._device, &ssao_layout_info, nullptr, &_ssaoPipelineLayout));

	// layout code
	VkShaderModule ssaoDrawShader;
	if (!vkutil::load_shader_module("ssao.comp.spv", engine._device, &ssaoDrawShader))
	{
		std::cout << "Error when building the compute shader \n";

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

	VK_CHECK(vkCreateComputePipelines(engine._device, VK_NULL_HANDLE, 1, &ssaoPipelineInfo, nullptr, &_ssaoPipeline));

	vkDestroyShaderModule(engine._device, ssaoDrawShader, nullptr);
	engine._mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(engine._device, _ssaoPipelineLayout, nullptr);
		vkDestroyPipeline(engine._device, _ssaoPipeline, nullptr);
		});
}