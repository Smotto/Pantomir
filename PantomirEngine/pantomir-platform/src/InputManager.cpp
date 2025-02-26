// InputManager.cpp
#include "InputManager.h"

#include "PantomirWindow.h"

InputManager::InputManager(const std::shared_ptr<PantomirWindow>& window) : m_window(window) {
	// Store this instance in the window's user pointer for callback access
	glfwSetWindowUserPointer(m_window->GetNativeWindow(), this);

	// Register GLFW callbacks
	glfwSetKeyCallback(m_window->GetNativeWindow(), KeyCallback);
	glfwSetMouseButtonCallback(m_window->GetNativeWindow(), MouseButtonCallback);
	glfwSetCursorPosCallback(m_window->GetNativeWindow(), MouseMoveCallback);
	glfwSetScrollCallback(m_window->GetNativeWindow(), ScrollCallback);

	// Initialize mouse position
	double xpos, ypos;
	glfwGetCursorPos(m_window->GetNativeWindow(), &xpos, &ypos);
	m_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

// Destructor: Clean up (GLFW handles callback reset on window destruction)
InputManager::~InputManager() {
    // No explicit cleanup needed for callbacks since GLFW resets them when the window is destroyed
}

// Register callback functions
void InputManager::RegisterKeyCallback(std::function<void(const KeyEvent&)> callback) {
    m_keyCallbacks.push_back(callback);
}

void InputManager::RegisterMouseButtonCallback(std::function<void(const MouseButtonEvent&)> callback) {
    m_mouseButtonCallbacks.push_back(callback);
}

void InputManager::RegisterMouseMoveCallback(std::function<void(const MouseMoveEvent&)> callback) {
    m_mouseMoveCallbacks.push_back(callback);
}

void InputManager::RegisterScrollCallback(std::function<void(const ScrollEvent&)> callback) {
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
void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        KeyEvent event{key, action};
        input->DispatchKeyEvent(event);

        // Update key state
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            input->m_keyStates[key] = action;
        }
    }
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        MouseButtonEvent event{button, action};
        input->DispatchMouseButtonEvent(event);

        // Update button state
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            input->m_mouseButtonStates[button] = action;
        }
    }
}

void InputManager::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        MouseMoveEvent event{glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos))};
        input->m_mousePosition = event.position; // Update current position
        input->DispatchMouseMoveEvent(event);
    }
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
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