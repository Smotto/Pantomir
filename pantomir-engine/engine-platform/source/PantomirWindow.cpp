﻿#include "PantomirWindow.h"
#include "LoggerMacros.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

PantomirWindow::PantomirWindow(int width, int height, const std::string& title)
    : m_title(title), m_width(width), m_height(height) {
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	// Telling GLFW to not use opengl, because we are making our own.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Gets the monitor with: "Make this my main display" enabled in display settings on Windows.
	GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

	LOG(Engine_Platform, Debug, "Video Mode: BGR depth: {}b {}g {}r | Resolution:{}x{} {}hz", mode->blueBits, mode->greenBits, mode->redBits, mode->width, mode->height, mode->refreshRate);

	// Use default dimensions if none provided, or clamp to monitor size
	if (width <= 0 || height <= 0) {
		m_width  = mode->width * 0.8;  // 80% of monitor width
		m_height = mode->height * 0.8; // 80% of monitor height
	} else {
		m_width  = std::min(width, mode->width);
		m_height = std::min(height, mode->height);
	}

	// Create window
	m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
	if (!m_window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	// Center the window on the primary monitor
	int xpos = (mode->width - m_width) / 2;
	int ypos = (mode->height - m_height) / 2;
	glfwSetWindowPos(m_window, xpos, ypos);

	// Set up window properties
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

void PantomirWindow::GetFramebufferSize(int& width, int& height) const {
	glfwGetFramebufferSize(m_window, &width, &height);
}

void PantomirWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto* app                 = static_cast<PantomirWindow*>(glfwGetWindowUserPointer(window));
	app->m_framebufferResized = true;
}