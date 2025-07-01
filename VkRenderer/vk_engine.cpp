
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1

#ifdef NSIGHT_AFTERMATH_ENABLED
#include "NsightAftermathGpuCrashTracker.h"
#include "NsightAftermathHelpers.h"
#include "NsightAftermathShaderDatabase.h"
#endif

#include "vk_buffers.h"
#include "vk_engine.h"
#include "vk_loader.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include <VulkanGeometryKHR.h>
#include <glm/gtx/transform.hpp>
#include <raytraceKHR_vk.h>
#include <vk_images.h>
#include <vk_initializers.h>
#include <vk_mem_alloc.h>
#include <vk_types.h>
#include <vk_utils.h>

#include <filesystem>
#include <iostream>
#include <random>

constexpr bool bUseValidationLayers = true;


void VulkanEngine::init() {
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    constexpr auto window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    _window =
        SDL_CreateWindow("ExperiRender", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         static_cast<int>(_windowExtent.width), static_cast<int>(_windowExtent.height), window_flags);

    mainCamera.velocity = glm::vec3(0.f);

    // Default camera position for general scene viewing
    mainCamera.position = glm::vec3(-0.645665, 0.081437, 1.63236);
    mainCamera.pitch = -0.276666f;
    mainCamera.yaw = 0.383333f;

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    init_descriptors();

    init_pipelines();

    ui::init_imgui(this);

    init_default_data();

    // Start with empty scene - no automatic loading
    // User can drag-and-drop GLTF files to load scenes

    // everything went fine
    _isInitialized = true;

    // Initialize ray tracing with empty scene
    raytracerPipeline.init_ray_tracing(this);
    // Create RT output image even without geometry
    raytracerPipeline.createRtOutputImageOnly(this);
    // Skip BLAS/TLAS creation until we have geometry to add
}

void VulkanEngine::load_scene_from_file(const std::string &filePath) {
    try {
        spdlog::info("Loading GLTF scene from: {}", filePath);

        // Load the GLTF file
        const auto sceneFile = loadGltf(this, filePath);

        if (sceneFile.has_value()) {
            // Extract filename for scene name
            std::filesystem::path path(filePath);
            std::string sceneName = path.stem().string();

            // Destroy cube pipeline since we're loading a scene
            if (cubePipeline.isInitialized()) {
                cubePipeline.destroy();
            }

            // Clear existing scenes and ray tracing texture references
            loadedScenes.clear();
            sceneInfos.clear();

            // Clear main draw context from previous scene
            mainDrawContext.OpaqueSurfaces.clear();
            mainDrawContext.TransparentSurfaces.clear();

            // Clear ray tracing texture references
            raytracerPipeline.loadedTextures.clear();
            raytracerPipeline.loadedNormTextures.clear();
            raytracerPipeline.loadedMetalRoughTextures.clear();

            // Add to loaded scenes
            loadedScenes[sceneName] = *sceneFile;

            // Create a scene info with same position as cube
            SceneDesc::SceneInfo sceneInfo;
            sceneInfo.name = sceneName;
            sceneInfo.filePath = filePath;
            sceneInfo.hasTransform = true;
            sceneInfo.scale = glm::vec3(1.0f); // Keep original scale
            sceneInfo.translate = glm::vec3(0.0f, -0.5f, 0.0f); // Same Y offset as cube
            sceneInfo.rotate = glm::vec3(0.0f); // No rotation
            sceneInfos[sceneName] = sceneInfo;

            spdlog::info("Successfully loaded scene: {}", sceneName);

            // Update ray tracing structures
            traverseScenes();
            raytracerPipeline.createBottomLevelAS(this);
            raytracerPipeline.createTopLevelAS(this);
            raytracerPipeline.createRtDescriptorSet(this);
            raytracerPipeline.createRtPipeline(this);
            raytracerPipeline.createRtShaderBindingTable(this);

        } else {
            spdlog::error("Failed to load GLTF file: {}", filePath);
        }
    } catch (const std::exception &e) {
        spdlog::error("Error loading scene: {}", e.what());
    }
}

