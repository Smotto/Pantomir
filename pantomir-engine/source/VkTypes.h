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

enum class MaterialPass : uint8_t {
	Opaque,
	AlphaMask,
	AlphaBlend,
	Other
};

struct MaterialPipeline {
	VkPipeline       pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance {
	MaterialPipeline*  pipeline;
	VkDescriptorSet    materialSet;
	MaterialPass       passType;
	VkCullModeFlagBits cullMode;
};

struct Vertex {
	glm::vec3 position;
	float     uv_x;
	glm::vec3 normal;
	float     uv_y;
	glm::vec4 tangent;
	glm::vec4 color;
};

struct AllocatedBuffer {
	VkBuffer          buffer;
	VmaAllocation     allocation;
	VmaAllocationInfo info;
};

// Holds the resources needed for a mesh
struct GPUMeshBuffers {
	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

// Push constants for our mesh object draws
struct GPUDrawPushConstants {
	glm::mat4       worldMatrix;
	VkDeviceAddress vertexBufferAddress;
};

struct AllocatedImage {
	VkImage       image;
	VkImageView   imageView;
	VmaAllocation allocation;
	VkExtent3D    imageExtent;
	VkFormat      imageFormat;
};

struct DrawContext;

class IRenderable {
	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// Implementation of a drawable scene node.
// The scene node can hold children and keeps a transform for propagating to them.
struct Node : IRenderable {
	// Parent pointer must be a weak pointer to avoid circular dependencies
	std::weak_ptr<Node>                _parent;
	std::vector<std::shared_ptr<Node>> _children;

	glm::mat4                          _localTransform;
	glm::mat4                          _worldTransform;

	void                               RefreshTransform(const glm::mat4& parentMatrix) {
        _worldTransform = parentMatrix * _localTransform;
        for (auto c : _children) {
            c->RefreshTransform(_worldTransform);
        }
	}

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& drawContext) {
		// Draw children
		for (auto& child : _children) {
			child->Draw(topMatrix, drawContext);
		}
	}
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