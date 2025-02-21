#ifndef VULKANDEVICE_H_
#define VULKANDEVICE_H_

#include "QueueFamilyIndices.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanSurface;
class VulkanInstance;

struct DeviceRequirements {
	VkPhysicalDeviceFeatures   requiredFeatures;
	VkPhysicalDeviceProperties desiredProperties;
};

class VulkanDevice final {
public:
	VulkanDevice(
	    const std::shared_ptr<VulkanInstance>& instance,
	    const VkSurfaceKHR                     surface,
	    const std::vector<const char*>&        deviceExtensions);
	~VulkanDevice();

	void             Cleanup();

	VkPhysicalDevice GetPhysicalDevice() const {
		return m_physicalDevice;
	}
	VkDevice GetLogicalDevice() const {
		return m_logicalDevice;
	}
	QueueFamilyIndices GetQueueFamilyIndices() const {
		return m_queueFamilyIndices;
	}
	VkQueue GetGraphicsQueue() const {
		return m_graphicsQueue;
	}
	VkQueue GetPresentQueue() const {
		return m_presentQueue;
	}
	VkSampleCountFlagBits GetMsaaSamples() const {
		return m_msaaSamples;
	}

	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device) const;

private:
	void                          Create();

	void                          PickPhysicalDevice();
	void                          CreateLogicalDevice();
	VkSampleCountFlagBits         GetMaxUsableSampleCount();
	bool                          IsDeviceSuitable(const VkPhysicalDevice device) const;
	uint32_t                      RateDeviceSuitability(const VkPhysicalDevice device) const;
	bool                          CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

	std::weak_ptr<VulkanInstance> m_vulkanInstance;
	const VkSurfaceKHR            m_vulkanSurface;
	VkPhysicalDevice              m_physicalDevice = VK_NULL_HANDLE;
	VkDevice                      m_logicalDevice  = VK_NULL_HANDLE;
	std::vector<const char*>      m_deviceExtensions;
	QueueFamilyIndices            m_queueFamilyIndices;
	VkQueue                       m_graphicsQueue;
	VkQueue                       m_presentQueue;
	VkSampleCountFlagBits         m_msaaSamples;
};

#endif /*! VULKANDEVICE_H_ */
