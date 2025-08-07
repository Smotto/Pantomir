#ifndef PANTOMIRFUNCTIONLIBRARY_H_
#define PANTOMIRFUNCTIONLIBRARY_H_

#include <vulkan/vulkan_core.h>

struct PantomirFunctionLibrary {
	static size_t BytesPerPixelFromFormat(const VkFormat format);
};

#endif /* PANTOMIRFUNCTIONLIBRARY_H_ */