void VulkanEngine::init_scenes(const std::string &jsonPath) {
    try {
        // Load all scenes from JSON
        std::vector<SceneDesc::SceneInfo> scenes = SceneDesc::getAllScenes(jsonPath);

        // Process each scene
        for (const auto &sceneInfo: scenes) {
            // Load the GLTF file
            const auto sceneFile = loadGltf(this, sceneInfo.filePath);

            if (sceneFile.has_value()) {
                // Add to loaded scenes (using your existing map type)
                loadedScenes[sceneInfo.name] = *sceneFile;
                // Store the scene info separately
                sceneInfos[sceneInfo.name] = sceneInfo;
                spdlog::info("Loaded scene: {}", sceneInfo.name);
            } else {
                spdlog::error("Failed to load scene: {}", sceneInfo.name);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading scenes: " << e.what() << std::endl;
    }
    // Initialize HDRI with the same JSON file
    hdrImage.load_hdri_to_buffer(this, jsonPath);
}


void VulkanEngine::init_default_data() {

    // some default lighting parameters
    sceneData.ambientColor = glm::vec4(.053f, .049f, .049f, .049f);
    sceneData.sunlightColor = glm::vec4(2.f);
    glm::vec3 sunDir = glm::vec3(0.607f, -10.0f, -1.791f); // this is the default value for 'structure' scene
    sceneData.sunlightDirection = glm::vec4(sunDir, 1.0f);
    sceneData.sunlightDirection.w = 1.573f; // sun intensity
    raytracerPipeline.prevSunDir = glm::vec4(
        sunDir, sceneData.sunlightDirection.w); // TODO: Change sunlight code so that prevSunDir can be set in rt file
    sceneData.enableShadows = true;
    sceneData.enableSSAO = true;
    sceneData.enablePBR = true;

    // 3 default textures, white, grey and black. 1 pixel each.
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    _whiteImage = vkutil::create_image(this, (void *) &white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT, false, "whiteImage");

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    _greyImage = vkutil::create_image(this, (void *) &grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                      VK_IMAGE_USAGE_SAMPLED_BIT, false, "greyImage");

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    _blackImage = vkutil::create_image(this, (void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT, false, "blackImage");

    // checkerboard texture
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 * 16> pixels{};
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[x + y * 16] = x % 2 ^ y % 2 ? magenta : black;
        }
    }
    _errorCheckerboardImage = vkutil::create_image(this, pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                                   VK_IMAGE_USAGE_SAMPLED_BIT, false, "errorCheckerboardImage");

    // Shadow light map
    _shadowMap.init_lightSpaceMatrix(this);

    _ssao.init_ssao_data(this);

    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;

    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

    _mainDeletionQueue.push_function([=] {
        vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
        vkDestroySampler(_device, _defaultSamplerLinear, nullptr);

        vkutil::destroy_image(this, _whiteImage);
        vkutil::destroy_image(this, _greyImage);
        vkutil::destroy_image(this, _blackImage);
        vkutil::destroy_image(this, _errorCheckerboardImage);
    });

    GLTFMetallic_Roughness::MaterialResources materialResources{};
    // default the material textures
    materialResources.colorImage = _whiteImage;
    materialResources.colorSampler = _defaultSamplerLinear;
    materialResources.metalRoughImage = _whiteImage;
    materialResources.metalRoughSampler = _defaultSamplerLinear;
    materialResources.normalImage = _greyImage;
    materialResources.normalSampler = _defaultSamplerLinear;

    // set the uniform buffer for the material data
    AllocatedBuffer materialConstants = vkutil::create_buffer(this, sizeof(GLTFMetallic_Roughness::MaterialConstants),
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              VMA_MEMORY_USAGE_CPU_TO_GPU, "Material Constants Buffer");

    // write the buffer
    vkutil::run_with_mapped_memory(_allocator, materialConstants.allocation, [&](void *data) {
        auto *sceneUniformData = static_cast<GLTFMetallic_Roughness::MaterialConstants *>(data);
        sceneUniformData->colorFactors = glm::vec4{1, 1, 1, 1};
        sceneUniformData->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
        sceneUniformData->hasMetalRoughTex = 0;
    });

    _mainDeletionQueue.push_function([=, this]() { vkutil::destroy_buffer(this, materialConstants); });

    materialResources.dataBuffer = materialConstants.buffer;
    materialResources.dataBufferOffset = 0;

    defaultData = metalRoughMaterial.write_material(this, _device, MaterialPass::MainColor, materialResources,
                                                    globalDescriptorAllocator);

    for (auto &m: testMeshes) {
        std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
        newNode->mesh = m;

        newNode->localTransform = glm::mat4{1.f};
        newNode->worldTransform = glm::mat4{1.f};

        for (auto &s: newNode->mesh->surfaces) {
            s.material = std::make_shared<GLTFMaterial>(defaultData);
        }

        loadedNodes[m->name] = std::move(newNode);
    }

    // RT defaults
    raytracerPipeline.setRTDefaultData();

    // Initialize cube pipeline
    cubePipeline.init(this);
}


// Global function to check if an object is visible in the current view
bool is_visible(const RenderObject &obj, const glm::mat4 &viewproj) {
    constexpr std::array corners{
        glm::vec3{1, 1, 1},  glm::vec3{1, 1, -1},  glm::vec3{1, -1, 1},  glm::vec3{1, -1, -1},
        glm::vec3{-1, 1, 1}, glm::vec3{-1, 1, -1}, glm::vec3{-1, -1, 1}, glm::vec3{-1, -1, -1},
    };

    const glm::mat4 matrix = viewproj * obj.transform;

    glm::vec3 min = {1.5, 1.5, 1.5};
    glm::vec3 max = {-1.5, -1.5, -1.5};

    for (int c = 0; c < 8; c++) {
        // project each corner into clip space
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

        // perspective correction
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;

        min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
        max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
    }

    // check the clip space box is within the view
    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
        return false;
    }
    return true;
}

