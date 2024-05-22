#pragma once

#include <vk_types.h>

#include "VkBootstrap.h"

struct FrameData {
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
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
	VkExtent2D _swapchainExtent; // Swapchain image resolution

	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	VkExtent2D _windowExtent{ 1700 , 900 };

	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() {
		return _frames[_frameNumber % FRAME_OVERLAP];
	}

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	struct SDL_Window* _window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private: 

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
};