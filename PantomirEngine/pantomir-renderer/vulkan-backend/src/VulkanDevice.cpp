#include "VulkanDevice.h"
#include "VulkanInstance.h"

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

VulkanDevice::VulkanDevice(const std::shared_ptr<VulkanInstance>& instance, const VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions)
    : m_vulkanInstance(instance), m_vulkanSurface(surface), m_deviceExtensions(deviceExtensions), m_graphicsQueue(nullptr), m_presentQueue(nullptr), m_msaaSamples() {
	Create();
}
VulkanDevice::~VulkanDevice() {
}

void VulkanDevice::Create() {
	PickPhysicalDevice(); // TODO: Maybe this should be a level higher because it's dealing with physical device compatibility, as we can have multiple logical devices later.
	CreateLogicalDevice();
}

void VulkanDevice::CreateLogicalDevice() {
	m_queueFamilyIndices = FindQueueFamilies(m_physicalDevice);
	std::set<uint32_t>                   uniqueQueueFamilies = {m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentFamily.value()};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float                                queuePriority       = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount       = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos       = queueCreateInfos.data();
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error(std::string(__func__) + "Failed to create logical device!");
	}

	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
}

void VulkanDevice::Cleanup() {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void VulkanDevice::PickPhysicalDevice() {
	std::shared_ptr<VulkanInstance> vulkanInstance = m_vulkanInstance.lock();
	if (!vulkanInstance) {
		throw std::runtime_error(std::string(__func__) + ": Required Vulkan object is no longer available!");
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance->GetNativeInstance(), &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error(std::string(__func__) + "Failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vulkanInstance->GetNativeInstance(), &deviceCount, devices.data());

	// TODO: ????? Why pick one if we are going to pick the best one already :|
	// First try to pick the first suitable one
	for (const auto &device : devices) {
		if (IsDeviceSuitable(device)) {
			m_physicalDevice = device;
			m_msaaSamples    = GetMaxUsableSampleCount();
			break;
		}
	}
	
	// Score devices and choose the best candidate.
	std::multimap<uint32_t, VkPhysicalDevice> candidates;
	for (const auto& device : devices) {
		uint32_t score = RateDeviceSuitability(device);
		candidates.insert({score, device});
	}
	if (candidates.rbegin()->first > 0) {
		m_physicalDevice = candidates.rbegin()->second;
	} else {
		throw std::runtime_error(std::string(__func__) + "Failed to find a suitable GPU!");
	}
}

uint32_t VulkanDevice::RateDeviceSuitability(const VkPhysicalDevice device) const {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	uint32_t score = 0;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}
	score += deviceProperties.limits.maxImageDimension2D;
	if (!deviceFeatures.geometryShader) {
		return 0;
	}
	return score;
}

bool VulkanDevice::IsDeviceSuitable(const VkPhysicalDevice device) const {
	QueueFamilyIndices       indices             = FindQueueFamilies(device); // TODO: maybe use a getter instead
	bool                     extensionsSupported = CheckDeviceExtensionSupport(device);
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	return indices.IsComplete() && extensionsSupported && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(const VkPhysicalDevice device) const {
	QueueFamilyIndices indices;
	uint32_t           queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int familyIndex = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = familyIndex;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, m_vulkanSurface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = familyIndex;
		}
		if (indices.IsComplete()) {
			break;
		}
		familyIndex++;
	}
	return indices;
}

VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	}

	return VK_SAMPLE_COUNT_1_BIT;
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

