#pragma once

#include <DeletionQueue.h>
#include <RenderObject.h>
#include <Vertex.h>
#include <functional>
#include <postprocess.h>
#include <raytracer.h>
#include <shadowmap.h>
#include <ssao.h>
#include <ui.h>
#include <vk_descriptors.h>
#include <vk_types.h>
#include "Hdri.h"
#include "Scene/SceneDesc.h"
#include "Scene/camera.h"
#include "cube.h"
#include "gbuffer.h"

#include <glm/glm.hpp>

#include "VkBootstrap.h"

struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkCommandBuffer _rtCommandBuffer;

    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    DeletionQueue _deletionQueue;
    DescriptorAllocatorGrowable _frameDescriptors;
};

constexpr unsigned int FRAME_OVERLAP = 1;

struct GLTFMetallic_Roughness {
    MaterialPipeline opaquePipeline;
    MaterialPipeline transparentPipeline;

    VkDescriptorSetLayout materialLayout;


    struct MaterialConstants {
        glm::vec4 colorFactors;
        glm::vec4 metal_rough_factors;
        int hasMetalRoughTex;

        // padding, we need it anyway for uniform buffers
        uint8_t padding[220];
    };

    struct MaterialResources {
        AllocatedImage colorImage;
        VkSampler colorSampler;
        uint32_t colorTexIndex;
        AllocatedImage metalRoughImage;
        VkSampler metalRoughSampler;
        AllocatedImage normalImage;
        VkSampler normalSampler;
        VkBuffer dataBuffer;
        uint32_t dataBufferOffset;

        glm::vec4 albedo;
        uint32_t albedoTexIndex;
        glm::vec4 metalRoughFactors;
    };

    DescriptorWriter writer;

    void build_pipelines(VulkanEngine *engine);

    static void clear_resources(VkDevice device);

    MaterialInstance write_material(VulkanEngine *engine, VkDevice device, MaterialPass pass,
                                    const MaterialResources &resources,
                                    DescriptorAllocatorGrowable &descriptorAllocator);
};

struct EngineStats {
    float frametime;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};

struct MeshNode final : Node {

    std::shared_ptr<MeshAsset> mesh;

    void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;
};

class VulkanEngine {
public:
    // Vulkan instance
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
    VkDevice _device; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface

    VkSwapchainKHR _swapchain; // Swapchain handle
    VkFormat _swapchainImageFormat; // Swapchain image format

    std::vector<VkImage> _swapchainImages; // Swapchain image handles
    std::vector<VkImageView> _swapchainImageViews; // Swapchain image view handles
    VkExtent2D _swapchainExtent; // Swapchain image resolution

    DrawContext mainDrawContext{};
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

    // GUI
    ui gui;

    void update_scene();

    Camera mainCamera;

    EngineStats stats;

    bool _isInitialized{false};
    int _frameNumber{0};
    bool stop_rendering{false};
    bool useRaytracer{false};

    // Ray tracing
    void traverseLoadedMeshNodesOnceForRT();
    Raytracer raytracerPipeline;

    VkExtent2D _windowExtent{1600, 900};

    float renderScale = 1.f;

    FrameData _frames[FRAME_OVERLAP];

    FrameData &get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

    VkQueue _graphicsQueue{};
    uint32_t _graphicsQueueFamily;

    struct SDL_Window *_window{nullptr};

    DeletionQueue _mainDeletionQueue;

    VmaAllocator _allocator;

    DescriptorAllocatorGrowable globalDescriptorAllocator;

    VkDescriptorSet _drawImageDescriptors{};
    VkDescriptorSetLayout _drawImageDescriptorLayout{};

    VkDescriptorSetLayout _singleImageDescriptorLayout;

    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

    GPUSceneData sceneData;

    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

    MaterialInstance defaultData;
    GLTFMetallic_Roughness metalRoughMaterial;

    // Gbuffer
    Gbuffer gbuffer;

    // HDRI
    HDRI hdrImage;

    // draw resources
    AllocatedImage _drawImage;
    AllocatedImage _depthImage;
    VkExtent2D _drawExtent{};

    // shadow resources
    shadowMap _shadowMap;

    // SSAO resources
    ssao _ssao;

    // Full screen quad resources
    PostProcessor postProcessor;

    // immediate submit structures
    VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;

    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;

    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;
    VkSampler _defaultSamplerShadowDepth;

    void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) const;

    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
    std::unordered_map<std::string, SceneDesc::SceneInfo> sceneInfos;

    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) const;

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;
    void destroy_buffer(const AllocatedBuffer &buffer) const;
    void upload_to_vma_allocation(const void *src, size_t size, const AllocatedBuffer &dst_allocation,
                                  size_t dst_offset = 0) const;

    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup();

    // draw loop
    void draw();

    // run main loop
    void run();

    // dynamic scene loading
    void load_scene_from_file(const std::string &filePath);

    bool resize_requested{false};
    bool drawGBufferPositions{false};

private:
    void draw_geometry(VkCommandBuffer cmd);
    void traverseScenes();

    void init_pipelines();

    void init_descriptors();

    void init_vulkan();
    void init_scenes(const std::string &jsonPath);
    void init_swapchain();
    void init_commands();
    void init_sync_structures();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain() const;
    void resize_swapchain();

    void init_default_data();

    // Cube pipeline
    CubePipeline cubePipeline;
};