void VulkanEngine::traverseScenes() {
    for (const auto &[sceneName, scenePtr]: loadedScenes) {
        glm::mat4 modelMatrix(1.0f);

        // Check if we have scene info with transformations
        auto infoIt = sceneInfos.find(sceneName);
        if (infoIt != sceneInfos.end() && infoIt->second.hasTransform) {
            const auto &transform = infoIt->second;

            // Apply transformations in order: scale, rotate, translate
            // Scale
            modelMatrix = glm::scale(modelMatrix, transform.scale);

            // Rotate (converting Euler angles from degrees to radians)
            glm::vec3 rotationRad = glm::radians(transform.rotate);
            modelMatrix = glm::rotate(modelMatrix, rotationRad.x, glm::vec3(1.0f, 0.0f, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, rotationRad.y, glm::vec3(0.0f, 1.0f, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, rotationRad.z, glm::vec3(0.0f, 0.0f, 1.0f));

            // Translate
            modelMatrix = glm::translate(modelMatrix, transform.translate);
        }

        // Draw the scene with the calculated model matrix
        scenePtr->Draw(modelMatrix, mainDrawContext);
    }
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {

        // make sure the GPU has stopped doing its things
        vkDeviceWaitIdle(_device);

        loadedScenes.clear();

        // Free command buffers first
        for (auto &frame: _frames) {
            vkFreeCommandBuffers(_device, frame._commandPool, 1, &frame._mainCommandBuffer);
        }
        vkFreeCommandBuffers(_device, _immCommandPool, 1, &_immCommandBuffer);

        for (auto &frame: _frames) {
            frame._deletionQueue.flush();
        }

        _mainDeletionQueue.flush();

        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vmaDestroyAllocator(_allocator);

        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }
}

void VulkanEngine::draw() {
    update_scene();

#ifdef NSIGHT_AFTERMATH_ENABLED
    // Update frame markers for GPU crash tracking
    updateFrameMarkers();
#endif

    // Double buffering
    // wait until the GPU has finished rendering the last frame. Timeout of 1
    // second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

    get_current_frame()._deletionQueue.flush();
    get_current_frame()._frameDescriptors.clear_pools(_device);

    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));


    // request image from the swapchain
    uint32_t swapchainImageIndex;
    VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore,
                                       nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
        return;
    }

    // naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // now that we are sure that the commands finished executing,
    // we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin the command buffer recording. We will use this command buffer exactly once,
    // so we want to let Vulkan know that
    const VkCommandBufferBeginInfo cmdBeginInfo =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    _drawExtent.height = static_cast<uint32_t>(
        static_cast<float>(std::min(_swapchainExtent.height, _drawImage.imageExtent.height)) * renderScale);

    _drawExtent.width = static_cast<uint32_t>(
        static_cast<float>(std::min(_swapchainExtent.width, _drawImage.imageExtent.width)) * renderScale);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Check if raytracer is enabled
    useRaytracer = (postProcessor._compositorData.useRayTracer == 1);

    // Always prepare the raytracer output image for either pathway
    vkutil::transition_image(cmd, raytracerPipeline._rtOutputImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

    if (useRaytracer) {
        // Raytracing path
        raytracerPipeline.raytrace(this, cmd, glm::vec4(0.0f));
        vkutil::transition_image(cmd, raytracerPipeline._rtOutputImage.image, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // Skip directly to post-processing using raytraced output
        vkutil::transition_image(cmd, postProcessor._fullscreenImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        postProcessor.draw(this, cmd);

        vkutil::transition_image(cmd, postProcessor._fullscreenImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    } else {
        // Rasterization path - all the normal draw calls
        vkutil::transition_image(cmd, raytracerPipeline._rtOutputImage.image, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        vkutil::transition_image(cmd, gbuffer.getGbufferPosInfo().image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        vkutil::transition_image(cmd, gbuffer.getGbufferNormInfo().image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        gbuffer.draw_gbuffer(this, cmd);

        vkutil::transition_image(cmd, _ssao._depthMap.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        vkutil::copy_image_to_image(cmd, _depthImage.image, _ssao._depthMap.image, _drawExtent, _ssao._depthMapExtent,
                                    VK_FILTER_NEAREST, VK_IMAGE_ASPECT_DEPTH_BIT);
        vkutil::transition_image(cmd, _ssao._depthMap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

        vkutil::transition_image(cmd, _ssao._ssaoImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_ASPECT_COLOR_BIT);
        _ssao.draw_ssao(this, cmd);
        vkutil::transition_image(cmd, _ssao._ssaoImage.image, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        vkutil::transition_image(cmd, _ssao._ssaoImageBlurred.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_ASPECT_COLOR_BIT);
        _ssao.draw_ssao_blur(this, cmd);
        vkutil::transition_image(cmd, _ssao._ssaoImageBlurred.image, VK_IMAGE_LAYOUT_GENERAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // transition main draw image
        vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // Shadow pass
        vkutil::transition_image(cmd, _shadowMap._depthShadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        _shadowMap.draw_depthShadowMap(this, cmd);
        vkutil::transition_image(cmd, _shadowMap._depthShadowMap.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

        vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

        hdrImage.draw_hdriMap(this, cmd);
        draw_geometry(cmd);

        // Transition draw image for post-processing
        vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        vkutil::transition_image(cmd, postProcessor._fullscreenImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        postProcessor.draw(this, cmd);

        vkutil::transition_image(cmd, postProcessor._fullscreenImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Common final steps for both paths
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    // Copy post-processed result to swapchain
    vkutil::copy_image_to_image(cmd, postProcessor._fullscreenImage.image, _swapchainImages[swapchainImageIndex],
                                _drawExtent, _swapchainExtent, VK_FILTER_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT);

    // Prepare swapchain for UI
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    // Draw UI
    ui::draw_imgui(this, cmd, _swapchainImageViews[swapchainImageIndex]);

    // Prepare for presentation
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);

    // Finalize command buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                   get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo =
        vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    const VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    // prepare present
    //  this will put the image we just rendered to into the visible window.
    //  we want to wait on the _renderSemaphore for that,
    //  as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    if (const VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
        presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
    }

    // increase the number of frames drawn
    _frameNumber++;
}

void VulkanEngine::update_scene() {

    mainDrawContext.OpaqueSurfaces.clear();

    mainCamera.update();

    const glm::mat4 view = mainCamera.getViewMatrix();

    //// camera projection
    glm::mat4 projection = glm::perspective(
        glm::radians(75.f), static_cast<float>(_windowExtent.width) / static_cast<float>(_windowExtent.height), 10000.f,
        0.01f);

    //// invert the Y direction on projection matrix so that we are more similar
    //// to OpenGL and gLTF axis
    projection[1][1] *= -1;

    mainCamera.update();

    sceneData.view = view;
    _ssao.ssaoData.view = view;
    sceneData.proj = projection;
    sceneData.viewproj = projection * view;
    _ssao.ssaoData.viewproj = projection * view;
    sceneData.cameraPosition = mainCamera.position;

    // shadows
    _shadowMap.update_lightSpaceMatrix(this);

    // Process all loaded scenes
    traverseScenes();

    // RT updates
    raytracerPipeline.rtSampleUpdates(this);
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        auto start = std::chrono::system_clock::now();

        mainCamera.isRotating = false;
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {

                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }

            // Handle drag-and-drop events
            if (e.type == SDL_DROPFILE) {
                char *dropped_filedir = e.drop.file;
                std::string filePath(dropped_filedir);
                spdlog::info("File dropped: {}", filePath);

                // Check if it's a GLTF file
                if (filePath.ends_with(".gltf") || filePath.ends_with(".glb")) {
                    load_scene_from_file(filePath);
                }
                // Check if it's an HDRI file
                else if (filePath.ends_with(".hdr") || filePath.ends_with(".exr")) {
                    hdrImage.load_hdri_from_file(this, filePath);
                } else {
                    spdlog::warn("Unsupported file type. Please drop a .gltf, .glb, .hdr, or .exr file.");
                }

                SDL_free(dropped_filedir);
            }

            mainCamera.processSDLEvent(e);

            // send SDL event to imgui for handling
            ui::handle_sdl_event(&e);
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            /*continue;*/
        }

        if (resize_requested) {
            raytracerPipeline.updateRtDescriptorSet(this);
            resize_swapchain();
        }

        ui::setup_imgui_panel(this);

        // our draw function
        draw();

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        stats.frametime = static_cast<float>(elapsed.count()) / 1000.f;
    }
}

void VulkanEngine::init_vulkan() {

#ifdef NSIGHT_AFTERMATH_ENABLED
    // Initialize GPU crash tracker with engine's marker map
    GpuCrashTracker gpuCrashTracker(m_markerMap);
    gpuCrashTracker.Initialize(false);
#endif

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("ExperiRender")
                        .set_engine_name("ExperiRender")
                        .request_validation_layers(bUseValidationLayers)
                        .require_api_version(1, 3, 0)
                        .use_default_debug_messenger()
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.runtimeDescriptorArray = true;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.shaderInt64 = true;

    // Custom GPU selection - find all GPUs first
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    // Try to find NVIDIA RTX GPU first
    bool foundRTXGPU = false;
    std::string rtxGpuName;

    for (const auto &device: devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        std::string deviceName = deviceProperties.deviceName;
        spdlog::info("Found GPU: {}", deviceName);

        // Check for NVIDIA RTX
        if (deviceProperties.vendorID == 0x10DE && // NVIDIA vendor ID
            deviceName.find("RTX") != std::string::npos) {
            rtxGpuName = deviceName;
            foundRTXGPU = true;
            spdlog::info("Found NVIDIA RTX GPU: {}", deviceName);
            break;
        }
    }

    // Define raytracing extensions for later use
    const std::vector<const char *> raytracing_extensions{
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };

    // Define diagnostic extensions
    const std::vector<const char *> diagnostic_extensions{
        VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
        VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
    };

    vkb::PhysicalDevice physicalDevice;

    if (foundRTXGPU) {
        // Raytracing features for RTX GPU
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructureFeatures.pNext = nullptr;
        accelerationStructureFeatures.accelerationStructure = true;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingPipelineFeatures{};
        raytracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        raytracingPipelineFeatures.pNext = nullptr;
        raytracingPipelineFeatures.rayTracingPipeline = true;

        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        rayQueryFeatures.rayQuery = VK_TRUE;

        VkDeviceDiagnosticsConfigCreateInfoNV diagnosticsConfig = {};
        diagnosticsConfig.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
        diagnosticsConfig.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
                                  VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                                  VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV;
        diagnosticsConfig.pNext = nullptr;


        // Try to select the RTX GPU with raytracing features
        try {
            vkb::PhysicalDeviceSelector rtxSelector{vkb_inst};
            physicalDevice = rtxSelector.set_minimum_version(1, 3)
                                 .set_required_features_13(features)
                                 .set_required_features_12(features12)
                                 .set_surface(_surface)
                                 .set_name(rtxGpuName) // Specifically select our RTX GPU by name
                                 .add_required_extensions(raytracing_extensions)
                                 .add_required_extensions(diagnostic_extensions)
                                 .add_required_extension_features(accelerationStructureFeatures)
                                 .add_required_extension_features(raytracingPipelineFeatures)
                                 .add_required_extension_features(rayQueryFeatures)
                                 .add_required_extension_features(diagnosticsConfig)
                                 .set_required_features(deviceFeatures)
                                 .select()
                                 .value();

            spdlog::info("Selected NVIDIA RTX GPU with ray tracing support: {}", rtxGpuName);
            raytracerPipeline.m_is_raytracing_supported = true;
        } catch (const std::exception &e) {
            spdlog::error("Failed to select RTX GPU with ray tracing: {}", e.what());
            foundRTXGPU = false;
            // Fall back to standard selection below
        }
    }

    // If no RTX GPU or selection failed, use standard selection without ray tracing
    if (!foundRTXGPU) {
        vkb::PhysicalDeviceSelector selector{vkb_inst};
        physicalDevice = selector.set_minimum_version(1, 3)
                             .set_required_features_13(features)
                             .set_required_features_12(features12)
                             .set_surface(_surface)
                             .set_required_features(deviceFeatures)
                             .select()
                             .value();

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice.physical_device, &deviceProperties);
        spdlog::info("Selected GPU (without ray tracing support): {}", deviceProperties.deviceName);
        raytracerPipeline.m_is_raytracing_supported = false;
    }

    // create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    if (raytracerPipeline.m_is_raytracing_supported) {
        experirender::vk::load_raytracing_functions(_instance);
    }

    // get the VkDevice handle used in the rest of a Vulkan application
    _device = vkbDevice.device;

#ifdef NSIGHT_AFTERMATH_ENABLED
    // Load vkCmdSetCheckpointNV function pointer for custom markers
    vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)vkGetDeviceProcAddr(_device, "vkCmdSetCheckpointNV");
    if (!vkCmdSetCheckpointNV) {
        spdlog::warn("Failed to load vkCmdSetCheckpointNV function pointer - custom GPU markers will not work");
    } else {
        spdlog::info("Successfully loaded vkCmdSetCheckpointNV for custom GPU crash markers");
    }
#endif
    _chosenGPU = physicalDevice.physical_device;

    // vkbootstrap to get a graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

#ifdef NSIGHT_AFTERMATH_ENABLED
    // Initialize marker map and frame index for GPU crash tracking
    for (auto& frameMap : m_markerMap) {
        frameMap.clear();
    }
    m_currentFrameIndex = 0;
    spdlog::info("Initialized Nsight Aftermath marker tracking system");
#endif

    _mainDeletionQueue.push_function([&]() {
        if (raytracerPipeline.m_is_raytracing_supported) {
            raytracerPipeline.m_rt_builder->destroy();
        }
    });
}


void VulkanEngine::init_commands() {

    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    const VkCommandPoolCreateInfo commandPoolInfo =
        vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (auto &_frame: _frames) {

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frame._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frame._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frame._mainCommandBuffer));

        _mainDeletionQueue.push_function([=] { vkDestroyCommandPool(_device, _frame._commandPool, nullptr); });
    }

    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

    // allocate the command buffer for immediate submits
    const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

    _mainDeletionQueue.push_function([=] { vkDestroyCommandPool(_device, _immCommandPool, nullptr); });
}

void VulkanEngine::init_sync_structures() {

    // create syncronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain
    // we want the fence to start signalled so we can wait on it on the first
    // frame
    const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));

    _mainDeletionQueue.push_function([=] { vkDestroyFence(_device, _immFence, nullptr); });

    for (auto &_frame: _frames) {

        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frame._renderFence));

        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frame._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frame._renderSemaphore));

        _mainDeletionQueue.push_function([=] {
            vkDestroyFence(_device, _frame._renderFence, nullptr);
            vkDestroySemaphore(_device, _frame._swapchainSemaphore, nullptr);
            vkDestroySemaphore(_device, _frame._renderSemaphore, nullptr);
        });
    }
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {

    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain =
        swapchainBuilder
            .set_desired_format(VkSurfaceFormatKHR{_swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    _swapchainExtent = vkbSwapchain.extent;
    // store the swapchain images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain() {
    create_swapchain(_windowExtent.width, _windowExtent.height);

    // draw image size will match the window
    const VkExtent3D drawImageExtent = {_windowExtent.width, _windowExtent.height, 1};

    // hardcoding the draw format to 32-bit float
    _drawImage.imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

    // for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

    // build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info =
        vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

    _depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    _depthImage.imageExtent = drawImageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

    // build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info =
        vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

    // add to deletion queues
    _mainDeletionQueue.push_function([=] {
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

        vkDestroyImageView(_device, _depthImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
    });
}

void VulkanEngine::destroy_swapchain() const {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // destroy swapchain resources
    for (auto _swapchainImageView: _swapchainImageViews) {
        vkDestroyImageView(_device, _swapchainImageView, nullptr);
    }
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd) {
#ifdef NSIGHT_AFTERMATH_ENABLED
    insertGPUMarker(cmd, "Begin draw_geometry");
#endif

    // reset counters
    stats.drawcall_count = 0;
    stats.triangle_count = 0;

    // VkClearValue clearVal = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };

    // begin a render pass  connected to our draw image
    VkRenderingAttachmentInfo depthAttachment =
        vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    std::array colorAttachments = {vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL)};

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, nullptr /*color attachments*/, &depthAttachment);
    renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderInfo.pColorAttachments = colorAttachments.data();

#ifdef NSIGHT_AFTERMATH_ENABLED
    insertGPUMarker(cmd, "Begin Render Pass Setup");
#endif

    vkCmdBeginRendering(cmd, &renderInfo);

    // If no scenes are loaded, draw the default cube
    if (loadedScenes.empty() && cubePipeline.isInitialized()) {
#ifdef NSIGHT_AFTERMATH_ENABLED
        insertGPUMarker(cmd, "Drawing Default Cube");
#endif
        cubePipeline.draw(this, cmd);
        vkCmdEndRendering(cmd);
        return;
    }

    // begin clock
    auto start = std::chrono::system_clock::now();

    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(mainDrawContext.OpaqueSurfaces.size());

    for (int i = 0; i < mainDrawContext.OpaqueSurfaces.size(); i++) {
        /*if (is_visible(mainDrawContext.OpaqueSurfaces[i], sceneData.viewproj)) {*/
        opaque_draws.push_back(i);
        /*}*/
    }

    // sort the opaque surfaces by material and mesh
    auto &surfaces = mainDrawContext.OpaqueSurfaces;
    std::ranges::sort(opaque_draws, [&](const auto &iA, const auto &iB) {
        const RenderObject &A = surfaces[iA];
        const RenderObject &B = surfaces[iB];
        if (A.material == B.material) {
            return A.indexBuffer < B.indexBuffer;
        }
        return A.material < B.material;
    });

    // allocate a new uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer =
        vkutil::create_buffer(this, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, "SceneDataBuffer_drawGeom");

    // add it to the deletion queue of this frame so it gets deleted once it's been used
    get_current_frame()._deletionQueue.push_function([=, this] { vkutil::destroy_buffer(this, gpuSceneDataBuffer); });

    // write the buffer
    GPUSceneData *sceneUniformData;
    VK_CHECK(vmaMapMemory(_allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void **>(&sceneUniformData)));
    *sceneUniformData = sceneData;
    vmaUnmapMemory(_allocator, gpuSceneDataBuffer.allocation);

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    writer.update_set(_device, globalDescriptor);

    // defined outside the draw function, this is the state we will try to skip
    MaterialPipeline *lastPipeline = nullptr;
    MaterialInstance *lastMaterial = nullptr;
    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject &r) {
        if (r.material != lastMaterial) {
            lastMaterial = r.material;
            if (r.material->pipeline != lastPipeline) {

                lastPipeline = r.material->pipeline;
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1,
                                        &globalDescriptor, 0, nullptr);


                VkViewport viewport = {};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(_windowExtent.width);
                viewport.height = static_cast<float>(_windowExtent.height);
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;

                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                scissor.extent.width = _windowExtent.width;
                scissor.extent.height = _windowExtent.height;

                vkCmdSetScissor(cmd, 0, 1, &scissor);
            }

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
                                    &r.material->materialSet, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 2, 1,
                                    gbuffer.getInputDescriptorSet(), 0, nullptr);
        }
        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
        // calculate final mesh matrix
        GPUDrawPushConstants push_constants{};
        push_constants.worldMatrix = r.transform;
        push_constants.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(GPUDrawPushConstants), &push_constants);

        stats.drawcall_count++;
        stats.triangle_count += static_cast<int>(r.indexCount) / 3;
        
