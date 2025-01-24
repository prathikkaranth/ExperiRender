
#include "vk_mem_alloc.h"

#include "shadowmap.h"
#include "vk_engine.h"

void shadowMap::init_lightSpaceMatrix(VulkanEngine* engine) {
	float near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(3, 5, -5.5), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	engine->sceneData.lightSpaceMatrix = lightProjection * lightView;	
}

void shadowMap::init_depthShadowMap(VulkanEngine* engine) {

	_depthShadowMap = engine->create_image(VkExtent3D{ engine->_windowExtent.width, engine->_windowExtent.height, 1 }, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { engine->_gpuSceneDataDescriptorLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 1;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &_depthShadowMapPipelineLayout));

	VkShaderModule shadowDepthMapFragShader;
	if (!vkutil::load_shader_module("ShadowDepthMap.frag.spv", engine->_device, &shadowDepthMapFragShader)) {
		throw std::runtime_error("Error when building the ShadowDepthMap fragment shader module");
	}

	VkShaderModule shadowDepthMapVertShader;
	if (!vkutil::load_shader_module("ShadowDepthMap.vert.spv", engine->_device, &shadowDepthMapVertShader)) {
		throw std::runtime_error("Error when building the ShadowDepthMap fragment shader module");
	}

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(shadowDepthMapVertShader, shadowDepthMapFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	pipelineBuilder.set_depth_format(_depthShadowMap.imageFormat);

	// use the triangle layout we created
	pipelineBuilder._pipelineLayout = _depthShadowMapPipelineLayout;

	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// create the pipeline
	_depthShadowMapPipeline = pipelineBuilder.build_pipeline(engine->_device);

	vkDestroyShaderModule(engine->_device, shadowDepthMapFragShader, nullptr);
	vkDestroyShaderModule(engine->_device, shadowDepthMapVertShader, nullptr);

	engine->_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(engine->_device, _depthShadowMapPipelineLayout, nullptr);
		vkDestroyPipeline(engine->_device, _depthShadowMapPipeline, nullptr);
	});

}

void shadowMap::draw_depthShadowMap(VulkanEngine* engine, VkCommandBuffer cmd) {

	VkClearValue clearVal = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
	//begin a render pass connected to our draw image
	VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthShadowMap.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	/*std::array<VkRenderingAttachmentInfo, 2> colorAttachments = {
		vkinit::attachment_info(_depthShadowMap.imageView, &clearVal, VK_IMAGE_LAYOUT_GENERAL),
	};*/

	VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_drawExtent, nullptr/*color attachments*/, &depthAttachment);
	renderInfo.colorAttachmentCount = 0;
	renderInfo.pColorAttachments = NULL;

	vkCmdBeginRendering(cmd, &renderInfo);

	std::vector<uint32_t> opaque_draws;
	opaque_draws.reserve(engine->mainDrawContext.OpaqueSurfaces.size());

	for (int i = 0; i < engine->mainDrawContext.OpaqueSurfaces.size(); i++) {
		/*if (is_visible(mainDrawContext.OpaqueSurfaces[i], sceneData.viewproj)) {*/
		opaque_draws.push_back(i);
		/*}*/
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
		const RenderObject& A = engine->mainDrawContext.OpaqueSurfaces[iA];
		const RenderObject& B = engine->mainDrawContext.OpaqueSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		else {
			return A.material < B.material;
		}
		});

	//allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = engine->create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once its been used
	engine->get_current_frame()._deletionQueue.push_function([=]() {
		engine->destroy_buffer(gpuSceneDataBuffer);
		});

	//write the buffer
	GPUSceneData* sceneUniformData;
	VK_CHECK(vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void**>(&sceneUniformData)));
	*sceneUniformData = engine->sceneData;
	vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(engine->_device, globalDescriptor);

	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	auto draw = [&](const RenderObject& r) {

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthShadowMapPipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthShadowMapPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);

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
		scissor.extent.width = engine->_windowExtent.width;
		scissor.extent.height = engine->_windowExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		/*vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
			&r.material->materialSet, 0, nullptr);*/

		if (r.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = r.indexBuffer;
			vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		// calculate final mesh matrix
		GPUDrawPushConstants push_constants;
		push_constants.worldMatrix = r.transform;
		push_constants.vertexBuffer = r.vertexBufferAddress;

		vkCmdPushConstants(cmd, _depthShadowMapPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

		vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
	};

	for (auto& r : opaque_draws) {
		draw(engine->mainDrawContext.OpaqueSurfaces[r]);
	}

	for (auto& r : engine->mainDrawContext.TransparentSurfaces) {
		draw(r);
	}

	// we delete the draw commands now that we processed them
	/*mainDrawContext.OpaqueSurfaces.clear();
	mainDrawContext.TransparentSurfaces.clear();*/

	vkCmdEndRendering(cmd);
}
