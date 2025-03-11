#include "InputManager.h"

#include "GLFW/glfw3.h"
#include "PantomirWindow.h"

InputManager::InputManager(const PantomirWindow* window)
    : m_window(window) {
	GLFWwindow* glfwWindow = m_window->GetNativeWindow();
	// Store this instance in the window's user pointer for callback access
	glfwSetWindowUserPointer(glfwWindow, this);

	// Register GLFW callbacks
	glfwSetKeyCallback(glfwWindow, KeyCallback);
	glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
	glfwSetCursorPosCallback(glfwWindow, MouseMoveCallback);
	glfwSetScrollCallback(glfwWindow, ScrollCallback);

	// Initialize mouse position
	double xpos, ypos;
	glfwGetCursorPos(glfwWindow, &xpos, &ypos);
	m_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

// Destructor: Clean up (GLFW handles callback reset on window destruction)
InputManager::~InputManager() = default;

// Register callback functions
void InputManager::RegisterKeyCallback(const std::function<void(const KeyEvent&)>& callback) {
    m_keyCallbacks.push_back(callback);
}

void InputManager::RegisterMouseButtonCallback(const std::function<void(const MouseButtonEvent&)>& callback) {
    m_mouseButtonCallbacks.push_back(callback);
}

void InputManager::RegisterMouseMoveCallback(const std::function<void(const MouseMoveEvent&)>& callback) {
    m_mouseMoveCallbacks.push_back(callback);
}

void InputManager::RegisterScrollCallback(const std::function<void(const ScrollEvent&)>& callback) {
    m_scrollCallbacks.push_back(callback);
}

// Query current input state
bool InputManager::IsKeyPressed(int key) const {
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && it->second == GLFW_PRESS;
}

bool InputManager::IsMouseButtonPressed(int button) const {
    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() && it->second == GLFW_PRESS;
}

bool InputManager::IsMouseButtonReleased(int button) const {
	auto it = m_mouseButtonStates.find(button);
	return it != m_mouseButtonStates.end() && it->second == GLFW_RELEASE;
}

glm::vec2 InputManager::GetMousePosition() const {
    return m_mousePosition;
}

float InputManager::GetScrollDelta() const {
    return m_scrollDelta;
}

// Update: Reset per-frame data (e.g., scroll delta)
void InputManager::Update() {
    m_scrollDelta = 0.0f;
    // Could clear other per-frame data here if added later
}

// Static GLFW callbacks: Forward to instance methods
void InputManager::KeyCallback(GLFWwindow* window, const int key, int scancode, const int action, int mods) {
	if (InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        const KeyEvent event{key, action};
        input->DispatchKeyEvent(event);

        // Update key state
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            input->m_keyStates[key] = action;
        }
    }
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        MouseButtonEvent event{button, action};
        input->DispatchMouseButtonEvent(event);

        // Update button state
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            input->m_mouseButtonStates[button] = action;
        }
    }
}

void InputManager::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        MouseMoveEvent event{glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos))};
        input->m_mousePosition = event.position; // Update current position
        input->DispatchMouseMoveEvent(event);
    }
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	if (InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        ScrollEvent event{static_cast<float>(yoffset)};
        input->m_scrollDelta = event.delta; // Update scroll delta
        input->DispatchScrollEvent(event);
    }
}

void InputManager::DispatchKeyEvent(const KeyEvent& event) {
    for (const auto& callback : m_keyCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}

void InputManager::DispatchMouseButtonEvent(const MouseButtonEvent& event) {
    for (const auto& callback : m_mouseButtonCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}

void InputManager::DispatchMouseMoveEvent(const MouseMoveEvent& event) {
    for (const auto& callback : m_mouseMoveCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}

void InputManager::DispatchScrollEvent(const ScrollEvent& event) {
    for (const auto& callback : m_scrollCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}