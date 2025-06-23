#include "raytracer.h"
#include "vk_engine.h"

#include <VulkanGeometryKHR.h>
#include <random>
#include <spdlog/spdlog.h>
#include <vk_images.h>

void Raytracer::init_ray_tracing(VulkanEngine *engine) {

    // Requesting ray tracing properties
    VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    prop2.pNext = &m_rtProperties;
    vkGetPhysicalDeviceProperties2(engine->_chosenGPU, &prop2);

    if (m_is_raytracing_supported) {
        m_rt_builder = std::make_unique<nvvk::RaytracingBuilderKHR>(engine, engine->_graphicsQueueFamily);
    }
}

void Raytracer::setRTDefaultData() {
    m_pcRay.samples_done = 0;
    max_samples = 200;
    m_pcRay.depth = 3;
}

void Raytracer::createBottomLevelAS(const VulkanEngine *engine) const {

    // BLAS - Storing each primitive in a geometry
    std::vector<nvvk::RaytracingBuilderKHR::BlasInput> blas_inputs;

    for (const auto &OpaqueSurface: engine->mainDrawContext.OpaqueSurfaces) {
        nvvk::RaytracingBuilderKHR::BlasInput input;
        input = experirender::vk::objectToVkGeometryKHR(OpaqueSurface);
        blas_inputs.emplace_back(std::move(input));
    }
    m_rt_builder->buildBlas(blas_inputs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void Raytracer::createRtOutputImageOnly(VulkanEngine *engine) {
    // Create just the RT output image without descriptors or acceleration structures
    _rtOutputImage = vkutil::create_image(
        engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    vmaSetAllocationName(engine->_allocator, _rtOutputImage.allocation, "RT Output Image");
    
    // Add to deletion queue
    engine->_mainDeletionQueue.push_function([=] {
        vkutil::destroy_image(engine, _rtOutputImage);
    });
}

void Raytracer::createTopLevelAS(const VulkanEngine *engine) const {
    // TLAS - Storing each BLAS
    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    tlas.reserve(engine->mainDrawContext.OpaqueSurfaces.size());

    for (std::uint32_t i = 0; i < engine->mainDrawContext.OpaqueSurfaces.size(); i++) {
        VkTransformMatrixKHR vk_transform = {};
        const glm::mat4 &t = engine->mainDrawContext.OpaqueSurfaces[i].transform;

        vk_transform.matrix[0][0] = t[0][0];
        vk_transform.matrix[0][1] = t[1][0];
        vk_transform.matrix[0][2] = t[2][0];
        vk_transform.matrix[0][3] = t[3][0]; // Translation X

        vk_transform.matrix[1][0] = t[0][1];
        vk_transform.matrix[1][1] = t[1][1];
        vk_transform.matrix[1][2] = t[2][1];
        vk_transform.matrix[1][3] = t[3][1]; // Translation Y

        vk_transform.matrix[2][0] = t[0][2];
        vk_transform.matrix[2][1] = t[1][2];
        vk_transform.matrix[2][2] = t[2][2];
        vk_transform.matrix[2][3] = t[3][2]; // Translation Z

        VkAccelerationStructureInstanceKHR instance{
            .transform = vk_transform,
            .instanceCustomIndex = i,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = m_rt_builder->getBlasDeviceAddress(i),
        };
        tlas.emplace_back(instance);
    }
    m_rt_builder->buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void Raytracer::createRtDescriptorSet(VulkanEngine *engine) {
    // Only create output image if it doesn't exist yet
    if (_rtOutputImage.image == VK_NULL_HANDLE) {
        _rtOutputImage = vkutil::create_image(
            engine, VkExtent3D{engine->_windowExtent.width, engine->_windowExtent.height, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        vmaSetAllocationName(engine->_allocator, _rtOutputImage.allocation, "RT Output Image");
        
        // Add to deletion queue
        engine->_mainDeletionQueue.push_function([=] {
            vkutil::destroy_image(engine, _rtOutputImage);
        });
    }

    {
        DescriptorLayoutBuilder m_rtDescSetLayoutBind;
        m_rtDescSetLayoutBind.add_binding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR); // TLAS
        m_rtDescSetLayoutBind.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // Output image
        m_rtDescSetLayout = m_rtDescSetLayoutBind.build(engine->_device, VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                                                             VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    }
    m_rtDescSet = engine->globalDescriptorAllocator.allocate(engine->_device, m_rtDescSetLayout);

    VkAccelerationStructureKHR tlas = m_rt_builder->getAccelerationStructure();
    VkWriteDescriptorSetAccelerationStructureKHR asInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asInfo.accelerationStructureCount = 1;
    asInfo.pAccelerationStructures = &tlas;
    // VkDescriptorImageInfo imageInfo{ {}, _rtOutputImage.imageView, VK_IMAGE_LAYOUT_GENERAL };

    DescriptorWriter rt_writer;
    rt_writer.write_accel_struct(0, asInfo, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    rt_writer.write_image(1, _rtOutputImage.imageView, engine->_defaultSamplerLinear, VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    rt_writer.update_set(engine->_device, m_rtDescSet);

    // Obj Descriptions
    {
        DescriptorLayoutBuilder m_objDescSetLayoutBind;
        m_objDescSetLayoutBind.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // ObjDesc buffer
        m_objDescSetLayout = m_objDescSetLayoutBind.build(engine->_device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    }

    std::vector<ObjDesc> objDescs;
    objDescs.reserve(engine->mainDrawContext.OpaqueSurfaces.size());
    for (auto &OpaqueSurface: engine->mainDrawContext.OpaqueSurfaces) {
        ObjDesc desc = {.vertexAddress = OpaqueSurface.vertexBufferAddress,
                        .indexAddress = OpaqueSurface.indexBufferAddress,
                        .firstIndex = OpaqueSurface.firstIndex};
        objDescs.push_back(desc);
    }

    m_objDescSet = engine->globalDescriptorAllocator.allocate(engine->_device, m_objDescSetLayout);

    AllocatedBuffer m_objDescSetBuffer = engine->create_buffer(
        sizeof(ObjDesc) * objDescs.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(engine->_allocator, m_objDescSetBuffer.allocation, "RT ObjDesc Buffer");

    ObjDesc *objDescsToMap;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, m_objDescSetBuffer.allocation, reinterpret_cast<void **>(&objDescsToMap)));
    memcpy(objDescsToMap, objDescs.data(), sizeof(ObjDesc) * objDescs.size());
    vmaUnmapMemory(engine->_allocator, m_objDescSetBuffer.allocation);

    DescriptorWriter obj_writer;
    obj_writer.write_buffer(0, m_objDescSetBuffer.buffer, sizeof(ObjDesc) * objDescs.size(), 0,
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    obj_writer.update_set(engine->_device, m_objDescSet);

    // Tex Descriptions
    // Put all textures in loadScenes to a vector
    for (std::uint32_t i = 0; i < engine->mainDrawContext.OpaqueSurfaces.size(); i++) {
        loadedTextures.push_back(engine->mainDrawContext.OpaqueSurfaces[i].material->colImage);
        loadedNormTextures.push_back(engine->mainDrawContext.OpaqueSurfaces[i].material->normImage);
        loadedMetalRoughTextures.push_back(engine->mainDrawContext.OpaqueSurfaces[i].material->metalRoughImage);
        engine->mainDrawContext.OpaqueSurfaces[i].material->albedoTexIndex = i;
    }

    // if textures are not empty
    if (!loadedTextures.empty() || !loadedNormTextures.empty() ||
        engine->hdrImage.get_hdriMap().image != VK_NULL_HANDLE) {
        auto nbTxt = static_cast<uint32_t>(loadedTextures.size());
        auto nbNormText = static_cast<uint32_t>(loadedNormTextures.size());
        auto nbMetalRoughText = static_cast<uint32_t>(loadedMetalRoughTextures.size());

        {
            DescriptorLayoutBuilder m_texSetLayoutBind;
            m_texSetLayoutBind.add_bindings(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt); // Tex Images
            m_texSetLayoutBind.add_bindings(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbNormText); // Normal Maps
            m_texSetLayoutBind.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // HDR Image
            m_texSetLayoutBind.add_bindings(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            nbMetalRoughText); // Metal Rough Maps
            m_texSetLayout = m_texSetLayoutBind.build(engine->_device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                                                           VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        }

        m_texDescSet = engine->globalDescriptorAllocator.allocate(engine->_device, m_texSetLayout);

        // Color Texture
        std::vector<VkDescriptorImageInfo> texDescs;
        texDescs.reserve(nbTxt);
        for (uint32_t i = 0; i < nbTxt; i++) {
            VkDescriptorImageInfo imageInfo{.sampler = engine->_defaultSamplerLinear,
                                            .imageView = loadedTextures[i].imageView,
                                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            texDescs.push_back(imageInfo);
        }

        // Normal Texture
        std::vector<VkDescriptorImageInfo> normTexDescs;
        normTexDescs.reserve(nbNormText);
        for (uint32_t i = 0; i < nbNormText; i++) {
            VkDescriptorImageInfo imageInfo{.sampler = engine->_defaultSamplerLinear,
                                            .imageView = loadedNormTextures[i].imageView,
                                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            normTexDescs.push_back(imageInfo);
        }

        // Metal Rough Texture
        std::vector<VkDescriptorImageInfo> metalRoughTexDescs;
        metalRoughTexDescs.reserve(nbMetalRoughText);
        for (uint32_t i = 0; i < nbMetalRoughText; i++) {
            VkDescriptorImageInfo imageInfo{.sampler = engine->_defaultSamplerLinear,
                                            .imageView = loadedMetalRoughTextures[i].imageView,
                                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            metalRoughTexDescs.push_back(imageInfo);
        }

        DescriptorWriter tex_writer;
        tex_writer.write_images(0, *texDescs.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt);
        tex_writer.write_images(1, *normTexDescs.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbNormText);
        tex_writer.write_image(2, engine->hdrImage.get_hdriMap().imageView, engine->hdrImage.get_hdriMapSampler(),
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        tex_writer.write_images(3, *metalRoughTexDescs.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                nbMetalRoughText); // Metal Roughness
        tex_writer.update_set(engine->_device, m_texDescSet);

        engine->_mainDeletionQueue.push_function(
            [=] { vkDestroyDescriptorSetLayout(engine->_device, m_texSetLayout, nullptr); });
    }

    // Mat descriptions
    std::vector<MaterialRTData> materialRTShaderData;

    for (auto &OpaqueSurface: engine->mainDrawContext.OpaqueSurfaces) {
        MaterialRTData matDesc{};
        matDesc.albedo = OpaqueSurface.material->albedo;
        matDesc.albedoTexIndex = OpaqueSurface.material->albedoTexIndex;
        matDesc.metal_rough_factors = OpaqueSurface.material->metalRoughFactors;
        materialRTShaderData.push_back(matDesc);
    }

    {
        DescriptorLayoutBuilder m_matDescSetLayoutBind;
        m_matDescSetLayoutBind.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // MatDesc buffer
        m_matDescSetLayout = m_matDescSetLayoutBind.build(engine->_device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    }

    m_matDescSet = engine->globalDescriptorAllocator.allocate(engine->_device, m_matDescSetLayout);

    AllocatedBuffer m_matDescSetBuffer =
        engine->create_buffer(sizeof(MaterialRTData) * materialRTShaderData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(engine->_allocator, m_matDescSetBuffer.allocation, "RT MatDesc Buffer");

    MaterialRTData *matDescsToMap;
    VK_CHECK(
        vmaMapMemory(engine->_allocator, m_matDescSetBuffer.allocation, reinterpret_cast<void **>(&matDescsToMap)));
    memcpy(matDescsToMap, materialRTShaderData.data(), sizeof(MaterialRTData) * materialRTShaderData.size());
    vmaUnmapMemory(engine->_allocator, m_matDescSetBuffer.allocation);

    DescriptorWriter mat_writer;
    mat_writer.write_buffer(0, m_matDescSetBuffer.buffer, sizeof(MaterialRTData) * materialRTShaderData.size(), 0,
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    mat_writer.update_set(engine->_device, m_matDescSet);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroyDescriptorSetLayout(engine->_device, m_rtDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, m_objDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, m_matDescSetLayout, nullptr);
        // _rtOutputImage is already handled by its own deletion function
        engine->destroy_buffer(m_objDescSetBuffer);
        engine->destroy_buffer(m_matDescSetBuffer);
    });
}

void Raytracer::updateRtDescriptorSet(const VulkanEngine *engine) const {
    // (1) Output buffer
    // VkDescriptorImageInfo imageInfo{ {}, _rtOutputImage.imageView, VK_IMAGE_LAYOUT_GENERAL };
    DescriptorWriter rt_writer;
    rt_writer.write_image(1, _rtOutputImage.imageView, engine->_defaultSamplerLinear, VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    rt_writer.update_set(engine->_device, m_rtDescSet);
}

void Raytracer::createRtPipeline(VulkanEngine *engine) {
    enum StageIndices { eRaygen, eMiss, eClosestHit, eShaderGroupCount };

    // All stages
    std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.pName = "main";

    // Raygen
    VkShaderModule rayTraceRaygen;
    if (!vkutil::load_shader_module("raytrace.rgen.spv", engine->_device, &rayTraceRaygen)) {
        spdlog::error("Error when building the rayTraceRaygen shader");
    }
    stage.module = rayTraceRaygen;
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages[eRaygen] = stage;

    // Miss
    VkShaderModule rayTraceMiss;
    if (!vkutil::load_shader_module("raytrace.rmiss.spv", engine->_device, &rayTraceMiss)) {
        spdlog::error("Error when building the rayTraceMiss shader");
    }
    stage.module = rayTraceMiss;
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages[eMiss] = stage;

    // Hit Group - Closest Hit
    VkShaderModule rayTraceHit;
    if (!vkutil::load_shader_module("raytrace.rchit.spv", engine->_device, &rayTraceHit)) {
        spdlog::error("Error when building the rayTraceHit shader");
    }
    stage.module = rayTraceHit;
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[eClosestHit] = stage;

    // Shader groups
    VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    // Raygen
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = eRaygen;
    m_rtShaderGroups.push_back(group);

    // Miss
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = eMiss;
    m_rtShaderGroups.push_back(group);

    // closest hit shader
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = eClosestHit;
    m_rtShaderGroups.push_back(group);

    // Push constant: we want to be able to update constants used by the shaders
    VkPushConstantRange pushConstant{VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                         VK_SHADER_STAGE_MISS_BIT_KHR,
                                     0, sizeof(PushConstantRay)};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

    // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
    std::vector<VkDescriptorSetLayout> rtDescSetLayouts = {engine->_gpuSceneDataDescriptorLayout, m_rtDescSetLayout,
                                                           m_objDescSetLayout, m_matDescSetLayout};

    if (!loadedTextures.empty()) {
        rtDescSetLayouts.push_back(m_texSetLayout);
    }
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout));

    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size()); // stages are shaders
    rayPipelineInfo.pStages = stages.data();

    // In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
    // two miss shader groups, and one hit group.
    rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
    rayPipelineInfo.pGroups = m_rtShaderGroups.data();

    // In this case, m_rtShaderGroups.size() == 3: we have one raygen group,
    // one miss shader group, and one hit group.
    rayPipelineInfo.maxPipelineRayRecursionDepth = 1; // Ray Depth
    rayPipelineInfo.layout = m_rtPipelineLayout;

    // Create the ray tracing pipeline
    VK_CHECK(vkCreateRayTracingPipelinesKHR(engine->_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline));

    // Clean up the shader modules
    for (auto &s: stages)
        vkDestroyShaderModule(engine->_device, s.module, nullptr);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroyPipelineLayout(engine->_device, m_rtPipelineLayout, nullptr);
        vkDestroyPipeline(engine->_device, m_rtPipeline, nullptr);
    });
}

// The Shader Binding Table (SBT)
void Raytracer::createRtShaderBindingTable(VulkanEngine *engine) {
    uint32_t missCount{1};
    uint32_t hitCount{1};
    auto handleCount = 1 + missCount + hitCount;
    uint32_t handleSize = m_rtProperties.shaderGroupHandleSize;

    // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be handled
    uint32_t handleSizeAligned = align_up(handleSize, m_rtProperties.shaderGroupBaseAlignment);

    m_rgenRegion.stride = align_up(handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
    m_rgenRegion.size = m_rgenRegion.stride;
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size = align_up(missCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
    m_hitRegion.stride = handleSizeAligned;
    m_hitRegion.size = align_up(hitCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);

    // Get the shader group handles
    uint32_t dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    auto result =
        vkGetRayTracingShaderGroupHandlesKHR(engine->_device, m_rtPipeline, 0, handleCount, dataSize, handles.data());
    assert(result == VK_SUCCESS);

    // Allocate the SBT buffer
    VkDeviceSize sbtBufferSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
    m_rtSBTBuffer = engine->create_buffer(sbtBufferSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                              VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                                          VMA_MEMORY_USAGE_CPU_ONLY);
    vmaSetAllocationName(engine->_allocator, m_rtSBTBuffer.allocation, "RT Shader Binding Table Buffer");

    // Find the SBT addresses for each group
    VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_rtSBTBuffer.buffer};
    VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(engine->_device, &info);
    m_rgenRegion.deviceAddress = sbtAddress;
    m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
    m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_missRegion.size;

    // Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    // Map the SBT buffer and write the handles
    uint8_t *pSBTBuffer;
    VK_CHECK(vmaMapMemory(engine->_allocator, m_rtSBTBuffer.allocation, reinterpret_cast<void **>(&pSBTBuffer)));
    uint8_t *pData{nullptr};
    uint32_t handleIdx{0};

    // Raygen
    pData = pSBTBuffer;
    memcpy(pData, getHandle(static_cast<int>(handleIdx++)), handleSize);

    // Miss
    pData = pSBTBuffer + m_rgenRegion.size;
    for (uint32_t c = 0; c < missCount; c++) {
        memcpy(pData, getHandle(static_cast<int>(handleIdx++)), handleSize);
        pData += m_missRegion.stride;
    }

    // Hit
    pData = pSBTBuffer + m_rgenRegion.size + m_missRegion.size;
    for (uint32_t c = 0; c < hitCount; c++) {
        memcpy(pData, getHandle(static_cast<int>(handleIdx++)), handleSize);
        pData += m_hitRegion.stride;
    }

    // Clean up
    vmaUnmapMemory(engine->_allocator, m_rtSBTBuffer.allocation);

    engine->_mainDeletionQueue.push_function([=] { engine->destroy_buffer(m_rtSBTBuffer); });
}

void Raytracer::resetSamples() {
    // Reset the sample index
    m_pcRay.samples_done = 0;
}

//--------------------------------------------------------------------------------------------------
// Ray Tracing the scene
//
void Raytracer::raytrace(VulkanEngine *engine, const VkCommandBuffer &cmdBuf, const glm::vec4 &clearColor) {

    // allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer =
        engine->create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    vmaSetAllocationName(engine->_allocator, gpuSceneDataBuffer.allocation, "SceneDataBuffer_drawGeom");

    // add it to the deletion queue of this frame so it gets deleted once its been used
    engine->get_current_frame()._deletionQueue.push_function(
        [=, this]() { engine->destroy_buffer(gpuSceneDataBuffer); });

    // write the buffer
    engine->upload_to_vma_allocation(&engine->sceneData, sizeof(GPUSceneData), gpuSceneDataBuffer);

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        engine->get_current_frame()._frameDescriptors.allocate(engine->_device, engine->_gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(engine->_device, globalDescriptor);

    if (m_pcRay.samples_done == max_samples) {
        return;
    }

    // Initializing the push constants
    std::random_device rd; // Non-deterministic seed source
    std::mt19937 gen(rd()); // Mersenne Twister engine
    m_pcRay.seed = gen();

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

    // if textures are empty, we don't bind the texture descriptor set
    if (loadedTextures.empty()) {
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0, 1,
                                &globalDescriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 1, 1, &m_rtDescSet,
                                0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 2, 1, &m_objDescSet,
                                0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 3, 1, &m_matDescSet,
                                0, nullptr);
    } else {
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0, 1,
                                &globalDescriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 1, 1, &m_rtDescSet,
                                0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 2, 1, &m_objDescSet,
                                0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 3, 1, &m_matDescSet,
                                0, nullptr);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 4, 1, &m_texDescSet,
                                0, nullptr);
    }

    vkCmdPushConstants(cmdBuf, m_rtPipelineLayout,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                           VK_SHADER_STAGE_MISS_BIT_KHR,
                       0, sizeof(PushConstantRay), &m_pcRay);

    vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, engine->_windowExtent.width,
                      engine->_windowExtent.height, 1);

    m_pcRay.samples_done++;
}

// RT updates
void Raytracer::rtSampleUpdates(const VulkanEngine *engine) {
    // Camera movement
    if (engine->mainCamera.isMoving) {
        resetSamples();
    }

    // Sunlight direction change
    bool sunDirChanged = false;

    if (engine->sceneData.sunlightDirection != prevSunDir) {
        sunDirChanged = true;
        prevSunDir = engine->sceneData.sunlightDirection; // Update previous value
    }

    if (sunDirChanged) {
        resetSamples();
    }

    // If max_samples has decreased, reset samples
    if (max_samples < prevMaxSamples) {
        resetSamples();
    }

    // Update previous max samples to the new value
    prevMaxSamples = max_samples;

    // If the depth has decreased, reset samples
    if (m_pcRay.depth < prevMaxDepth || m_pcRay.depth > prevMaxDepth) {
        resetSamples();
    }

    // Update previous max depth to the new value
    prevMaxDepth = m_pcRay.depth;
}