#ifndef VERTEX_H_
#define VERTEX_H_

#include <array>
#include <cstddef>
#include <functional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

//---------------------------------------------------------------------
// Vertex structure with inline Vulkan descriptions and equality operator
//---------------------------------------------------------------------
struct Vertex {
	glm::vec3                              pos;
	glm::vec3                              color;
	glm::vec2                              texCoord;

	// Returns the Vulkan binding description for this vertex type
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	// Returns the Vulkan attribute descriptions for this vertex type
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		// Position attribute
		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(Vertex, pos);

		// Color attribute
		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset   = offsetof(Vertex, color);

		// Texture coordinate attribute
		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	// Equality operator used in hashing and containers
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
template <>
struct hash<Vertex> {
	std::size_t operator()(const Vertex& vertex) const noexcept {
		std::size_t seed        = 0;

		auto        hashCombine = [](std::size_t& seed, auto& value) {
            seed ^= std::hash<std::decay_t<decltype(value)>>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};
		hashCombine(seed, vertex.pos);
		hashCombine(seed, vertex.color);
		hashCombine(seed, vertex.texCoord);

		return seed;
	}
};
} // namespace std
#endif /*! VERTEX_H_ */
