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
	// load HDRI
		// Calculate buffer size based on loaded data
	size_t faceSize = width * height * nrComponents;
	size_t totalSize = faceSize * 8;

	// Create a staging buffer
	AllocatedBuffer uploadBuffer = engine->create_buffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vmaSetAllocationName(engine->_allocator, uploadBuffer.allocation, "HDRI Upload Buffer");

	// Copy data for all faces to staging buffer
	for (int i = 0; i < 6; i++) {
		unsigned char* dstPtr = static_cast<unsigned char*>(uploadBuffer.info.pMappedData) + (i * faceSize);
		memcpy(dstPtr, cubemapData[i], faceSize);
		stbi_image_free(cubemapData[i]);
	}

	// Create destination image
	VkExtent3D imageSize = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

	VkImageCreateInfo imgInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
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
	newImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// Create the image
	VK_CHECK(vmaCreateImage(engine->_allocator, &imgInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr));
	vmaSetAllocationName(engine->_allocator, newImage.allocation, "HDRI Image");

	// Create the image view
	VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = newImage.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	vkCreateImageView(engine->_device, &viewInfo, nullptr, &newImage.imageView);

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
		std::vector<VkBufferImageCopy> copyRegions(6);
		for (uint32_t face = 0; face < 6; face++) {
			copyRegions[face].bufferOffset = face * faceSize;
			copyRegions[face].bufferRowLength = 0;  // Tightly packed
			copyRegions[face].bufferImageHeight = 0;  // Tightly packed
			copyRegions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegions[face].imageSubresource.mipLevel = 0;
			copyRegions[face].imageSubresource.baseArrayLayer = face;  // Select face
			copyRegions[face].imageSubresource.layerCount = 1;  // One layer per face
			copyRegions[face].imageExtent = imageSize;
			copyRegions[face].imageOffset = { 0, 0, 0 };
		}

		vkCmdCopyBufferToImage(
			cmd,
			uploadBuffer.buffer,
			newImage.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			6,  // 6 regions
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