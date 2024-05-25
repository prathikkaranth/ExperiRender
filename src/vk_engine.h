#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_pipelines.h>
#include <deque>
#include <functional>
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

	VkSemaphore _swapchainSemaphore , _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:

	//Vulkan instance
	VkInstance _instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface; // Vulkan window surface

	VkSwapchainKHR _swapchain; // Swapchain handle
	VkFormat _swapchainImageFormat; // Swapchain image format

	std::vector<VkImage> _swapchainImages; // Swapchain image handles
	std::vector<VkImageView> _swapchainImageViews; // Swapchain image view handles
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};
	VkExtent2D _swapchainExtent; // Swapchain image resolution

	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	bool stop_rendering{ false };

	VkExtent2D _windowExtent{ 1700 , 900 };

	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() {
		return _frames[_frameNumber % FRAME_OVERLAP];
	}

	VkQueue _graphicsQueue{};
	uint32_t _graphicsQueueFamily;

	struct SDL_Window* _window{ nullptr };

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors{};
	VkDescriptorSetLayout _drawImageDescriptorLayout{};

	VkPipeline _gradientPipeline{};
	VkPipelineLayout _gradientPipelineLayout{};

	//draw resources
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent{};

	// immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private: 

	void draw_background(VkCommandBuffer cmd);

	void init_imgui();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	void init_pipelines();
	void init_background_pipelines();

	void init_descriptors();

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
};