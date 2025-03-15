#ifndef VKTYPES_H_
#define VKTYPES_H_

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <vector>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct AllocatedImage {
	VkImage       _image;
	VkImageView   _imageView;
	VmaAllocation _allocation;
	VkExtent3D    _imageExtent;
	VkFormat      _imageFormat;
};

#define VK_CHECK(x)                                                          \
	do {                                                                     \
		VkResult err = x;                                                    \
		if (err) {                                                           \
			std::println("Detected Vulkan error: {}", string_VkResult(err)); \
			abort();                                                         \
		}                                                                    \
	} while (0)

#endif /*! VKTYPES_H_ */