#include "Hdri.h"

#include <filesystem>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include "vk_buffers.h"
#include "vk_images.h"

void HDRI::load_hdri_to_buffer(VulkanEngine *engine, const std::string &jsonFilePath) {
    try {
        // Get HDRI info from JSON
        SceneDesc::HDRIInfo hdriInfo = SceneDesc::getHDRIInfo(jsonFilePath);

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);

        // Use the filepath from JSON
        float *data = stbi_loadf(hdriInfo.filePath.c_str(), &width, &height, &nrComponents, 0);

        if (!data) {
            spdlog::error("Failed to load HDRI image from {}", hdriInfo.filePath);
            // Fallback to default if specified path fails
            std::string defaultPath = "../assets/HDRI/pretoria_gardens_4k.hdr";
            spdlog::info("Trying default HDRI path: {}", defaultPath);
            data = stbi_loadf(defaultPath.c_str(), &width, &height, &nrComponents, 0);

            if (!data) {
                spdlog::error("Failed to load default HDRI image!");
                return;
            }
        }

        // Use the loaded data...
        _hdriMap = vkutil::create_hdri_image(engine, data, width, height, nrComponents, "HDRI Map Image");

        if (_hdriMap.image == VK_NULL_HANDLE) {
            spdlog::error("Failed to initialize HDRI!");
        } else {
            spdlog::info("HDRI initialized successfully from {}", hdriInfo.filePath);
        }

        // Sampler configs
        VkSamplerCreateInfo sampl2 = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampl2.magFilter = VK_FILTER_LINEAR;
        sampl2.minFilter = VK_FILTER_LINEAR;
        sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampl2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(engine->_device, &sampl2, nullptr, &_hdriMapSampler);

        stbi_set_flip_vertically_on_load(false);

        // Cleanup will be handled by a single deletion queue entry in init_hdriMap

    } catch (const std::exception &e) {
        spdlog::error("Error loading HDRI from JSON: {}", e.what());
        // Fallback to original hardcoded path
        load_hdri_to_buffer_fallback(engine);
    }
}

void HDRI::load_hdri_from_file(VulkanEngine *engine, const std::string &hdriFilePath) {
    try {
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);

        spdlog::info("Loading HDRI file: {}", hdriFilePath);
        float *data = stbi_loadf(hdriFilePath.c_str(), &width, &height, &nrComponents, 0);

        if (!data) {
            spdlog::error("Failed to load HDRI image from {}", hdriFilePath);
            return;
        }

        // Clean up old HDRI allocation before creating new one to prevent VMA leaks
        // Use frame deletion queue instead of device wait idle to avoid stutters

        if (_hdriMap.image != VK_NULL_HANDLE) {
            AllocatedImage oldHdriMap = _hdriMap;
            engine->get_current_frame()._deletionQueue.push_function(
                [=] { vkutil::destroy_image(engine, oldHdriMap); });
            _hdriMap = {};
        }
        if (_hdriMapSampler != VK_NULL_HANDLE) {
            VkSampler oldSampler = _hdriMapSampler;
            engine->get_current_frame()._deletionQueue.push_function(
                [=] { vkDestroySampler(engine->_device, oldSampler, nullptr); });
            _hdriMapSampler = VK_NULL_HANDLE;
        }

        // Create new HDRI image
        _hdriMap = vkutil::create_hdri_image(engine, data, width, height, nrComponents, "HDRI Map Image");

        if (_hdriMap.image == VK_NULL_HANDLE) {
            spdlog::error("Failed to initialize HDRI!");
            return;
        }

        // Create sampler
        VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(engine->_device, &samplerInfo, nullptr, &_hdriMapSampler);

        stbi_set_flip_vertically_on_load(false);

        // Cleanup is handled by the single deletion queue entry in init_hdriMap

        spdlog::info("HDRI loaded successfully from {}", hdriFilePath);

    } catch (const std::exception &e) {
        spdlog::error("Error loading HDRI file: {}", e.what());
    }
}

// Add this fallback method that uses the original hardcoded path
void HDRI::load_hdri_to_buffer_fallback(VulkanEngine *engine) {
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    float *data = stbi_loadf("../assets/HDRI/pretoria_gardens_4k.hdr", &width, &height, &nrComponents, 0);

    if (!data) {
        spdlog::error("Failed to load HDRI image!");
        return;
    }

    // Use the loaded data...
    _hdriMap = vkutil::create_hdri_image(engine, data, width, height, nrComponents, "HDRI Map Image");

    if (_hdriMap.image == VK_NULL_HANDLE) {
        spdlog::error("Failed to initialize cubemap!");
    } else {
        spdlog::info("Cubemap initialized successfully");
    }

    // Sampler configs
    VkSamplerCreateInfo sampl2 = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampl2.magFilter = VK_FILTER_LINEAR;
    sampl2.minFilter = VK_FILTER_LINEAR;
    sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampl2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(engine->_device, &sampl2, nullptr, &_hdriMapSampler);

    stbi_set_flip_vertically_on_load(false);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroySampler(engine->_device, _hdriMapSampler, nullptr);
        vkutil::destroy_image(engine, _hdriMap);
    });
}

