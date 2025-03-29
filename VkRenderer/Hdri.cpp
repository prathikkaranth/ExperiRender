#include "Hdri.h"

#include <stb_image.h>
#include "vk_images.h"
#include <spdlog/spdlog.h>

void HDRI::init_hdriMap(VulkanEngine* engine) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	const char* hdrFilePath = "..\\assets\\HDRI\\pretoria_gardens_4k.hdr";
	float *data = stbi_loadf(hdrFilePath, &width, &height, &nrComponents, 0);

	if (data) {
		spdlog::info("Loading HDRI: {}", hdrFilePath);
		spdlog::info("HDRI dimensions: {}x{} with {} components", width, height, nrComponents);

		VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
		// loaded HDRI
		_hdriMap = vkutil::create_hdri_image(engine, data, width, height, nrComponents);

		// Sampler configs
		VkSamplerCreateInfo sampl2 = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampl2.magFilter = VK_FILTER_LINEAR;
		sampl2.minFilter = VK_FILTER_LINEAR;
		sampl2.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampl2.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampl2.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		vkCreateSampler(engine->_device, &sampl2, nullptr, &_hdriMapSampler);

		stbi_image_free(data);
	}

	else
	{
		spdlog::error("Cannot load HDRI !!!!!");
	}
}
