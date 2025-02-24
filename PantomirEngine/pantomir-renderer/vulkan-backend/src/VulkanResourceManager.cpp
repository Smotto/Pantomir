#include "VulkanResourceManager.h"

#include "VulkanBufferManager.h"
#include "VulkanDeviceManager.h"

VulkanResourceManager::VulkanResourceManager(std::shared_ptr<VulkanDeviceManager> deviceManager,
                                 std::shared_ptr<VulkanBufferManager> bufferManager)
    : m_deviceManager(deviceManager), m_bufferManager(bufferManager) {
}

VulkanResourceManager::~VulkanResourceManager() {
	VkDevice device = m_deviceManager->GetLogicalDevice();
	for (auto& pair : m_models) {
		VulkanBufferManager::RenderModel& model = pair.second;
		vkDestroyBuffer(device, model.indexBuffer, nullptr);
		vkFreeMemory(device, model.indexBufferMemory, nullptr);
		vkDestroyBuffer(device, model.vertexBuffer, nullptr);
		vkFreeMemory(device, model.vertexBufferMemory, nullptr);
	}
	m_models.clear();
}

std::shared_ptr<VulkanBufferManager> VulkanResourceManager::GetBufferManager() {
	return m_bufferManager;
}

VulkanBufferManager::RenderModel* VulkanResourceManager::LoadModel(const std::string& path) {
	auto it = m_models.find(path);
	if (it != m_models.end()) {
		return &it->second;
	}

	ModelLoader::RawModel            rawModel    = m_modelLoader.LoadModel(path);
	VulkanBufferManager::RenderModel renderModel = m_bufferManager->CreateRenderModel(rawModel);
	m_models[path]                               = renderModel;
	return &m_models[path];
}