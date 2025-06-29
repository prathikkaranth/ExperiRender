#include "cube.h"
#include <spdlog/spdlog.h>
#include "vk_buffers.h"
#include "vk_descriptors.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"

void CubePipeline::init(VulkanEngine *engine) {
    enginePtr = engine;

    // Load cube shaders
    VkShaderModule cubeVertShader;
    if (!vkutil::load_shader_module("cube.vert.spv", engine->_device, &cubeVertShader)) {
        spdlog::error("Error when building the cube vertex shader module");
        return;
    }

    VkShaderModule cubeFragShader;
    if (!vkutil::load_shader_module("cube.frag.spv", engine->_device, &cubeFragShader)) {
        spdlog::error("Error when building the cube fragment shader module");
        return;
    }

    // Create pipeline layout (reuse existing scene descriptor layout)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &engine->_gpuSceneDataDescriptorLayout;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // Pipeline builder
    PipelineBuilder pipelineBuilder;

    // Set shaders
    pipelineBuilder.set_shaders(cubeVertShader, cubeFragShader);

    // Set pipeline state
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // Set render format
    pipelineBuilder.add_color_attachment(engine->_drawImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    // No vertex input (vertices generated in shader) - don't call set_vertex_input
    pipelineBuilder._pipelineLayout = pipelineLayout;

    pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // Clean up shader modules
    vkDestroyShaderModule(engine->_device, cubeVertShader, nullptr);
    vkDestroyShaderModule(engine->_device, cubeFragShader, nullptr);

    hasPipeline = true;
    spdlog::info("Cube pipeline created successfully");

    // Add to deletion queue
    engine->_mainDeletionQueue.push_function([=, this]() { destroy(); });
}

void CubePipeline::destroy() {
    if (!enginePtr)
        return;

    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(enginePtr->_device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(enginePtr->_device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    hasPipeline = false;
}

void CubePipeline::draw(VulkanEngine *engine, VkCommandBuffer cmd) {
    if (!hasPipeline)
        return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Create scene uniform buffer
    AllocatedBuffer gpuSceneDataBuffer = vkutil::create_buffer(
        engine, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // Update scene data
    vkutil::upload_to_buffer(engine, &engine->sceneData, sizeof(GPUSceneData), gpuSceneDataBuffer);

    // Create descriptor set
    VkDescriptorSet sceneDescriptor =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(engine->_device, sceneDescriptor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sceneDescriptor, 0, nullptr);

    // Draw 36 vertices (12 triangles)
    vkCmdDraw(cmd, 36, 1, 0, 0);

    // Clean up buffer
    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });
}