#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <vk_loader.h>	
#include <functional>
#include <camera.h>
#include <vk_utils.h>
#include <ssao.h>
#include <RenderObject.h>
#include <shadowmap.h>
#include <raytraceKHR_vk.h>

#include <glm/glm.hpp>

#include "VkBootstrap.h"

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct FrameData {
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	VkCommandBuffer _rtCommandBuffer;

	VkSemaphore _swapchainSemaphore , _renderSemaphore;
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

		//padding, we need it anyway for uniform buffers
		uint8_t padding[220];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
		AllocatedImage normalImage;
		VkSampler normalSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	DescriptorWriter writer;

	void build_pipelines(VulkanEngine* engine);
	void clear_resources(VkDevice device);

	MaterialInstance write_material(VulkanEngine* engine, VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;		
	std::vector<RenderObject> TransparentSurfaces;
	
};

struct EngineStats {
	float frametime;
	int triangle_count;
	int drawcall_count;
	float scene_update_time;
	float mesh_draw_time;
};

// Push constant structure for the ray tracer
struct PushConstantRay
{
	glm::vec4  clearColor;
	glm::vec4  lightPosition;
	glm::mat4  viewInverse;
	glm::mat4  projInverse;
	float lightIntensity;
	int   lightType;
	std::uint32_t seed;
	std::uint32_t samples_done;
};

struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

class VulkanEngine {
public:

	//Vulkan instance
	VkInstance _instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface; // Vulkan window surface

	// Ray tracing features
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

	VkSwapchainKHR _swapchain; // Swapchain handle
	VkFormat _swapchainImageFormat; // Swapchain image format

	std::vector<VkImage> _swapchainImages; // Swapchain image handles
	std::vector<VkImageView> _swapchainImageViews; // Swapchain image view handles
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};
	VkExtent2D _swapchainExtent; // Swapchain image resolution

	DrawContext mainDrawContext;
	std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

	// Ray tracing accel struct + variables
	bool m_is_raytracing_supported{ false };
	std::unique_ptr<nvvk::RaytracingBuilderKHR> m_rt_builder;
	void traverseLoadedMeshNodesOnceForRT();
	void createBottomLevelAS();
	void createTopLevelAS();

	void update_scene();

	Camera mainCamera;

	EngineStats stats;

	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	bool stop_rendering{ false };
	bool useRaytracer{ false };

	VkExtent2D _windowExtent{ 1280 , 720 };

	float renderScale = 1.f;

	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() {
		return _frames[_frameNumber % FRAME_OVERLAP];
	}

	VkQueue _graphicsQueue{};
	uint32_t _graphicsQueueFamily
;

	struct SDL_Window* _window{ nullptr };

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	DescriptorAllocatorGrowable globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors{};
	VkDescriptorSetLayout _drawImageDescriptorLayout{};

	VkDescriptorSet _gbufferInputDescriptors{};
	VkDescriptorSetLayout _gbufferInputDescriptorLayout{};
	VkDescriptorSet _gbufferPosOutputDescriptor{};

	VkDescriptorSetLayout _singleImageDescriptorLayout;

	// Ray tracing descriptors
	VkDescriptorPool m_rtDescPool;
	VkDescriptorSetLayout m_rtDescSetLayout;
	VkDescriptorSet m_rtDescSet;
	VkDescriptorPool m_objDescPool;
	VkDescriptorSetLayout m_objDescSetLayout;
	VkDescriptorSet m_objDescSet;
	VkDescriptorSetLayout m_texSetLayout;
	VkDescriptorSet m_texDescSet;

	VkPipeline _gradientPipeline{};
	VkPipelineLayout _gradientPipelineLayout{};

	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;

	VkPipelineLayout _gbufferPipelineLayout;
	VkPipeline _gbufferPipeline;

	// Ray Tracing Pipeline
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
	VkPipelineLayout                                  m_rtPipelineLayout;
	VkPipeline                                        m_rtPipeline;

	// Push constant for ray tracer
	PushConstantRay m_pcRay{};

	GPUMeshBuffers rectangle;
	std::vector<std::shared_ptr<MeshAsset>> testMeshes;

	GPUSceneData sceneData;
	glm::vec4 prevSunDir;

	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& image);

	MaterialInstance defaultData;
	GLTFMetallic_Roughness metalRoughMaterial;

	//draw resources
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
	VkExtent2D _drawExtent{};

	// shadow resources
	shadowMap _shadowMap;

	//SSAO resources
	ssao _ssao;

	// Raytacing resources
	AllocatedImage _rtOutputImage;
	// Put all textures in loadScenes to a vector
	std::vector<AllocatedImage> loadedTextures;

	AllocatedBuffer m_rtSBTBuffer;
	VkStridedDeviceAddressRegionKHR m_rgenRegion{};
	VkStridedDeviceAddressRegionKHR m_missRegion{};
	VkStridedDeviceAddressRegionKHR m_hitRegion{};
	VkStridedDeviceAddressRegionKHR m_callRegion{};

	// immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;
	AllocatedImage _ssaoNoiseImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;
	VkSampler _defaultSamplerShadowDepth;
	VkSampler _gbufferSampler;

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	// Ray tracing funcs
	void createRtDescriptorSet();
	void updateRtDescriptorSet();
	void createRtPipeline();
	void createRtShaderBindingTable();
	void raytrace(const VkCommandBuffer& cmdBuf, const glm::vec4& clearColor);
	void resetSamples();
	void rtSampleUpdates();

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);
	void upload_to_vma_allocation(const void* src,
		size_t size,
		const AllocatedBuffer& dst_allocation,
		size_t dst_offset = 0);

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	void RemoveTex();

	bool resize_requested{ false };
	bool drawGBufferPositions{ false };

	// GBuffer
	void init_gbuffer();
	AllocatedImage _gbufferPosition;
	AllocatedImage _gbufferNormal;

private: 

	void draw_background(VkCommandBuffer cmd);
	void draw_geometry(VkCommandBuffer cmd);
	void draw_gbuffer(VkCommandBuffer cmd);

	void init_imgui();
	void setup_imgui_panel();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	void init_pipelines();

	void init_descriptors();

	void init_vulkan();
	void init_ray_tracing();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
	void resize_swapchain();

	void init_default_data();
};