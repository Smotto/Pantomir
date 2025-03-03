#include "VulkanResourceManager.h"

#include "VulkanBufferManager.h"
#include "VulkanDeviceManager.h"

#include <filesystem>

VulkanResourceManager::VulkanResourceManager(const VulkanDeviceManager* deviceManager, VulkanBufferManager* bufferManager)
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

VulkanBufferManager* VulkanResourceManager::GetBufferManager() const {
	return m_bufferManager;
}

VulkanBufferManager::RenderModel* VulkanResourceManager::LoadModel(const std::filesystem::path& path) {
	std::string relativePath = path.string();
	auto it = m_models.find(relativePath);
	if (it != m_models.end()) {
		return &it->second;
	}

	try {
		ModelLoader::RawModel rawModel = m_modelLoader.LoadModel(path);
		VulkanBufferManager::RenderModel renderModel = m_bufferManager->CreateRenderModel(rawModel);
		return &m_models.emplace(relativePath, renderModel).first->second; // emplace avoids unnecessary copies
	} catch (const std::exception& e) {
		return nullptr;
	}
}

void VulkanResourceManager::UnloadModel(const std::filesystem::path& path) {
	std::string relativePath = path.string();
	auto it = m_models.find(relativePath);
	if (it != m_models.end()) {
		VkDevice device = m_deviceManager->GetLogicalDevice();
		VulkanBufferManager::RenderModel& model = it->second;
		vkDestroyBuffer(device, model.indexBuffer, nullptr);
		vkFreeMemory(device, model.indexBufferMemory, nullptr);
		vkDestroyBuffer(device, model.vertexBuffer, nullptr);
		vkFreeMemory(device, model.vertexBufferMemory, nullptr);
		m_models.erase(it);
	}
}