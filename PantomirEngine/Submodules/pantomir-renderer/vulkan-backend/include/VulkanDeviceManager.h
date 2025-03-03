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
	~VulkanDeviceManager();
	VulkanDeviceManager(const VulkanInstanceManager* instanceManager, const std::vector<const char*>& deviceExtensions);

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

	QueueFamilyIndices      FindQueueFamilies(const VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device) const;

private:
	void                          PickPhysicalDevice();
	void                          CreateLogicalDevice();

	VkSampleCountFlagBits         GetMaxUsableSampleCount();
	VkPhysicalDevice              ChooseBestPhysicalDevice();
	uint32_t                      RateDeviceSuitability(const VkPhysicalDevice device) const;
	bool                          IsDeviceSuitable(const VkPhysicalDevice device) const;
	bool                          CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

	const VulkanInstanceManager*  m_vulkanInstanceManager;
	std::vector<const char*>      m_deviceExtensions = {};

	std::vector<VkPhysicalDevice> m_physicalDevices;
	VkPhysicalDevice              m_physicalDevice;
	VkDevice                      m_logicalDevice;

	QueueFamilyIndices            m_queueFamilyIndices;
	VkQueue                       m_graphicsQueue;
	VkQueue                       m_presentQueue;

	VkSampleCountFlagBits         m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

#endif /*! VULKANDEVICEMANAGER_H_ */