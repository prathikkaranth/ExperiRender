#include "gbuffer.h"
#include <spdlog/spdlog.h>
#include "vk_buffers.h"
#include "vk_images.h"

void Gbuffer::init_gbuffer(VulkanEngine *engine) {

    _gbufferPosition = vkutil::create_image(
        engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    vmaSetAllocationName(engine->_allocator, _gbufferPosition.allocation, "GBuffer Position Image");
    _gbufferNormal = vkutil::create_image(
        engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    vmaSetAllocationName(engine->_allocator, _gbufferNormal.allocation, "GBuffer Normal Image");

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayout layouts[] = {engine->_gpuSceneDataDescriptorLayout};

    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 1;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &_gbufferPipelineLayout));

    VkShaderModule gbufferFragShader;
    if (!vkutil::load_shader_module("GBuffer.frag.spv", engine->_device, &gbufferFragShader)) {
        spdlog::error("Error when building the Gbuffer fragment shader module");
    }

    VkShaderModule gbufferVertexShader;
    if (!vkutil::load_shader_module("GBuffer.vert.spv", engine->_device, &gbufferVertexShader)) {
        spdlog::error("Error when building the Gbuffer vertex shader module");
    }

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(gbufferVertexShader, gbufferFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipelineBuilder.add_color_attachment(_gbufferPosition.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);
    pipelineBuilder.add_color_attachment(_gbufferNormal.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);

    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _gbufferPipelineLayout;

    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // create the pipeline
    _gbufferPipeline = pipelineBuilder.build_pipeline(engine->_device);

    // create a sample for gbuffer stores
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;

    // builder for the gbuffer input
    DescriptorLayoutBuilder gbufferLayoutBuilder;
    gbufferLayoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    gbufferLayoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    _gbufferInputDescriptorLayout = gbufferLayoutBuilder.build(engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);

    vkCreateSampler(engine->_device, &sampl, nullptr, &_gbufferSampler);

    // destruction
    vkDestroyShaderModule(engine->_device, gbufferFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, gbufferVertexShader, nullptr);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroyPipelineLayout(engine->_device, _gbufferPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, _gbufferInputDescriptorLayout, nullptr);
        vkDestroyPipeline(engine->_device, _gbufferPipeline, nullptr);
        vkutil::destroy_image(engine, _gbufferNormal);
        vkutil::destroy_image(engine, _gbufferPosition);
        vkDestroySampler(engine->_device, _gbufferSampler, nullptr);
    });
}

void Gbuffer::draw_gbuffer(VulkanEngine *engine, VkCommandBuffer cmd) {

    // allocate a descriptor set for our GBUFFER input
    _gbufferInputDescriptors =
        engine->globalDescriptorAllocator.allocate(engine->_device, _gbufferInputDescriptorLayout);

    DescriptorWriter gbuffer_writer;
    gbuffer_writer.write_image(0, _gbufferPosition.imageView, _gbufferSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    gbuffer_writer.write_image(1, _gbufferNormal.imageView, _gbufferSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    gbuffer_writer.update_set(engine->_device, _gbufferInputDescriptors);

    VkClearValue clearVal = {.color = {0.0f, 0.0f, 0.0f, 1.0f}};
    // begin a render pass  connected to our draw image
    VkRenderingAttachmentInfo depthAttachment =
        vkinit::depth_attachment_info(engine->_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    std::array<VkRenderingAttachmentInfo, 2> colorAttachments = {
        vkinit::attachment_info(_gbufferPosition.imageView, &clearVal, VK_IMAGE_LAYOUT_GENERAL),
        vkinit::attachment_info(_gbufferNormal.imageView, &clearVal, VK_IMAGE_LAYOUT_GENERAL),
    };

    VkRenderingInfo renderInfo =
        vkinit::rendering_info(engine->_drawExtent, nullptr /*color attachments*/, &depthAttachment);
    renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderInfo.pColorAttachments = colorAttachments.data();

    vkCmdBeginRendering(cmd, &renderInfo);

    // begin clock
    // auto start = std::chrono::system_clock::now();

    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(engine->mainDrawContext.OpaqueSurfaces.size());
    for (int i = 0; i < engine->mainDrawContext.OpaqueSurfaces.size(); i++) {
        /*if (is_visible(mainDrawContext.OpaqueSurfaces[i], sceneData.viewproj)) {*/
        opaque_draws.push_back(i);

        /*}*/
    }

    // sort the opaque surfaces by material and mesh
    auto &surfaces = engine->mainDrawContext.OpaqueSurfaces;
    std::ranges::sort(opaque_draws, [&](const auto &iA, const auto &iB) {
        const RenderObject &A = surfaces[iA];
        const RenderObject &B = surfaces[iB];
        if (A.material == B.material) {
            return A.indexBuffer < B.indexBuffer;
        }
        return A.material < B.material;
    });

    // allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer = vkutil::create_buffer(
        engine, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(engine->_allocator, gpuSceneDataBuffer.allocation, "SceneDataBuffer_DrawGbuffer");

    // add it to the deletion queue of this frame so it gets deleted once its been used
    engine->get_current_frame()._deletionQueue.push_function(
        [=, this]() { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });

    // write the buffer
    vkutil::upload_to_buffer(engine, &engine->sceneData, sizeof(GPUSceneData), gpuSceneDataBuffer);

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(engine->_device, globalDescriptor);

    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject &r) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipelineLayout, 0, 1, &globalDescriptor,
                                0, nullptr);

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
        GPUDrawPushConstants push_constants{};
        push_constants.worldMatrix = r.transform;
        push_constants.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, _gbufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(GPUDrawPushConstants), &push_constants);

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    };


    for (auto &r: opaque_draws) {
        draw(engine->mainDrawContext.OpaqueSurfaces[r]);
    }

    for (auto &r: engine->mainDrawContext.TransparentSurfaces) {
        draw(r);
    }

    // we delete the draw commands now that we processed them
    /*mainDrawContext.OpaqueSurfaces.clear();
    mainDrawContext.TransparentSurfaces.clear();*/

    vkCmdEndRendering(cmd);
}