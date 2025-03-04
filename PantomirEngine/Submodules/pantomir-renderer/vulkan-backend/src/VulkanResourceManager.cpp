#include "VulkanResourceManager.h"
#include "LoggerMacros.h"
#include "VulkanBufferManager.h"
#include "VulkanDeviceManager.h"
#include "Vertex.h"

#include <filesystem>
#include <xxhash.h>  // Include xxHash header

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

std::map<uint64_t, VulkanBufferManager::RenderModel>& VulkanResourceManager::GetModels() {
    return m_models;
}

uint64_t VulkanResourceManager::ComputePathHash(const std::filesystem::path& path) {
    std::string pathStr = path.string();
	XXH64_hash_t hash = XXH3_64bits_withSeed(pathStr.c_str(), pathStr.size(), 0);
    return hash;
}

void VulkanResourceManager::LoadModel(const std::filesystem::path& path) {
    LOG_DEBUG("ResourceManager", "Loading model: {}", path.string());
    uint64_t hash = ComputePathHash(path);
	if (auto it = m_models.find(hash); it != m_models.end()) {
        return;
    }

    try {
        ModelLoader::RawModel rawModel = m_modelLoader.LoadModel(path);
        VulkanBufferManager::RenderModel renderModel = m_bufferManager->CreateRenderModel(rawModel);
        auto inserted = m_models.emplace(hash, renderModel);
    } catch (const std::exception& e) {
		LOG_ERROR("VulkanResourceManager", e.what());
    	return;
    }
}

void VulkanResourceManager::UnloadModel(const std::filesystem::path& path) {
    LOG_DEBUG("ResourceManager", "Unloading model: {}", path.string());
    uint64_t hash = ComputePathHash(path);
    auto it = m_models.find(hash);
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