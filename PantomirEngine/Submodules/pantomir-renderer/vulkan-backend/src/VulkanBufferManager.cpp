#include "VulkanBufferManager.h"

#include "Vertex.h"
#include "VulkanDeviceManager.h"

#include <stdexcept>

VulkanBufferManager::VulkanBufferManager(const VulkanDeviceManager* deviceManager)
    : m_deviceManager(deviceManager) {
	CreateCommandPool();
}
VulkanBufferManager::~VulkanBufferManager() {
	vkDestroyCommandPool(m_deviceManager->GetLogicalDevice(), m_commandPool, nullptr);
}

VulkanBufferManager::RenderModel VulkanBufferManager::CreateRenderModel(const ModelLoader::RawModel& rawModel) {
	RenderModel    model            = {};

	// Vertex buffer
	VkDeviceSize   vertexBufferSize = rawModel.vertices.size() * sizeof(Vertex);
	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
	memcpy(data, rawModel.vertices.data(), vertexBufferSize);
	vkUnmapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory);

	CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model.vertexBuffer, model.vertexBufferMemory);
	CopyBuffer(stagingBuffer, model.vertexBuffer, vertexBufferSize);

	vkDestroyBuffer(m_deviceManager->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, nullptr);

	// Index buffer
	VkDeviceSize indexBufferSize = rawModel.indices.size() * sizeof(uint32_t);
	CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	vkMapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
	memcpy(data, rawModel.indices.data(), indexBufferSize);
	vkUnmapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory);

	CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model.indexBuffer, model.indexBufferMemory);
	CopyBuffer(stagingBuffer, model.indexBuffer, indexBufferSize);

	vkDestroyBuffer(m_deviceManager->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, nullptr);

	model.indexCount = static_cast<uint32_t>(rawModel.indices.size());
	return model;
}

void VulkanBufferManager::CreateCommandPool() {
	QueueFamilyIndices      queueFamilyIndices = m_deviceManager->FindQueueFamilies(m_deviceManager->GetPhysicalDevice());
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_deviceManager->GetLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

VkCommandPool VulkanBufferManager::GetCommandPool() const {
	return m_commandPool;
}

uint32_t VulkanBufferManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_deviceManager->GetPhysicalDevice(), &memProperties);

	for (uint32_t index = 0; index < memProperties.memoryTypeCount; index++) {
		if ((typeFilter & (1 << index)) && (memProperties.memoryTypes[index].propertyFlags & properties) == properties) {
			return index;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanBufferManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Create a handle (an object) that defines what kind of data you'll be storing, how to use it, and size.
	if (vkCreateBuffer(m_deviceManager->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) // Create in GPU Memory, the VkBufferView holds this VkBuffer.
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_deviceManager->GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	// TODO: You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative
	if (vkAllocateMemory(m_deviceManager->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) // Reserving a block of GPU Memory
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_deviceManager->GetLogicalDevice(), buffer, bufferMemory, 0);
}

void VulkanBufferManager::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy    copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanBufferManager::BeginSingleTimeCommands() const {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_deviceManager->GetLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}
void VulkanBufferManager::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(m_deviceManager->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_deviceManager->GetGraphicsQueue());

	vkFreeCommandBuffers(m_deviceManager->GetLogicalDevice(), m_commandPool, 1, &commandBuffer);
}