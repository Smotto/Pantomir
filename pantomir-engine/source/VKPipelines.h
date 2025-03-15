#ifndef VKPIPELINES_H_
#define VKPIPELINES_H_

#include "VkTypes.h"

namespace vkutil {
	bool LoadShaderModule(const char*     filePath,
	                      VkDevice        device,
	                      VkShaderModule* outShaderModule);
}

#endif /*! VKPIPELINES_H_ */
