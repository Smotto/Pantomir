#include "PantomirEngine.h"

#include <exception>
#include <iostream>

int PantomirEngine::Start() {
	try {
		m_window         = std::make_shared<PantomirWindow>(800, 600, "Pantomir Window");
		m_vulkanRenderer = std::make_unique<VulkanRenderer>(m_window->GetNativeWindow());

		MainLoop();
	} catch (const std::exception& exception) {
		std::cerr << "Exception: " << exception.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void PantomirEngine::MainLoop() {
	while (!m_window->ShouldClose()) {
		m_window->PollEvents();
		m_vulkanRenderer->DrawFrame();
	}
	m_vulkanRenderer->DeviceWaitIdle(); // Ensure all device operations are
	                                    // finished before exiting.
}