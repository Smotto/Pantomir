#ifndef VULKANBUFFERMANAGER_H_
#define VULKANBUFFERMANAGER_H_

#include "ModelLoader.h"

#include <memory>
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

	VulkanBufferManager(std::shared_ptr<VulkanDeviceManager> deviceManager);
	~VulkanBufferManager();

	RenderModel   CreateRenderModel(const ModelLoader::RawModel& rawModel);

	VkCommandPool GetCommandPool() const;

private:
	std::shared_ptr<VulkanDeviceManager> m_deviceManager;
	VkCommandPool                        m_commandPool;

	uint32_t                             FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void                                 CreateCommandPool();
	void                                 CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void                                 CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	VkCommandBuffer                      BeginSingleTimeCommands();
	void                                 EndSingleTimeCommands(VkCommandBuffer commandBuffer);
};
#endif /*! VULKANBUFFERMANAGER_H_ */