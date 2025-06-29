
#include <spdlog/spdlog.h>
#include <vk_buffers.h>
#include <vk_images.h>
#include "vk_mem_alloc.h"

#include "shadowmap.h"
#include "vk_engine.h"

namespace {
    constexpr int SHADOWMAP_SIZE = 4096;
}

void shadowMap::init_lightSpaceMatrix(VulkanEngine *engine) {

    near_plane = 0.01f, far_plane = 90.0f;
    left = -20.f, right = 20.f;
    bottom = -20.f, top = 20.f;
    lightProjection = glm::ortho(left, right, bottom, top, far_plane, near_plane);
    /*lightProjection[1][1] *= -1.0f;*/
    glm::vec3 sunlightDir =
        glm::normalize(glm::vec3(engine->sceneData.sunlightDirection.x, engine->sceneData.sunlightDirection.y,
                                 engine->sceneData.sunlightDirection.z));
    glm::vec3 lightPos = -sunlightDir * 25.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    engine->sceneData.lightSpaceMatrix = lightProjection * lightView;
}

void shadowMap::update_lightSpaceMatrix(VulkanEngine *engine) {

    lightProjection = glm::ortho(left, right, bottom, top, far_plane, near_plane);
    /*lightProjection[1][1] *= -1;*/
    glm::vec3 sunlightDir =
        glm::normalize(glm::vec3(engine->sceneData.sunlightDirection.x, engine->sceneData.sunlightDirection.y,
                                 engine->sceneData.sunlightDirection.z));
    glm::vec3 lightPos = -sunlightDir * 25.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    engine->sceneData.lightSpaceMatrix = lightProjection * lightView;
}

void shadowMap::init_depthShadowMap(VulkanEngine *engine) {

    _depthShadowMap = vkutil::create_image(engine, VkExtent3D{SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1}, VK_FORMAT_D32_SFLOAT,
                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    vmaSetAllocationName(engine->_allocator, _depthShadowMap.allocation, "Shadow Depth Map");

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

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &_depthShadowMapPipelineLayout));

    VkShaderModule shadowDepthMapFragShader;
    if (!vkutil::load_shader_module("ShadowDepthMap.frag.spv", engine->_device, &shadowDepthMapFragShader)) {
        spdlog::error("Error when building the ShadowDepthMap fragment shader module");
    }

    VkShaderModule shadowDepthMapVertShader;
    if (!vkutil::load_shader_module("ShadowDepthMap.vert.spv", engine->_device, &shadowDepthMapVertShader)) {
        spdlog::error("Error when building the ShadowDepthMap vertex shader module");
    }

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(shadowDepthMapVertShader, shadowDepthMapFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    pipelineBuilder.set_depth_format(_depthShadowMap.imageFormat);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _depthShadowMapPipelineLayout;

    // create the pipeline
    _depthShadowMapPipeline = pipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, shadowDepthMapFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, shadowDepthMapVertShader, nullptr);

    VkSamplerCreateInfo sampl2 = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl2.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    sampl2.magFilter = VK_FILTER_NEAREST;
    sampl2.minFilter = VK_FILTER_NEAREST;
    sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampl2.compareEnable = VK_TRUE;
    sampl2.compareOp = VK_COMPARE_OP_LESS;

    vkCreateSampler(engine->_device, &sampl2, nullptr, &_shadowDepthMapSampler);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroyPipelineLayout(engine->_device, _depthShadowMapPipelineLayout, nullptr);
        vkDestroyPipeline(engine->_device, _depthShadowMapPipeline, nullptr);
        vkDestroySampler(engine->_device, _shadowDepthMapSampler, nullptr);
        vkutil::destroy_image(engine, _depthShadowMap);
    });
}

void shadowMap::draw_depthShadowMap(VulkanEngine *engine, VkCommandBuffer cmd) const {

    // begin a render pass connected to our draw image
    VkRenderingAttachmentInfo depthAttachment =
        vkinit::depth_attachment_info(_depthShadowMap.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    /*std::array<VkRenderingAttachmentInfo, 2> colorAttachments = {
        vkinit::attachment_info(_depthShadowMap.imageView, &clearVal, VK_IMAGE_LAYOUT_GENERAL),
    };*/

    VkRenderingInfo renderInfo = vkinit::rendering_info(VkExtent2D{SHADOWMAP_SIZE, SHADOWMAP_SIZE},
                                                        nullptr /*color attachments*/, &depthAttachment);
    renderInfo.colorAttachmentCount = 0;
    renderInfo.pColorAttachments = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);

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
    vmaSetAllocationName(engine->_allocator, gpuSceneDataBuffer.allocation, "Shadow Map Scene Data Buffer");

    // add it to the deletion queue of this frame so it gets deleted once its been used
    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });

    // write the buffer
    GPUSceneData *sceneUniformData;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void **>(&sceneUniformData)));
    *sceneUniformData = engine->sceneData;
    vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(engine->_device, globalDescriptor);

    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject &r) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthShadowMapPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _depthShadowMapPipelineLayout, 0, 1,
                                &globalDescriptor, 0, nullptr);

        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(SHADOWMAP_SIZE);
        viewport.height = static_cast<float>(SHADOWMAP_SIZE);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = SHADOWMAP_SIZE;
        scissor.extent.height = SHADOWMAP_SIZE;

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

        vkCmdPushConstants(cmd, _depthShadowMapPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GPUDrawPushConstants),
                           &push_constants);

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
