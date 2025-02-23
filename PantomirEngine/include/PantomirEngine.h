#ifndef PANTOMIRENGINE_H_
#define PANTOMIRENGINE_H_

#include "PantomirWindow.h"
#include "VulkanRenderer.h"

#include <memory>

class VulkanDeviceManager;
class VulkanDevice;

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

	std::shared_ptr<PantomirWindow>        m_pantomirWindow;
	std::shared_ptr<VulkanInstanceManager> m_vulkanInstanceManager;
	std::shared_ptr<VulkanDeviceManager>   m_vulkanDeviceManager;
	std::unique_ptr<VulkanRenderer>        m_vulkanRenderer;

	void                                   MainLoop();
};
#endif /*! PANTOMIRENGINE_H_ */