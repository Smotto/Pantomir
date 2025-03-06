#ifndef VULKANINSTANCEMANAGER_H_
#define VULKANINSTANCEMANAGER_H_

#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

class VulkanInstanceManager final {
public:
	VulkanInstanceManager(
	    GLFWwindow*                     window,
	    const std::vector<const char*>& validationLayers,
	    const bool&                     enableValidation);
	~VulkanInstanceManager();

	[[nodiscard]] VkInstance GetNativeInstance() const {
		return m_instance;
	}
	[[nodiscard]] VkSurfaceKHR GetNativeSurface() const {
		return m_surface;
	}
	[[nodiscard]] const std::vector<const char*>& GetExtensions() const {
		return m_extensions;
	}
	[[nodiscard]] VkDebugUtilsMessengerEXT GetDebugMessenger() const {
		return m_debugMessenger;
	}

private:
	void                     CreateSurface();
	void                     SetupDebugMessenger();

	bool                     CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	void                     PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	GLFWwindow*              m_window;
	VkInstance               m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR             m_surface  = VK_NULL_HANDLE;
	std::vector<const char*> m_extensions;
	std::vector<const char*> m_validationLayers;

	bool                     m_enableValidation;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};
#endif /*! VULKANINSTANCEMANAGER_H_ */
