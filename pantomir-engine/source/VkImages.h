#ifndef VKIMAGES_H_
#define VKIMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil
{
	void TransitionImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage source, VkImage destination, VkExtent2D sourceSize, VkExtent2D destinationSize);
	void GenerateMipmaps(const VkCommandBuffer commandBuffer, const VkImage image, VkExtent2D imageSize);
} // namespace vkutil

#endif /*! VKIMAGES_H_ */
