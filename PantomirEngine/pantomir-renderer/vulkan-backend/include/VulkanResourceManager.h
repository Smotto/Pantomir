#ifndef VULKANRESOURCEMANAGER_H_
#define VULKANRESOURCEMANAGER_H_

#include "VulkanBufferManager.h"
#include <memory>
#include <unordered_map>

class VulkanDeviceManager;

class VulkanResourceManager final {
public:
	VulkanResourceManager(std::shared_ptr<VulkanDeviceManager> deviceManager,
	                      std::shared_ptr<VulkanBufferManager> bufferManager);
	~VulkanResourceManager();

	std::shared_ptr<VulkanBufferManager> GetBufferManager();

	VulkanBufferManager::RenderModel*    LoadModel(const std::string& path);

private:
	std::shared_ptr<VulkanDeviceManager>                              m_deviceManager;
	std::shared_ptr<VulkanBufferManager>                              m_bufferManager;
	ModelLoader                                                       m_modelLoader;
	std::unordered_map<std::string, VulkanBufferManager::RenderModel> m_models;
};
#endif /*! VULKANRESOURCEMANAGER_H_ */