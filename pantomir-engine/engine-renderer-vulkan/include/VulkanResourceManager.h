#ifndef VULKANRESOURCEMANAGER_H_
#define VULKANRESOURCEMANAGER_H_

#include "VulkanBufferManager.h"
#include <map>

class std::filesystem::path;

class VulkanDeviceManager;

class VulkanResourceManager final {
public:
	VulkanResourceManager(const VulkanDeviceManager* deviceManager,
	                      VulkanBufferManager*       bufferManager);
	~VulkanResourceManager();

	[[nodiscard]] VulkanBufferManager*                                       GetBufferManager() const;
	[[nodiscard]] std::map<std::uint64_t, VulkanBufferManager::RenderModel>& GetModels();
	uint64_t                                                                 ComputePathHash(const std::filesystem::path& path);
	void                                                                     LoadModel(const std::filesystem::path& path);
	void                                                                     UnloadModel(const std::filesystem::path& path);

private:
	const VulkanDeviceManager*                           m_deviceManager;
	VulkanBufferManager*                                 m_bufferManager;
	ModelLoader                                          m_modelLoader;
	std::map<uint64_t, VulkanBufferManager::RenderModel> m_models;
};
#endif /*! VULKANRESOURCEMANAGER_H_ */