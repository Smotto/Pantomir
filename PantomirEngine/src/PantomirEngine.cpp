#include "PantomirEngine.h"

#include "InputManager.h"
#include "VulkanDeviceManager.h"
#include "VulkanInstanceManager.h"
#include "VulkanResourceManager.h"

#include <exception>
#include <iostream>

const std::vector<const char*> m_vulkanValidationLayers = {"VK_LAYER_KHRONOS_validation"};
std::vector<const char*>       m_vulkanDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME};

PantomirEngine::PantomirEngine()
    : m_pantomirWindow(std::make_shared<PantomirWindow>(800, 600, "Pantomir Window")),
      m_inputManager(std::make_shared<InputManager>(m_pantomirWindow)),
      m_vulkanInstanceManager(std::make_shared<VulkanInstanceManager>(m_pantomirWindow, m_vulkanValidationLayers, m_enableValidationLayers)),
      m_vulkanDeviceManager(std::make_shared<VulkanDeviceManager>(m_vulkanInstanceManager, m_vulkanDeviceExtensions)),
      m_vulkanBufferManager(std::make_shared<VulkanBufferManager>(m_vulkanDeviceManager)),
      m_resourceManager(std::make_shared<VulkanResourceManager>(m_vulkanDeviceManager, m_vulkanBufferManager)),
      m_vulkanRenderer(std::make_unique<VulkanRenderer>(m_pantomirWindow->GetNativeWindow(), m_inputManager, m_vulkanDeviceManager, m_resourceManager)) {
}

PantomirEngine::~PantomirEngine() {
}

int PantomirEngine::Start() {
	// TODO: Implement job based system instead of doing a main loop.
	try {
		MainLoop();
	} catch (const std::exception& exception) {
		std::cerr << "Exception: " << exception.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void PantomirEngine::MainLoop() {
	while (!m_pantomirWindow->ShouldClose()) {
		m_pantomirWindow->PollEvents();
		m_vulkanRenderer->DrawFrame();
	}
	m_vulkanRenderer->DeviceWaitIdle(); // Ensure all device operations are
	                                    // finished before exiting.
}