#ifdef NSIGHT_AFTERMATH_ENABLED
        // Mark individual draw call - this is where TDR might occur
        std::string drawMarker = "DrawCall #" + std::to_string(stats.drawcall_count);
        insertGPUMarker(cmd, drawMarker);
#endif
        
        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    };

    stats.drawcall_count = 0;
    stats.triangle_count = 0;

#ifdef NSIGHT_AFTERMATH_ENABLED
    insertGPUMarker(cmd, "Drawing Opaque Surfaces");
#endif

    for (auto &r: opaque_draws) {
        draw(mainDrawContext.OpaqueSurfaces[r]);
    }

#ifdef NSIGHT_AFTERMATH_ENABLED
    insertGPUMarker(cmd, "Drawing Transparent Surfaces");
#endif

    for (auto &r: mainDrawContext.TransparentSurfaces) {
        draw(r);
    }

#ifdef NSIGHT_AFTERMATH_ENABLED
    insertGPUMarker(cmd, "End Geometry Drawing");
#endif

    // we delete the draw commands now that we processed them
    mainDrawContext.OpaqueSurfaces.clear();
    mainDrawContext.TransparentSurfaces.clear();

    vkCmdEndRendering(cmd);

    auto end = std::chrono::system_clock::now();

    // convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.mesh_draw_time = static_cast<float>(elapsed.count()) / 1000.f;
}

