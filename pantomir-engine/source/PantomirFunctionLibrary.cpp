#include "PantomirFunctionLibrary.h"

#include <stdexcept>

size_t PantomirFunctionLibrary::BytesPerPixelFromFormat(const VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8G8B8A8_UNORM:
			return 4; // 4 channels, 1 byte each
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 12; // 3 channels, 4 bytes each
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 8;
		case VK_FORMAT_R32_SFLOAT:
			return 4;
		default:
			throw std::runtime_error("Unhandled format in BytesPerPixelFromFormat");
	}
}