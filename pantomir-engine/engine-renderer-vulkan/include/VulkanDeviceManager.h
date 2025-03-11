#ifndef VULKANDEVICEMANAGER_H_
#define VULKANDEVICEMANAGER_H_

#include "QueueFamilyIndices.h"
#include "VulkanInstanceManager.h"
#include "VulkanRenderer.h"

#include <vector>
#include <vulkan/vulkan.h>

class VulkanInstanceManager;
class VulkanSurface;

struct DeviceRequirements {
	VkPhysicalDeviceFeatures   requiredFeatures;
	VkPhysicalDeviceProperties desiredProperties;
};

class VulkanDeviceManager final {
public:
	VulkanDeviceManager(const VulkanInstanceManager* instanceManager, const std::vector<const char*>& deviceExtensions);
	~VulkanDeviceManager();

	[[nodiscard]] const VulkanInstanceManager* GetInstanceManager() const {
		return m_vulkanInstanceManager;
	}

	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const {
		return m_physicalDevice;
	}

	[[nodiscard]] VkDevice GetLogicalDevice() const {
		return m_logicalDevice;
	}

	[[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices() const {
		return m_queueFamilyIndices;
	}

	[[nodiscard]] VkQueue GetGraphicsQueue() const {
		return m_graphicsQueue;
	}

	[[nodiscard]] VkQueue GetPresentQueue() const {
		return m_presentQueue;
	}

	[[nodiscard]] VkSampleCountFlagBits GetMsaaSamples() const {
		return m_msaaSamples;
	}

	SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice physicalDevice) const;

private:
	void                                PickBestPhysicalDevice();
	void                                CreateLogicalDevice();
	void                                CreateQueues();

	QueueFamilyIndices                  FindQueueFamilies(const VkPhysicalDevice physicalDevice) const;
	[[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;
	uint32_t                            RateDeviceSuitability(const VkPhysicalDevice physicalDevice) const;
	bool                                IsDeviceSuitable(const VkPhysicalDevice physicalDevice) const;
	bool                                CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice) const;

	const VulkanInstanceManager*        m_vulkanInstanceManager;
	std::vector<const char*>            m_deviceExtensions = {};

	std::vector<VkPhysicalDevice>       m_physicalDevices;
	VkPhysicalDevice                    m_physicalDevice{};
	VkDevice                            m_logicalDevice{};

	QueueFamilyIndices                  m_queueFamilyIndices;
	VkQueue                             m_graphicsQueue{};
	VkQueue                             m_presentQueue{};

	VkSampleCountFlagBits               m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

#endif /*! VULKANDEVICEMANAGER_H_ */