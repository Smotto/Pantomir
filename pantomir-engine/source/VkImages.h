#ifndef VKIMAGES_H_
#define VKIMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil {
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
};

#endif /*! VKIMAGES_H_ */
