#include "PantomirWindow.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

PantomirWindow::PantomirWindow(int width, int height, const std::string& title)
    : m_title(title), m_width(width), m_height(height) {
	if (!glfwInit()) {
		throw std::runtime_error(std::string(__func__) + "Failed to initialize GLFW!");
	}

	// We don't want an OpenGL context; our renderer will create a Vulkan surface.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create the GLFW window with a raw pointer
	m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
	if (!m_window) {
		glfwTerminate();
		throw std::runtime_error(std::string(__func__) + "Failed to create GLFW window!");
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
}

PantomirWindow::~PantomirWindow() {
	if (m_window) {
		glfwDestroyWindow(m_window);
	}
	glfwTerminate();
}

bool PantomirWindow::ShouldClose() const {
	return glfwWindowShouldClose(m_window);
}

void PantomirWindow::PollEvents() const {
	glfwPollEvents();
}

void PantomirWindow::GetFramebufferSize(int& width, int& height) {
	glfwGetFramebufferSize(m_window, &width, &height);
}

void PantomirWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// Since it's static, we need a way to get the app. We can get it through the window, since we SetWindowUserPointer.
	auto app                  = reinterpret_cast<PantomirWindow*>(glfwGetWindowUserPointer(window));
	app->m_framebufferResized = true; // Edit the instance's boolean variable.
}
