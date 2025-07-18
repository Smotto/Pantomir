#ifndef VKIMAGES_H_
#define VKIMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil {
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
	void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
} // namespace vkutil

#endif /*! VKIMAGES_H_ */
