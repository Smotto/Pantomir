#include "VkImages.h"
#include "VkInitializers.h"

void vkutil::TransitionImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
	VkImageMemoryBarrier2 imageBarrier { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext                  = nullptr;

	// src is operations must complete before the barrier
	// dst is operation must wait for the barrier
	// TODO: Fix later, this stalls the GPU pipeline because all commands are stopped, not specific ones.
	imageBarrier.srcStageMask           = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // All previous commands
	imageBarrier.srcAccessMask          = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask           = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // All future commands
	imageBarrier.dstAccessMask          = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
	imageBarrier.oldLayout              = currentLayout;
	imageBarrier.newLayout              = newLayout;

	const VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange       = vkinit::ImageSubresourceRange(aspectMask);
	imageBarrier.image                  = image;

	VkDependencyInfo depInfo {};
	depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext                   = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers    = &imageBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void vkutil::CopyImageToImage(const VkCommandBuffer commandBuffer, const VkImage source, const VkImage destination, const VkExtent2D sourceSize, const VkExtent2D destinationSize) {
	VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x               = sourceSize.width;
	blitRegion.srcOffsets[1].y               = sourceSize.height;
	blitRegion.srcOffsets[1].z               = 1;

	blitRegion.dstOffsets[1].x               = destinationSize.width;
	blitRegion.dstOffsets[1].y               = destinationSize.height;
	blitRegion.dstOffsets[1].z               = 1;

	blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount     = 1;
	blitRegion.srcSubresource.mipLevel       = 0;

	blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount     = 1;
	blitRegion.dstSubresource.mipLevel       = 0;

	VkBlitImageInfo2 blitInfo { .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage       = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage       = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter         = VK_FILTER_LINEAR;
	blitInfo.regionCount    = 1;
	blitInfo.pRegions       = &blitRegion;

	// We can use cmdcopyimage, or cmdblitimage. 2 main ways to copy in vulkan for images.
	vkCmdBlitImage2(commandBuffer, &blitInfo);
}

void vkutil::GenerateMipmaps(const VkCommandBuffer commandBuffer, const VkImage image, VkExtent2D imageSize) {
	const int mipLevels = static_cast<int>(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
	for (int mip = 0; mip < mipLevels; mip++) {
		VkExtent2D halfSize = imageSize;
		halfSize.width /= 2;
		halfSize.height /= 2;

		VkImageMemoryBarrier2 imageBarrier { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

		imageBarrier.srcStageMask                  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask                 = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask                  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask                 = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageAspectFlags aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange              = vkinit::ImageSubresourceRange(aspectMask);
		imageBarrier.subresourceRange.levelCount   = 1;
		imageBarrier.subresourceRange.baseMipLevel = mip;
		imageBarrier.image                         = image;

		VkDependencyInfo depInfo { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers    = &imageBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		if (mip < mipLevels - 1) {
			VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

			blitRegion.srcOffsets[1].x               = imageSize.width;
			blitRegion.srcOffsets[1].y               = imageSize.height;
			blitRegion.srcOffsets[1].z               = 1;

			blitRegion.dstOffsets[1].x               = halfSize.width;
			blitRegion.dstOffsets[1].y               = halfSize.height;
			blitRegion.dstOffsets[1].z               = 1;

			blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount     = 1;
			blitRegion.srcSubresource.mipLevel       = mip;

			blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount     = 1;
			blitRegion.dstSubresource.mipLevel       = mip + 1;

			VkBlitImageInfo2 blitInfo { .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
			blitInfo.dstImage       = image;
			blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			blitInfo.srcImage       = image;
			blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			blitInfo.filter         = VK_FILTER_LINEAR;
			blitInfo.regionCount    = 1;
			blitInfo.pRegions       = &blitRegion;

			vkCmdBlitImage2(commandBuffer, &blitInfo);

			imageSize = halfSize;
		}
	}

	// TODO: Do a manual optimized transition instead.
	// Transition all mip levels into the final read_only layout
	TransitionImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}