void VulkanEngine::init_descriptors() {
    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                                                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

    globalDescriptorAllocator.init(_device, 10, sizes);


    // make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout =
            builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
                                       VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _singleImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // allocate a descriptor set for our draw image
    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    writer.update_set(_device, _drawImageDescriptors);

    _mainDeletionQueue.push_function([&]() {
        globalDescriptorAllocator.destroy_pools(_device);
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _singleImageDescriptorLayout, nullptr);
    });

    for (auto &_frame: _frames) {
        // create a descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        _frame._frameDescriptors = DescriptorAllocatorGrowable{};
        _frame._frameDescriptors.init(_device, 1000, frame_sizes);

        _mainDeletionQueue.push_function([&]() { _frame._frameDescriptors.destroy_pools(_device); });
    }
}


void VulkanEngine::init_pipelines() {
    // HDRI PIPELINE
    hdrImage.init_hdriMap(this);

    // GBuffer PIPELINE
    gbuffer.init_gbuffer(this);

    // SHADOW PIPELINE
    _shadowMap.init_depthShadowMap(this);

    // SSAO PIPELINE
    _ssao.init_ssao(this);
    _ssao.init_ssao_blur(this);

    // FULLSCREEN PIPELINE
    postProcessor.init(this);

    metalRoughMaterial.build_pipelines(this);
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) const {
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    const VkCommandBufferBeginInfo cmdBeginInfo =
        vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    const VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}

