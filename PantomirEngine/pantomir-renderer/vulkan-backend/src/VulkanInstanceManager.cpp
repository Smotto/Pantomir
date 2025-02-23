#include "VulkanInstanceManager.h"

#include "PantomirWindow.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

VulkanInstanceManager::VulkanInstanceManager(const std::shared_ptr<PantomirWindow>& pantomirWindow, const std::vector<const char*>& validationLayers, const bool& enableValidation)
    : m_pantomirWindow(pantomirWindow), m_validationLayers(validationLayers), m_enableValidation(enableValidation) {
	if (m_enableValidation) {
		SetupDebugMessenger();
	}

	if (m_enableValidation && !CheckValidationLayerSupport()) {
		throw std::runtime_error(std::string(__func__) + "Validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Pantomir Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "Pantomir";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo         = &appInfo;

	std::vector<const char*> extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount    = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames  = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (m_enableValidation) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext             = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error(std::string(__func__) + "Failed to create Vulkan instance!");
	}

	CreateSurface();
}

VulkanInstanceManager::~VulkanInstanceManager() {
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	if (m_enableValidation) {
		auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (func) {
			func(m_instance, m_debugMessenger, nullptr);
		}
	}
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanInstanceManager::CreateSurface() {
	if (glfwCreateWindowSurface(m_instance, m_pantomirWindow->GetNativeWindow(), nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error(": Failed to create window surface!");
	}
}

void VulkanInstanceManager::SetupDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
	if (func != nullptr) {
		if (func(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error(std::string(__func__) + "Failed to set up debug messenger!");
		}
	}
}

bool VulkanInstanceManager::CheckValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	for (const char* layerName : m_validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (std::strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;
}

std::vector<const char*> VulkanInstanceManager::GetRequiredExtensions() {
	uint32_t                 glfwExtensionCount = 0;
	const char**             glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (m_enableValidation) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

void VulkanInstanceManager::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo                 = {};
	createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData) -> VkBool32 {
		std::cerr << "Validation layer: " << data->pMessage << "\n";
		return VK_FALSE;
	};
}
