#include "postprocess.h"
#include <spdlog/spdlog.h>
#include <vk_buffers.h>
#include <vk_images.h>
#include <vk_pipelines.h>

void PostProcessor::init(VulkanEngine *engine) {

    // init compositor data
    _compositorData.useRayTracer = 0;
    _compositorData.exposure = 4.0f;
    _compositorData.showGrid = 0;
    _compositorData.useFXAA = 0; // TODO: Keeping this on by default doesnt load because of rt accell structure
                                 // validation error. Should debug this later.

    // init FXAA data
    _fxaaData.R_inverseFilterTextureSize =
        glm::vec3(1.0f / static_cast<float>(engine->_windowExtent.width), 1.0f / static_cast<float>(engine->_windowExtent.height), 0.0f);
    _fxaaData.R_fxaaSpanMax = 8.0f;
    _fxaaData.R_fxaaReduceMin = 1.0f / 128.0f;
    _fxaaData.R_fxaaReduceMul = 1.0f / 8.0f;

    _fullscreenImage = vkutil::create_image(
        engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        false, "Post Process Image");

    _fxaaImage = vkutil::create_image(engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1},
                                      VK_FORMAT_R8G8B8A8_UNORM,
                                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                      false, "FXAA Image");

    // Create a sampler for the image
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

    // allocate a descriptor set for our post process input
    _postProcessDescriptorSet =
        engine->globalDescriptorAllocator.allocate(engine->_device, _postProcessDescriptorSetLayout);

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyDescriptorSetLayout(engine->_device, _postProcessDescriptorSetLayout, nullptr); });

    VkPipelineLayoutCreateInfo post_process_layout_info{};
    post_process_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    post_process_layout_info.pNext = nullptr;
    post_process_layout_info.setLayoutCount = 1;
    post_process_layout_info.pSetLayouts = &_postProcessDescriptorSetLayout;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &post_process_layout_info, nullptr, &_postProcessPipelineLayout));

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyPipelineLayout(engine->_device, _postProcessPipelineLayout, nullptr); });

    // layout code
    VkShaderModule fullscreenDrawVertShader;
    if (!vkutil::load_shader_module("Fullscreen.vert.spv", engine->_device, &fullscreenDrawVertShader)) {
        spdlog::error("Error when building the Fullscreen Vertex shader");
    }
    VkShaderModule fullscreenDrawFragShader;
    if (!vkutil::load_shader_module("Fullscreen.frag.spv", engine->_device, &fullscreenDrawFragShader)) {
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

    // render format
    pipelineBuilder.add_color_attachment(_fullscreenImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _postProcessPipelineLayout;

    // create the pipeline
    _postProcessPipeline = pipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, fullscreenDrawFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, fullscreenDrawVertShader, nullptr);

    // FXAA PIPELINE
    {
        DescriptorLayoutBuilder fxaaBuilder;
        fxaaBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        fxaaBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _fxaaDescriptorSetLayout = fxaaBuilder.build(engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // allocate a descriptor set for our FXAA input
    _fxaaDescriptorSet = engine->globalDescriptorAllocator.allocate(engine->_device, _fxaaDescriptorSetLayout);

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyDescriptorSetLayout(engine->_device, _fxaaDescriptorSetLayout, nullptr); });

    VkPipelineLayoutCreateInfo fxaa_layout_info{};
    fxaa_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    fxaa_layout_info.pNext = nullptr;
    fxaa_layout_info.setLayoutCount = 1;
    fxaa_layout_info.pSetLayouts = &_fxaaDescriptorSetLayout;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &fxaa_layout_info, nullptr, &_fxaaPipelineLayout));

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyPipelineLayout(engine->_device, _fxaaPipelineLayout, nullptr); });

    // FXAA shaders
    VkShaderModule fxaaVertShader;
    if (!vkutil::load_shader_module("Fullscreen.vert.spv", engine->_device, &fxaaVertShader)) {
        spdlog::error("Error when building the FXAA Vertex shader");
    }
    VkShaderModule fxaaFragShader;
    if (!vkutil::load_shader_module("FXAA.frag.spv", engine->_device, &fxaaFragShader)) {
        spdlog::error("Error when building the FXAA Fragment shader");
    }

    // Build FXAA pipeline
    PipelineBuilder fxaaPipelineBuilder;
    fxaaPipelineBuilder.set_shaders(fxaaVertShader, fxaaFragShader);
    fxaaPipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    fxaaPipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    fxaaPipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    fxaaPipelineBuilder.set_multisampling_none();
    fxaaPipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    fxaaPipelineBuilder.add_color_attachment(_fxaaImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);

    // use the FXAA layout we created
    fxaaPipelineBuilder._pipelineLayout = _fxaaPipelineLayout;

    // create the pipeline
    _fxaaPipeline = fxaaPipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, fxaaFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, fxaaVertShader, nullptr);

    // GRID PIPELINE
    {
        DescriptorLayoutBuilder gridBuilder;
        gridBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // scene data
        _gridDescriptorSetLayout = gridBuilder.build(engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyDescriptorSetLayout(engine->_device, _gridDescriptorSetLayout, nullptr); });

    VkPipelineLayoutCreateInfo grid_layout_info{};
    grid_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    grid_layout_info.pNext = nullptr;
    grid_layout_info.setLayoutCount = 1;
    grid_layout_info.pSetLayouts = &_gridDescriptorSetLayout;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &grid_layout_info, nullptr, &_gridPipelineLayout));

    engine->_mainDeletionQueue.push_function(
        [=, this] { vkDestroyPipelineLayout(engine->_device, _gridPipelineLayout, nullptr); });

    // Grid shaders
    VkShaderModule gridVertShader;
    if (!vkutil::load_shader_module("Grid.vert.spv", engine->_device, &gridVertShader)) {
        spdlog::error("Error when building the Grid Vertex shader");
    }
    VkShaderModule gridFragShader;
    if (!vkutil::load_shader_module("Grid.frag.spv", engine->_device, &gridFragShader)) {
        spdlog::error("Error when building the Grid Fragment shader");
    }

    // Build grid pipeline
    PipelineBuilder gridPipelineBuilder;
    gridPipelineBuilder.set_shaders(gridVertShader, gridFragShader);
    gridPipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gridPipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    gridPipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    gridPipelineBuilder.set_multisampling_none();
    gridPipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    gridPipelineBuilder.set_depth_format(VK_FORMAT_D32_SFLOAT);

    // render to the draw image format for geometry pass
    gridPipelineBuilder.add_color_attachment(engine->_drawImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);
    gridPipelineBuilder._pipelineLayout = _gridPipelineLayout;

    _gridPipeline = gridPipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, gridFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, gridVertShader, nullptr);

    engine->_mainDeletionQueue.push_function([=, this] {
        vkDestroyPipeline(engine->_device, _postProcessPipeline, nullptr);
        vkDestroyPipeline(engine->_device, _fxaaPipeline, nullptr);
        vkDestroyPipeline(engine->_device, _gridPipeline, nullptr);
        vkDestroySampler(engine->_device, _fullscreenImageSampler, nullptr);
        vkutil::destroy_image(engine, _fullscreenImage);
        vkutil::destroy_image(engine, _fxaaImage);
    });
}

