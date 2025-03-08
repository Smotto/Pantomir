#include "PantomirEngine.h"

#include "InputManager.h"
#include "PantomirWindow.h"
#include "VulkanDeviceManager.h"
#include "VulkanInstanceManager.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

#include <exception>
#include <iostream>

const std::vector<const char*> m_vulkanValidationLayers = {"VK_LAYER_KHRONOS_validation"};
std::vector<const char*>       m_vulkanDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME};

PantomirEngine::PantomirEngine() {
	m_pantomirWindow        = std::make_unique<PantomirWindow>(0, 0, "Pantomir Window");
	m_inputManager          = std::make_unique<InputManager>(m_pantomirWindow.get());
	m_vulkanInstanceManager = std::make_unique<VulkanInstanceManager>(m_pantomirWindow->GetNativeWindow(), m_vulkanValidationLayers, m_enableValidationLayers);
	m_vulkanDeviceManager   = std::make_unique<VulkanDeviceManager>(m_vulkanInstanceManager.get(), m_vulkanDeviceExtensions);
	m_vulkanBufferManager   = std::make_unique<VulkanBufferManager>(m_vulkanDeviceManager.get());
	m_resourceManager       = std::make_unique<VulkanResourceManager>(m_vulkanDeviceManager.get(), m_vulkanBufferManager.get());
	m_vulkanRenderer        = std::make_unique<VulkanRenderer>(m_pantomirWindow->GetNativeWindow(), m_inputManager.get(), m_vulkanDeviceManager.get(), m_resourceManager.get());
}

PantomirEngine::~PantomirEngine() = default;

int PantomirEngine::Start() const {
	// TODO: Implement job based system instead of doing a main loop.
	try {
		MainLoop();
	} catch (const std::exception& exception) {
		std::cerr << "Exception: " << exception.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void PantomirEngine::MainLoop() const {
	while (!m_pantomirWindow->ShouldClose()) {
		m_pantomirWindow->PollEvents();
		m_vulkanRenderer->DrawFrame();
	}
	m_vulkanRenderer->DeviceWaitIdle(); // Ensure all device operations are
	                                    // finished before exiting.
}

int main(int argc, char* argv[]) {
	PantomirEngine engine;
	return engine.Start();
}