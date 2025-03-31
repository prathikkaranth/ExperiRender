#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vk_initializers.h"
#include "vk_images.h"

AllocatedImage vkutil::create_image(VulkanEngine* engine, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocated images on dedicated GPU memory
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(engine->_allocator, &img_info, &allocInfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct 
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image view for the image
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(engine->_device, &view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage vkutil::create_image(VulkanEngine* engine, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	const size_t pixel_size = format == VK_FORMAT_R32G32B32A32_SFLOAT ? 16 : 4;
	size_t data_size = size.depth * size.width * size.height * pixel_size;
	AllocatedBuffer uploadbuffer = engine->create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vmaSetAllocationName(engine->_allocator, uploadbuffer.allocation, "Image Upload Buffer");

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = create_image(engine, size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);
	vmaSetAllocationName(engine->_allocator, new_image.allocation, "Image Allocation");

	engine->immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		if (mipmapped) {
			vkutil::generate_mipmaps(cmd, new_image.image, VkExtent2D{ new_image.imageExtent.width,new_image.imageExtent.height });
		}
		else {
			vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		});
	engine->destroy_buffer(uploadbuffer);
	return new_image;
}

AllocatedImage vkutil::create_hdri_image(VulkanEngine* engine, std::vector<unsigned char*> cubemapData, int width, int height, int nrComponents)
{
	// Calculate buffer size based on loaded data and format
	VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

	// Calculate source size based on nrComponents from stb_image
	size_t srcBytesPerPixel = nrComponents;
	size_t srcFaceSize = width * height * srcBytesPerPixel;

	// Calculate destination size for the R16G16B16A16_SFLOAT format
	size_t dstBytesPerPixel = 8; // 16-bit per channel × 4 channels / 8 bits per byte
	size_t dstFaceSize = width * height * dstBytesPerPixel;
	size_t totalSize = dstFaceSize * 6;

	// Debug output
	printf("Image dimensions: %d x %d with %d components\n", width, height, nrComponents);
	printf("Source bytes per pixel: %zu\n", srcBytesPerPixel);
	printf("Source face size: %zu bytes\n", srcFaceSize);
	printf("Destination bytes per pixel: %zu\n", dstBytesPerPixel);
	printf("Destination face size: %zu bytes\n", dstFaceSize);
	printf("Total buffer size: %zu bytes\n", totalSize);

	// Create destination image structure first, before any memory operations
	VkExtent3D imageSize = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

	// Setup image create info
	VkImageCreateInfo imgInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.format = format;
	imgInfo.extent = imageSize;
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = 6;
	imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	AllocatedImage newImage;
	newImage.imageExtent = imageSize;
	newImage.imageFormat = format;

	// Create a staging buffer - make sure we're using the correct size
	AllocatedBuffer uploadBuffer = engine->create_buffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vmaSetAllocationName(engine->_allocator, uploadBuffer.allocation, "HDRI Upload Buffer");

	// Verify the buffer was created with the correct size
	printf("Upload buffer allocated size: %zu\n", uploadBuffer.info.size);

	// Verify buffer is mapped
	if (uploadBuffer.info.pMappedData == nullptr) {
		printf("ERROR: Upload buffer not properly mapped!\n");
		// Map it explicitly if not mapped
		void* mappedData = nullptr;
		VK_CHECK(vmaMapMemory(engine->_allocator, uploadBuffer.allocation, &mappedData));
		uploadBuffer.info.pMappedData = mappedData;
	}

	// Process each face - using a more cautious approach
	std::vector<VkBufferImageCopy> copyRegions;
	copyRegions.reserve(6);

	// Initialize the buffer to zeros for safety
	memset(uploadBuffer.info.pMappedData, 0, totalSize);

	for (int i = 0; i < 6; i++) {
		// Skip faces with no data
		if (cubemapData[i] == nullptr) {
			printf("Face %d: NULL data, skipping\n", i);
			continue;
		}

		printf("Processing face %d\n", i);

		size_t dstOffset = i * dstFaceSize;
		if (dstOffset + dstFaceSize > uploadBuffer.info.size) {
			printf("ERROR: Face %d would exceed buffer bounds! Offset: %zu, Size: %zu, Buffer size: %zu\n",
				i, dstOffset, dstFaceSize, uploadBuffer.info.size);
			continue;  // Skip this face
		}

		unsigned char* dstPtr = static_cast<unsigned char*>(uploadBuffer.info.pMappedData) + dstOffset;

		// Direct copy would be unsafe if formats don't match
		// Always do a pixel-by-pixel conversion for safety
		printf("Converting data for face %d\n", i);

		// Use a simple conversion loop - one pixel at a time
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				size_t srcIdx = (y * width + x) * srcBytesPerPixel;
				size_t dstIdx = (y * width + x) * dstBytesPerPixel;

				// Ensure we don't read past the end of source data
				if (srcIdx + srcBytesPerPixel > srcFaceSize) {
					printf("ERROR: Source index out of bounds at pixel (%d,%d)\n", x, y);
					break;
				}

				// Ensure we don't write past the end of destination buffer
				if (dstIdx + dstBytesPerPixel > dstFaceSize) {
					printf("ERROR: Destination index out of bounds at pixel (%d,%d)\n", x, y);
					break;
				}

				// Convert source to 16-bit float format
				for (int c = 0; c < 4; c++) {
					// Get the float value (0.0 to 1.0)
					float floatValue = 0.0f;
					if (c < nrComponents) {
						// Convert 8-bit to float (0-255 to 0.0-1.0)
						floatValue = cubemapData[i][srcIdx + c] / 255.0f;
					}
					else if (c == 3) {
						// Alpha channel - set to 1.0 if not in source
						floatValue = 1.0f;
					}

					// Convert float to half-float (16-bit)
					uint32_t x = *reinterpret_cast<uint32_t*>(&floatValue);
					uint16_t h = ((x >> 16) & 0x8000) | // sign bit
						((((x & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | // exponential
						((x >> 13) & 0x03ff); // mantissa
					uint16_t half = h;

					// Write to destination
					uint16_t* dstPixel = reinterpret_cast<uint16_t*>(dstPtr + dstIdx + c * 2);
					*dstPixel = half;
				}
			}
		}

		printf("Conversion completed for face %d\n", i);

		// Free source data after conversion
		stbi_image_free(cubemapData[i]);
		cubemapData[i] = nullptr;

		// Setup copy region for this face
		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = dstOffset;
		copyRegion.bufferRowLength = 0;  // Tightly packed
		copyRegion.bufferImageHeight = 0;  // Tightly packed
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = i;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageSize;
		copyRegion.imageOffset = { 0, 0, 0 };

		copyRegions.push_back(copyRegion);
	}

	printf("Processed %zu valid faces\n", copyRegions.size());

	// Always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// Allocate and create the image
	VK_CHECK(vmaCreateImage(engine->_allocator, &imgInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr));
	vmaSetAllocationName(engine->_allocator, newImage.allocation, "HDRI Cubemap Image");

	// Create the image view for cubemap
	VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = newImage.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	VK_CHECK(vkCreateImageView(engine->_device, &viewInfo, nullptr, &newImage.imageView));

	// Only proceed with upload if we have valid data
	if (!copyRegions.empty()) {
		// Upload the image data and transition layout
		engine->immediate_submit([&](VkCommandBuffer cmd) {
			// Transition image to transfer destination layout
			VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = newImage.image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 6;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmd,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			// Copy the buffer to each face of the cubemap
			vkCmdCopyBufferToImage(
				cmd,
				uploadBuffer.buffer,
				newImage.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(copyRegions.size()),
				copyRegions.data()
			);

			// Transition to shader read layout
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
			});
	}
	else {
		printf("WARNING: No valid faces to upload!\n");
	}

	// Clean up the staging buffer
	engine->destroy_buffer(uploadBuffer);

	return newImage;
}

void vkutil::destroy_image(VulkanEngine* engine, const AllocatedImage& image)
{
	vkDestroyImageView(engine->_device, image.imageView, nullptr);
	vmaDestroyImage(engine->_allocator, image.image, image.allocation);
}

void vkutil::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkImageAspectFlags mask)
{
	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    /*VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;*/
	VkImageAspectFlags aspectMask = mask;
    imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vkutil::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize, VkFilter filter, VkImageAspectFlags mask)
{
	VkImageBlit2 blitRegion{};
	blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	blitRegion.pNext = nullptr;

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = mask;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = mask;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{};
	blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	blitInfo.pNext = nullptr;
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = filter;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

void vkutil::generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize)
{
	int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
	for (int mip = 0; mip < mipLevels; mip++) {

		VkExtent2D halfSize = imageSize;
		halfSize.width /= 2;
		halfSize.height /= 2;

		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = mip;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);

		if (mip < mipLevels - 1) {
			VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

			blitRegion.srcOffsets[1].x = imageSize.width;
			blitRegion.srcOffsets[1].y = imageSize.height;
			blitRegion.srcOffsets[1].z = 1;

			blitRegion.dstOffsets[1].x = halfSize.width;
			blitRegion.dstOffsets[1].y = halfSize.height;
			blitRegion.dstOffsets[1].z = 1;

			blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = mip;

			blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = mip + 1;

			VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
			blitInfo.dstImage = image;
			blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			blitInfo.srcImage = image;
			blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			blitInfo.filter = VK_FILTER_LINEAR;
			blitInfo.regionCount = 1;
			blitInfo.pRegions = &blitRegion;

			vkCmdBlitImage2(cmd, &blitInfo);

			imageSize = halfSize;
		}
	}

	// transition all mip levels into the final read_only layout
	transition_image(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
}