GPUMeshBuffers VulkanEngine::uploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices) const {
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface{};

    // create vertex buffer
    newSurface.vertexBuffer =
        vkutil::create_buffer(this, vertexBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              VMA_MEMORY_USAGE_GPU_ONLY, "Vertex Buffer");

    // find the address of the vertex buffer
    const VkBufferDeviceAddressInfo deviceAdressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                     .buffer = newSurface.vertexBuffer.buffer};
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAdressInfo);

    // create index buffer
    newSurface.indexBuffer =
        vkutil::create_buffer(this, indexBufferSize,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              VMA_MEMORY_USAGE_GPU_ONLY, "Index Buffer");

    const VkBufferDeviceAddressInfo deviceAdressInfo2{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = newSurface.indexBuffer.buffer};
    newSurface.indexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAdressInfo2);

    const AllocatedBuffer staging =
        vkutil::create_buffer(this, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VMA_MEMORY_USAGE_CPU_ONLY, "Staging Buffer");

    vkutil::upload_to_buffer(this, vertices.data(), vertexBufferSize, staging);
    vkutil::upload_to_buffer(this, indices.data(), indexBufferSize, staging, vertexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{0};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });

    vkutil::destroy_buffer(this, staging);

    return newSurface;
}

void VulkanEngine::resize_swapchain() {
    vkDeviceWaitIdle(_device);

    destroy_swapchain();

    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _windowExtent.width = w;
    _windowExtent.height = h;

    create_swapchain(_windowExtent.width, _windowExtent.height);

    resize_requested = false;
}


