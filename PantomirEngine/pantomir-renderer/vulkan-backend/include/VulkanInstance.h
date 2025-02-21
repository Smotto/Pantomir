#ifndef VULKANINSTANCE_H_
#define VULKANINSTANCE_H_

#include <vector>
#include <vulkan/vulkan.h>

class VulkanInstance final {
public:
	VulkanInstance(const std::vector<const char*>& validationLayers,
				   const bool&                     enableValidation);
	~VulkanInstance();

	VkInstance GetNativeInstance() const {
		return m_instance;
	}
	void Cleanup();

	VkDebugUtilsMessengerEXT GetDebugMessenger() const {
		return m_debugMessenger;
	}

private:
	void                     CreateInstance();
	void                     SetupDebugMessenger();
	bool                     CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	void                     PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkInstance               m_instance       = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	std::vector<const char*> m_validationLayers;
	bool                     m_enableValidation;
};
#endif /*! VULKANINSTANCE_H_ */
