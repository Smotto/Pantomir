#ifndef PANTOMIRFUNCTIONLIBRARY_H_
#define PANTOMIRFUNCTIONLIBRARY_H_

#include <vulkan/vulkan_core.h>

struct PantomirFunctionLibrary {
	constexpr static float PI = 3.1415926535897932384626433832795f;
	constexpr static float GOLDEN_RATIO = 1.6180339887498948482045868343656381f;

	static size_t          BytesPerPixelFromFormat(const VkFormat format);
};

#endif /* PANTOMIRFUNCTIONLIBRARY_H_ */