void HDRI::init_hdriMap(VulkanEngine *engine) {
    // Check if we have a valid HDRI image loaded
    if (_hdriMap.image == VK_NULL_HANDLE) {
        // Create a default grey 1x1 HDRI image
        float greyPixel[4] = {0.026f, 0.026f, 0.026f, 1.0f}; // RGBA float - medium grey
        _hdriMap = vkutil::create_image(engine, (void *) greyPixel, VkExtent3D{1, 1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
                                        VK_IMAGE_USAGE_SAMPLED_BIT, false, "Default HDRI Image");

        // Create sampler
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;
        sampl.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(engine->_device, &sampl, nullptr, &_hdriMapSampler);

        // Cleanup will be handled by a single deletion queue entry in init_hdriMap
    }

    _hdriOutImage = vkutil::create_image(
        engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, false,
        "HDRI Output Image");

    // HDRI
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        hdriMapDescriptorSetLayout =
            builder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    VkDescriptorSetLayout layouts[] = {hdriMapDescriptorSetLayout, engine->_gpuSceneDataDescriptorLayout};

    VkPipelineLayoutCreateInfo hdri_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    hdri_pipeline_layout_info.setLayoutCount = 2;
    hdri_pipeline_layout_info.pSetLayouts = layouts;
    hdri_pipeline_layout_info.flags = 0;
    hdri_pipeline_layout_info.pushConstantRangeCount = 0;
    hdri_pipeline_layout_info.pPushConstantRanges = nullptr;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &hdri_pipeline_layout_info, nullptr, &_hdriMapPipelineLayout));

    VkShaderModule skyboxVertShader;
    if (!vkutil::load_shader_module("Skybox.vert.spv", engine->_device, &skyboxVertShader)) {
        spdlog::error("Error when building the Skybox fragment shader module");
    }

    VkShaderModule skyboxFragShader;
    if (!vkutil::load_shader_module("Skybox.frag.spv", engine->_device, &skyboxFragShader)) {
        spdlog::error("Error when building the Skybox vertex shader module");
    }

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(skyboxVertShader, skyboxFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // render format
    pipelineBuilder.add_color_attachment(engine->_drawImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);
    pipelineBuilder.add_color_attachment(VK_FORMAT_R16G16B16A16_SFLOAT, PipelineBuilder::BlendMode::NO_BLEND);

    pipelineBuilder.set_depth_format(VK_FORMAT_D32_SFLOAT);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = _hdriMapPipelineLayout;

    // create the pipeline
    _hdriMapPipeline = pipelineBuilder.build_pipeline(engine->_device);

    // destruction
    vkDestroyShaderModule(engine->_device, skyboxVertShader, nullptr);
    vkDestroyShaderModule(engine->_device, skyboxFragShader, nullptr);

    engine->_mainDeletionQueue.push_function([=] { cleanup(engine); });
}

void HDRI::draw_hdriMap(VulkanEngine *engine, VkCommandBuffer cmd) {

    VkClearValue clearVal = {.color = {0.0f, 0.0f, 0.0f, 1.0f}};

    std::array<VkRenderingAttachmentInfo, 2> colorAttachments = {
        vkinit::attachment_info(engine->_drawImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        vkinit::attachment_info(_hdriOutImage.imageView, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
    };

    VkClearValue depthClear = {.depthStencil = {1.0f, 0}};
    VkRenderingAttachmentInfo depthAttachment =
        vkinit::depth_attachment_info(engine->_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    depthAttachment.clearValue = depthClear;
    VkRenderingInfo renderInfo =
        vkinit::rendering_info(engine->_windowExtent, nullptr /*color attachments*/, &depthAttachment);
    renderInfo.pColorAttachments = colorAttachments.data();
    renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());

    vkCmdBeginRendering(cmd, &renderInfo);

    // Allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer =
        vkutil::create_buffer(engine, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, "Skybox Scene Data Buffer");

    // Add it to the deletion queue
    engine->get_current_frame()._deletionQueue.push_function(
        [=] { vkutil::destroy_buffer(engine, gpuSceneDataBuffer); });

    // Write the scene data
    GPUSceneData *sceneUniformData;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void **>(&sceneUniformData)));
    *sceneUniformData = engine->sceneData;
    vmaUnmapMemory(engine->_allocator, gpuSceneDataBuffer.allocation);

    // Create and update descriptor set
    VkDescriptorSet globalDescriptor =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

    {
        DescriptorWriter writer;
        writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(engine->_device, globalDescriptor);
    }
    hdriMapDescriptorSet =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, hdriMapDescriptorSetLayout);
    {
        DescriptorWriter writer;
        writer.write_image(0, _hdriMap.imageView, _hdriMapSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.update_set(engine->_device, hdriMapDescriptorSet);
    }

    // Bind pipeline and descriptors
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriMapPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriMapPipelineLayout, 0, 1, &hdriMapDescriptorSet,
                            0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriMapPipelineLayout, 1, 1, &globalDescriptor, 0,
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

void HDRI::cleanup(VulkanEngine *engine) {
    if (_hdriMapSampler != VK_NULL_HANDLE) {
        vkDestroySampler(engine->_device, _hdriMapSampler, nullptr);
        _hdriMapSampler = VK_NULL_HANDLE;
    }
    if (_hdriMap.image != VK_NULL_HANDLE) {
        vkutil::destroy_image(engine, _hdriMap);
        _hdriMap = {};
    }
    if (_hdriOutImage.image != VK_NULL_HANDLE) {
        vkutil::destroy_image(engine, _hdriOutImage);
        _hdriOutImage = {};
    }
    if (_hdriMapPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(engine->_device, _hdriMapPipeline, nullptr);
        _hdriMapPipeline = VK_NULL_HANDLE;
    }
    if (_hdriMapPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(engine->_device, _hdriMapPipelineLayout, nullptr);
        _hdriMapPipelineLayout = VK_NULL_HANDLE;
    }
    if (hdriMapDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(engine->_device, hdriMapDescriptorSetLayout, nullptr);
        hdriMapDescriptorSetLayout = VK_NULL_HANDLE;
    }
}
