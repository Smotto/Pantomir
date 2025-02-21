#ifndef PANTOMIRENGINE_H_
#define PANTOMIRENGINE_H_

#include "PantomirWindow.h"
#include "VulkanRenderer.h"
#include <memory>

class VulkanDevice;

class PantomirEngine {
public:
	int Start();

private:
	std::shared_ptr<PantomirWindow> m_window;
	std::unique_ptr<VulkanRenderer> m_vulkanRenderer;

	void                            MainLoop();
};
#endif /*! PANTOMIRENGINE_H_ */