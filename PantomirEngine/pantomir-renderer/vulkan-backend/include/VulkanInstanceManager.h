#ifndef VULKANINSTANCEMANAGER_H_
#define VULKANINSTANCEMANAGER_H_

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class PantomirWindow;

class VulkanInstanceManager final {
public:
	VulkanInstanceManager(
	    const std::shared_ptr<PantomirWindow>& pantomirWindow,
	    const std::vector<const char*>&        validationLayers,
	    const bool&                            enableValidation);
	~VulkanInstanceManager();

	VkInstance GetNativeInstance() const {
		return m_instance;
	}
	VkSurfaceKHR GetNativeSurface() const {
		return m_surface;
	}
	const std::vector<const char*>& GetExtensions() const {
		return m_extensions;
	}
	VkDebugUtilsMessengerEXT GetDebugMessenger() const {
		return m_debugMessenger;
	}

private:
	void                          CreateSurface();
	void                          SetupDebugMessenger();

	bool                          CheckValidationLayerSupport();
	std::vector<const char*>      GetRequiredExtensions();
	void                          PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	bool                          m_enableValidation;
	VkDebugUtilsMessengerEXT      m_debugMessenger = VK_NULL_HANDLE;
	VkInstance                    m_instance       = VK_NULL_HANDLE;

	VkSurfaceKHR                  m_surface        = VK_NULL_HANDLE;
	std::vector<const char*>      m_extensions;
	std::weak_ptr<PantomirWindow> m_pantomirWindow;
	std::vector<const char*>      m_validationLayers;
};
#endif /*! VULKANINSTANCEMANAGER_H_ */
