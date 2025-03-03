#ifndef PANTOMIRENGINE_H_
#define PANTOMIRENGINE_H_

#include <memory>

class InputManager;
class VulkanInstanceManager;
class VulkanDeviceManager;
class VulkanDevice;
class VulkanRenderer;
class VulkanResourceManager;
class VulkanBufferManager;
class PantomirWindow;

class PantomirEngine {
public:
	PantomirEngine();
	~PantomirEngine();
	int Start();

private:
#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	std::unique_ptr<PantomirWindow>        m_pantomirWindow;
	std::unique_ptr<InputManager>          m_inputManager;
	std::unique_ptr<VulkanInstanceManager> m_vulkanInstanceManager;
	std::unique_ptr<VulkanDeviceManager>   m_vulkanDeviceManager;
	std::unique_ptr<VulkanBufferManager>   m_vulkanBufferManager;
	std::unique_ptr<VulkanResourceManager> m_resourceManager;
	std::unique_ptr<VulkanRenderer>        m_vulkanRenderer;

	void                                   MainLoop();
};
#endif /*! PANTOMIRENGINE_H_ */