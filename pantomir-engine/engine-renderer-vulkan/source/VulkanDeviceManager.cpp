#include "VulkanDeviceManager.h"

#include "LoggerMacros.h"
#include "VulkanInstanceManager.h"
#include "VulkanRenderer.h"

#include <map>
#include <set>
#include <stdexcept>

VulkanDeviceManager::VulkanDeviceManager(const VulkanInstanceManager* instanceManager, const std::vector<const char*>& deviceExtensions)
    : m_vulkanInstanceManager(instanceManager), m_deviceExtensions(deviceExtensions) {
	PickBestPhysicalDevice();
	CreateLogicalDevice();
	CreateQueues();
}

VulkanDeviceManager::~VulkanDeviceManager() {
	if (m_logicalDevice != VK_NULL_HANDLE) {
		vkDestroyDevice(m_logicalDevice, nullptr);
		m_logicalDevice = VK_NULL_HANDLE;
	}
}

void VulkanDeviceManager::PickBestPhysicalDevice() {
	uint32_t   deviceCount = 0;
	VkInstance vkInstance  = m_vulkanInstanceManager->GetNativeInstance();
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	m_physicalDevices.resize(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, m_physicalDevices.data());

	// First try to pick the first suitable one
	for (const auto& device : m_physicalDevices) {
		if (IsDeviceSuitable(device)) {
			m_physicalDevice = device;
			m_msaaSamples    = GetMaxUsableSampleCount();
			break;
		}
	}

	// Then choose the best candidate by scoring them
	std::multimap<int, VkPhysicalDevice> candidates;
	for (const auto& device : m_physicalDevices) {
		int score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		m_physicalDevice = candidates.rbegin()->second;
	} else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}


/*
========================
VulkanRenderer::CreateLogicalDevice
VkDevice : The “logical” GPU context that you actually execute things on.
VkQueue : Execution “port” for commands. GPUs will have a set of queues with different properties.
	Some allow only graphics commands, others only allow memory commands, etc.
	Command buffers are executed by submitting them into a queue, which will copy the rendering commands onto the GPU for execution.
========================
*/
void VulkanDeviceManager::CreateLogicalDevice() {
	// Step 1: Find some unique groups of units on the GPU that are logically together.
	m_queueFamilyIndices                                     = FindQueueFamilies(m_physicalDevice);
	std::set<uint32_t>                   uniqueQueueFamilies = {m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentFamily.value()};

	// Step 2: Add info for each queue for each family.
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float                                queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount       = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Step 3: Create the logical device with approved features.
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
		throw std::runtime_error("failed to create logical device!");
	}
}

void VulkanDeviceManager::CreateQueues() {
	// Step 4: Logical device will have queue handles ready after creation, allowing you to execute commands on the GPU.
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
}

uint32_t VulkanDeviceManager::RateDeviceSuitability(const VkPhysicalDevice physicalDevice) const {
	// Rate the device based on properties and features
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures   deviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	uint32_t score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

bool VulkanDeviceManager::IsDeviceSuitable(const VkPhysicalDevice physicalDevice) const {
	bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
	bool swapChainAdequate   = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);
		swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

	return queueFamilyIndices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanDeviceManager::CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice) const {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkSampleCountFlagBits VulkanDeviceManager::GetMaxUsableSampleCount() const {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT)
		return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT)
		return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT)
		return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT)
		return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT)
		return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT)
		return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

QueueFamilyIndices VulkanDeviceManager::FindQueueFamilies(const VkPhysicalDevice physicalDevice) const {
	QueueFamilyIndices queueFamilyIndices;

	uint32_t           queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int          index   = 0;
	VkSurfaceKHR surface = m_vulkanInstanceManager->GetNativeSurface();
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphicsFamily = index;
			LOG(Engine, Debug, "Queue Family: graphics available: {}", queueFamilyIndices.graphicsFamily.has_value());
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
		if (presentSupport) {
			queueFamilyIndices.presentFamily = index;
			LOG(Engine, Debug, "Queue Family: present available: {}", queueFamilyIndices.presentFamily.has_value());
		}

		if (queueFamilyIndices.IsComplete()) {
			LOG(Engine, Debug, "Queue Family: COMPLETE: {} | {}", queueFamilyIndices.graphicsFamily.has_value(), queueFamilyIndices.presentFamily.has_value());
			break;
		}

		index++;
	}

	return queueFamilyIndices;
}

SwapChainSupportDetails VulkanDeviceManager::QuerySwapChainSupport(const VkPhysicalDevice physicalDevice) const {
	SwapChainSupportDetails details;
	VkSurfaceKHR            surface = m_vulkanInstanceManager->GetNativeSurface();
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}