void PostProcessor::draw(VulkanEngine *engine, VkCommandBuffer cmd) {
    VkClearValue clearVal = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

    std::array<VkRenderingAttachmentInfo, 1> colorAttachments = {
        vkinit::attachment_info(_fullscreenImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
    };

    // Pass the color attachments directly to rendering_info
    VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_windowExtent,
                                                        colorAttachments.data(), // Pass your color attachments directly
                                                        nullptr // No depth attachment
    );

    vkCmdBeginRendering(cmd, &renderInfo);

    _postProcessDescriptorSet =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, _postProcessDescriptorSetLayout);

    // Allocate a new uniform buffer for the scene data
    AllocatedBuffer compositorSceneDataBuffer =
        vkutil::create_buffer(engine, sizeof(CompositorData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, "Compositor Scene Data Buffer");

    // Add it to the deletion queue
    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, compositorSceneDataBuffer); });

    // Write the scene data
    CompositorData *compositorUniformBuffer;
    VK_CHECK(vmaMapMemory(engine->_allocator, compositorSceneDataBuffer.allocation,
                          reinterpret_cast<void **>(&compositorUniformBuffer)));
    *compositorUniformBuffer = _compositorData;
    vmaUnmapMemory(engine->_allocator, compositorSceneDataBuffer.allocation);

    {
        DescriptorWriter writer;
        writer.write_image(0, engine->raytracerPipeline._rtOutputImage.imageView,
                           engine->_resourceManager.getLinearSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(1, engine->_drawImage.imageView, engine->_resourceManager.getLinearSampler(),
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_buffer(2, compositorSceneDataBuffer.buffer, sizeof(CompositorData), 0,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(engine->_device, _postProcessDescriptorSet);
    }

    // Bind pipeline and descriptors
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _postProcessPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _postProcessPipelineLayout, 0, 1,
                            &_postProcessDescriptorSet, 0, nullptr);

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

void PostProcessor::draw_fxaa(VulkanEngine *engine, VkCommandBuffer cmd) {
    VkClearValue clearVal = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

    std::array<VkRenderingAttachmentInfo, 1> colorAttachments = {
        vkinit::attachment_info(_fxaaImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
    };

    // Pass the color attachments directly to rendering_info
    VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_windowExtent,
                                                        colorAttachments.data(), // Pass your color attachments directly
                                                        nullptr // No depth attachment
    );

    vkCmdBeginRendering(cmd, &renderInfo);

    _fxaaDescriptorSet =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, _fxaaDescriptorSetLayout);

    // Allocate a new uniform buffer for the FXAA data
    AllocatedBuffer fxaaDataBuffer = vkutil::create_buffer(engine, sizeof(FXAAData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                           VMA_MEMORY_USAGE_CPU_TO_GPU, "FXAA Data Buffer");

    // Add it to the deletion queue
    engine->get_current_frame()._deletionQueue.push_function([=] { vkutil::destroy_buffer(engine, fxaaDataBuffer); });

    // Write the FXAA data
    FXAAData *fxaaUniformBuffer;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, fxaaDataBuffer.allocation, reinterpret_cast<void **>(&fxaaUniformBuffer)));
    *fxaaUniformBuffer = _fxaaData;
    vmaUnmapMemory(engine->_allocator, fxaaDataBuffer.allocation);

    {
        DescriptorWriter writer;
        writer.write_image(0, _fullscreenImage.imageView, _fullscreenImageSampler,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_buffer(1, fxaaDataBuffer.buffer, sizeof(FXAAData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(engine->_device, _fxaaDescriptorSet);
    }

    // Bind pipeline and descriptors
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _fxaaPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _fxaaPipelineLayout, 0, 1, &_fxaaDescriptorSet, 0,
                            nullptr);

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

void PostProcessor::draw_grid_only(VulkanEngine *engine, VkCommandBuffer cmd) {
    // Only draw grid, not the fullscreen composition
    if (!_compositorData.showGrid) {
        return;
    }

    std::array<VkRenderingAttachmentInfo, 1> colorAttachments = {
        vkinit::attachment_info(_fullscreenImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
    };

    VkRenderingInfo renderInfo = vkinit::rendering_info(engine->_windowExtent, colorAttachments.data(), nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    AllocatedBuffer gpuSceneDataBuffer =
        vkutil::create_buffer(engine, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, "Grid Scene Data Buffer");

    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });

    GPUSceneData *sceneUniformData;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void **>(&sceneUniformData)));
    *sceneUniformData = engine->sceneData;
    vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

    VkDescriptorSet gridDescriptorSet =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, _gridDescriptorSetLayout);

    {
        DescriptorWriter gridWriter;
        gridWriter.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0,
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        gridWriter.update_set(engine->_device, gridDescriptorSet);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipelineLayout, 0, 1, &gridDescriptorSet, 0,
                            nullptr);

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

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

void PostProcessor::draw_grid_geometry(VulkanEngine *engine, VkCommandBuffer cmd) {

    if (!_compositorData.showGrid) {
        return;
    }

    AllocatedBuffer gpuSceneDataBuffer =
        vkutil::create_buffer(engine, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, "Grid Geometry Scene Data Buffer");

    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });

    GPUSceneData *sceneUniformData;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void **>(&sceneUniformData)));
    *sceneUniformData = engine->sceneData;
    vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

    VkDescriptorSet gridDescriptorSet =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, _gridDescriptorSetLayout);

    {
        DescriptorWriter gridWriter;
        gridWriter.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0,
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        gridWriter.update_set(engine->_device, gridDescriptorSet);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gridPipelineLayout, 0, 1, &gridDescriptorSet, 0,
                            nullptr);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(engine->_drawExtent.width);
    viewport.height = static_cast<float>(engine->_drawExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = engine->_drawExtent;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