void GLTFMetallic_Roughness::build_pipelines(VulkanEngine *engine) {
    VkShaderModule meshFragShader;
    if (!vkutil::load_shader_module("mesh.frag.spv", engine->_device, &meshFragShader)) {
        spdlog::error("Error when building the triangle fragment shader module");
    }

    VkShaderModule meshVertexShader;
    if (!vkutil::load_shader_module("mesh.vert.spv", engine->_device, &meshVertexShader)) {
        spdlog::error("Error when building the triangle vertex shader module");
    }

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // builder for colorMap,metallicMap, normalMap, ssaoMap, shadowMap
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // builder for the gbuffer input
    /*DescriptorLayoutBuilder gbufferLayoutBuilder;
    gbufferLayoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    gbufferLayoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    engine->_gbufferInputDescriptorLayout = gbufferLayoutBuilder.build(engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);*/

    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    const VkDescriptorSetLayout layouts[] = {engine->_gpuSceneDataDescriptorLayout, materialLayout,
                                             engine->gbuffer.getInputDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 3;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipelineBuilder.add_color_attachment(engine->_drawImage.imageFormat, PipelineBuilder::BlendMode::NO_BLEND);

    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = newLayout;

    // finally build the pipeline
    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // create the transparent variant
    pipelineBuilder.clear_attachments();
    pipelineBuilder.add_color_attachment(engine->_drawImage.imageFormat, PipelineBuilder::BlendMode::ADDITIVE_BLEND);

    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);

    engine->_mainDeletionQueue.push_function([=] {
        vkDestroyDescriptorSetLayout(engine->_device, materialLayout, nullptr);
        vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
        vkDestroyPipeline(engine->_device, opaquePipeline.pipeline, nullptr);
        vkDestroyPipeline(engine->_device, transparentPipeline.pipeline, nullptr);
    });
}

