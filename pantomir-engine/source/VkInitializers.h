#ifndef VKINITIALIZERS_H_
#define VKINITIALIZERS_H_

#include "VkTypes.h"
#include <vulkan/vulkan_core.h>

namespace vkinit {
	constexpr VkCommandPoolCreateInfo CommandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags = 0) {
		return {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext            = nullptr,
			.flags            = flags,
			.queueFamilyIndex = queueFamilyIndex
		};
	}

	constexpr VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool pool, const uint32_t count = 1) {
		return {
			.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext              = nullptr,
			.commandPool        = pool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count
		};
	}

	constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo(const VkCommandBufferUsageFlags flags = 0) {
		return {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext            = nullptr,
			.flags            = flags,
			.pInheritanceInfo = nullptr
		};
	}

	constexpr VkFenceCreateInfo FenceCreateInfo(const VkFenceCreateFlags flags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags
		};
	}

	constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo(const VkSemaphoreCreateFlags flags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags
		};
	}

	constexpr VkSemaphoreSubmitInfo SemaphoreSubmitInfo(const VkPipelineStageFlags2 stageMask, const VkSemaphore semaphore) {
		return {
			.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext       = nullptr,
			.semaphore   = semaphore,
			.value       = 1,
			.stageMask   = stageMask,
			.deviceIndex = 0
		};
	}

	constexpr VkCommandBufferSubmitInfo CommandBufferSubmitInfo(const VkCommandBuffer commandBuffer) {
		return {
			.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext         = nullptr,
			.commandBuffer = commandBuffer,
			.deviceMask    = 0
		};
	}

	constexpr VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* commandBufferSubmitInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
		return {
			.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext                    = nullptr,
			.waitSemaphoreInfoCount   = static_cast<uint32_t>(waitSemaphoreInfo ? 1 : 0),
			.pWaitSemaphoreInfos      = waitSemaphoreInfo,
			.commandBufferInfoCount   = 1,
			.pCommandBufferInfos      = commandBufferSubmitInfo,
			.signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfo ? 1 : 0),
			.pSignalSemaphoreInfos    = signalSemaphoreInfo
		};
	}

	constexpr VkPresentInfoKHR PresentInfo() {
		return {
			.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext              = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores    = nullptr,
			.swapchainCount     = 0,
			.pSwapchains        = nullptr,
			.pImageIndices      = nullptr
		};
	}

	constexpr VkRenderingAttachmentInfo AttachmentInfo(const VkImageView view, const VkClearValue* clear, const VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
		return {
			.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext       = nullptr,
			.imageView   = view,
			.imageLayout = layout,
			.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue  = clear ? *clear : VkClearValue {}
		};
	}

	constexpr VkRenderingAttachmentInfo DepthAttachmentInfo(const VkImageView view, const VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
		return {
			.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext       = nullptr,
			.imageView   = view,
			.imageLayout = layout,
			.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue  = { .depthStencil = { .depth = 0.0F, .stencil = 0 } } // Clear depth is 0.0 because we Reversed Z
		};
	}

	constexpr VkRenderingInfo RenderingInfo(const VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment) {
		return {
			.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext                = nullptr,
			.renderArea           = { .offset = { 0, 0 }, .extent = renderExtent },
			.layerCount           = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments    = colorAttachment,
			.pDepthAttachment     = depthAttachment,
			.pStencilAttachment   = nullptr
		};
	}

	constexpr VkImageSubresourceRange ImageSubresourceRange(const VkImageAspectFlags aspectMask) {
		return {
			.aspectMask     = aspectMask,
			.baseMipLevel   = 0,
			.levelCount     = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount     = VK_REMAINING_ARRAY_LAYERS
		};
	}

	constexpr VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(const VkDescriptorType type, const VkShaderStageFlags stageFlags, const uint32_t binding) {
		return {
			.binding            = binding,
			.descriptorType     = type,
			.descriptorCount    = 1,
			.stageFlags         = stageFlags,
			.pImmutableSamplers = nullptr
		};
	}

	constexpr VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding* bindings, const uint32_t bindingCount) {
		return {
			.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext        = nullptr,
			.flags        = 0,
			.bindingCount = bindingCount,
			.pBindings    = bindings
		};
	}

	constexpr VkWriteDescriptorSet WriteDescriptorImage(const VkDescriptorType type, const VkDescriptorSet descriptorSet, VkDescriptorImageInfo* imageInfo, const uint32_t binding) {
		return {
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext           = nullptr,
			.dstSet          = descriptorSet,
			.dstBinding      = binding,
			.descriptorCount = 1,
			.descriptorType  = type,
			.pImageInfo      = imageInfo
		};
	}

	constexpr VkWriteDescriptorSet WriteDescriptorBuffer(const VkDescriptorType type, const VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, const uint32_t binding) {
		return {
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext           = nullptr,
			.dstSet          = dstSet,
			.dstBinding      = binding,
			.descriptorCount = 1,
			.descriptorType  = type,
			.pBufferInfo     = bufferInfo
		};
	}

	constexpr VkDescriptorBufferInfo BufferInfo(const VkBuffer buffer, const VkDeviceSize offset, const VkDeviceSize range) {
		return {
			.buffer = buffer,
			.offset = offset,
			.range  = range
		};
	}

	constexpr VkImageCreateInfo ImageCreateInfo(const VkFormat format, const VkImageUsageFlags usageFlags, const VkExtent3D extent) {
		return {
			.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext       = nullptr,
			.imageType   = VK_IMAGE_TYPE_2D,
			.format      = format,
			.extent      = extent,
			.mipLevels   = 1,
			.arrayLayers = 1,
			.samples     = VK_SAMPLE_COUNT_1_BIT,
			.tiling      = VK_IMAGE_TILING_OPTIMAL,
			.usage       = usageFlags
		};
	}

	constexpr VkImageViewCreateInfo ImageViewCreateInfo(const VkFormat format, const VkImage image, const VkImageAspectFlags aspectFlags) {
		return {
			.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext            = nullptr,
			.image            = image,
			.viewType         = VK_IMAGE_VIEW_TYPE_2D,
			.format           = format,
			.subresourceRange = {
			    .aspectMask     = aspectFlags,
			    .baseMipLevel   = 0,
			    .levelCount     = 1,
			    .baseArrayLayer = 0,
			    .layerCount     = 1 }
		};
	}

	constexpr VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo() {
		return {
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext                  = nullptr,
			.flags                  = 0,
			.setLayoutCount         = 0,
			.pSetLayouts            = nullptr,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges    = nullptr
		};
	}

	constexpr VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderStageFlagBits stage, const VkShaderModule shaderModule, const char* entry = "main") {
		return {
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.stage  = stage,
			.module = shaderModule,
			.pName  = entry
		};
	}
} // namespace vkinit

#endif /*! VKINITIALIZERS_H_ */
