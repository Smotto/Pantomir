#ifndef VULKANRESOURCEMANAGER_H_
#define VULKANRESOURCEMANAGER_H_

#include "VulkanBufferManager.h"
#include <memory>
#include <unordered_map>

namespace std::filesystem {
class path;
}

class VulkanDeviceManager;

class VulkanResourceManager final {
public:
	VulkanResourceManager(const VulkanDeviceManager* deviceManager,
	                      VulkanBufferManager*       bufferManager);
	~VulkanResourceManager();

	VulkanBufferManager*              GetBufferManager() const;
	VulkanBufferManager::RenderModel* LoadModel(const std::filesystem::path& path);

private:
	const VulkanDeviceManager*                                        m_deviceManager;
	VulkanBufferManager*                                              m_bufferManager;
	ModelLoader                                                       m_modelLoader;
	std::unordered_map<std::string, VulkanBufferManager::RenderModel> m_models;
};
#endif /*! VULKANRESOURCEMANAGER_H_ */