MaterialInstance GLTFMetallic_Roughness::write_material(VulkanEngine *engine, VkDevice device, MaterialPass pass,
                                                        const MaterialResources &resources,
                                                        DescriptorAllocatorGrowable &descriptorAllocator) {
    MaterialInstance matData{};
    matData.passType = pass;
    if (pass == MaterialPass::Transparent) {
        matData.pipeline = &transparentPipeline;
    } else {
        matData.pipeline = &opaquePipeline;
    }

    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);


    writer.clear();
    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(3, resources.normalImage.imageView, resources.normalSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(4, engine->_ssao._ssaoImageBlurred.imageView, engine->_ssao._ssaoSampler,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(5, engine->_shadowMap._depthShadowMap.imageView, engine->_shadowMap._shadowDepthMapSampler,
                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.update_set(device, matData.materialSet);

    matData.colImage = resources.colorImage;
    matData.colSampler = resources.colorSampler;

    matData.normImage = resources.normalImage;
    matData.normSampler = resources.normalSampler;

    matData.metalRoughImage = resources.metalRoughImage;
    matData.metalRoughSampler = resources.metalRoughSampler;

    matData.albedo = resources.albedo;
    matData.metalRoughFactors = resources.metalRoughFactors;

    return matData;
}

void GLTFMetallic_Roughness::clear_resources(VkDevice device) {}

void MeshNode::Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    const glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (auto &s: mesh->surfaces) {
        RenderObject def{};
        def.indexCount = s.count;
        def.firstIndex = s.startIndex;
        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        def.material = &s.material->data;
        def.bounds = s.bounds;
        def.transform = nodeMatrix;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
        def.indexBufferAddress = mesh->meshBuffers.indexBufferAddress;
        def.vertexCount = mesh->nbVertices;

        if (s.material->data.passType == MaterialPass::Transparent) {
            ctx.TransparentSurfaces.push_back(def);
        } else {
            ctx.OpaqueSurfaces.push_back(def);
        }
    }

    // recurse down
    Node::Draw(topMatrix, ctx);
}

#ifdef NSIGHT_AFTERMATH_ENABLED
// Insert a GPU marker for crash tracking
void VulkanEngine::insertGPUMarker(VkCommandBuffer cmd, const std::string& markerName) {
    if (vkCmdSetCheckpointNV) {
        // Create a unique marker ID from string hash
        std::hash<std::string> hasher;
        uint64_t markerId = hasher(markerName);
        
        // Store the marker in the current frame's map
        uint32_t frameIndex = m_currentFrameIndex % 4;
        m_markerMap[frameIndex][markerId] = markerName;
        
        // Insert the checkpoint
        vkCmdSetCheckpointNV(cmd, (void*)markerId);
        
        // Optional: Log marker insertion for debugging
        // spdlog::debug("GPU Marker inserted: {} (ID: {})", markerName, markerId);
    }
}

// Update frame markers - call this at the start of each frame
void VulkanEngine::updateFrameMarkers() {
    m_currentFrameIndex++;
    
    // Clear old marker data (keep only last 4 frames)
    uint32_t frameIndex = m_currentFrameIndex % 4;
    m_markerMap[frameIndex].clear();
}
#endif
