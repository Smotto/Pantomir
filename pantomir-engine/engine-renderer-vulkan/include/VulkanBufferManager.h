#ifndef VULKANBUFFERMANAGER_H_
#define VULKANBUFFERMANAGER_H_

#include "ModelLoader.h"

#include <vulkan/vulkan.h>

class VulkanDeviceManager;

class VulkanBufferManager final {
public:
	// TODO: Resource manager has to destroy these for each model. I don't like this abstraction.
	struct RenderModel {
		VkBuffer       vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer       indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t       indexCount;
	};

	explicit VulkanBufferManager(const VulkanDeviceManager* deviceManager);
	~VulkanBufferManager();

	RenderModel CreateRenderModel(const ModelLoader::RawModel& rawModel);

	[[nodiscard]] VkCommandPool GetCommandPool() const;

private:
	const VulkanDeviceManager* m_deviceManager;
	VkCommandPool              m_commandPool = VK_NULL_HANDLE;

	[[nodiscard]] uint32_t        FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void                          CreateCommandPool();
	void                          CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void                          CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	[[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
	void                          EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
};
#endif /*! VULKANBUFFERMANAGER_H_ */