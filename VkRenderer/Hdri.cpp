#include "Hdri.h"

#include <stb_image.h>
#include "vk_images.h"
#include <spdlog/spdlog.h>

void HDRI::init_hdriMap(VulkanEngine* engine) {

	// Create an array of the six cubemap face filenames
	std::vector<const char*> filenames = {
		"..\\assets\\HDRI\\Standard-Cube-Map\\px.png", // Positive X
		"..\\assets\\HDRI\\Standard-Cube-Map\\nx.png", // Negative X
		"..\\assets\\HDRI\\Standard-Cube-Map\\py.png", // Positive Y
		"..\\assets\\HDRI\\Standard-Cube-Map\\ny.png", // Negative Y
		"..\\assets\\HDRI\\Standard-Cube-Map\\pz.png", // Positive Z
		"..\\assets\\HDRI\\Standard-Cube-Map\\nz.png"  // Negative Z
	};

	// Prepare a vector to hold the image data for each face
	std::vector<unsigned char*> cubemapData(6, nullptr);
	int width, height, nrComponents;

	// Load each face of the cubemap
	for (int i = 0; i < 6; i++) {
		// You might want to set flip to false for cubemaps
		stbi_set_flip_vertically_on_load(false);

		// Load the image
		cubemapData[i] = stbi_load(filenames[i], &width, &height, &nrComponents, 0);

		if (!cubemapData[i]) {
			spdlog::error("Failed to load cubemap face: {}", filenames[i]);
			// Free any already loaded faces
			for (int j = 0; j < i; j++) {
				stbi_image_free(cubemapData[j]);
			}
			return; // Or handle the error as appropriate
		}

		spdlog::info("Loaded cubemap face {}: {}x{} with {} components",
			i, width, height, nrComponents);
	}

	// Use the loaded data...
	_hdriMap = vkutil::create_hdri_image(engine, cubemapData, width, height, nrComponents);

	if (_hdriMap.image == VK_NULL_HANDLE) {
		spdlog::error("Failed to initialize cubemap!");
	}
	else {
		spdlog::info("Cubemap initialized successfully");
	}
	
	// Sampler configs
	VkSamplerCreateInfo sampl2 = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampl2.magFilter = VK_FILTER_LINEAR;
	sampl2.minFilter = VK_FILTER_LINEAR;
	sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampl2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	vkCreateSampler(engine->_device, &sampl2, nullptr, &_hdriMapSampler);

	engine->_mainDeletionQueue.push_function([=]() {
		
		vkDestroySampler(engine->_device, _hdriMapSampler, nullptr);
		vkutil::destroy_image(engine, _hdriMap);
		});
}
