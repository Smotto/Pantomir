#ifndef INPUTMANAGER_H_
#define INPUTMANAGER_H_

#include "InputEvents.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class PantomirWindow;

class InputManager final {
public:
	explicit InputManager(const std::shared_ptr<PantomirWindow>& window);
	~InputManager();

	// Register callbacks for events
	void      RegisterKeyCallback(std::function<void(const KeyEvent&)> callback);
	void      RegisterMouseButtonCallback(std::function<void(const MouseButtonEvent&)> callback);
	void      RegisterMouseMoveCallback(std::function<void(const MouseMoveEvent&)> callback);
	void      RegisterScrollCallback(std::function<void(const ScrollEvent&)> callback);

	// Query current state
	bool      IsKeyPressed(int key) const;
	bool      IsMouseButtonPressed(int button) const;
	bool      IsMouseButtonReleased(int button) const;
	glm::vec2 GetMousePosition() const;
	float     GetScrollDelta() const;

	// Update method to clear per-frame data
	void      Update();

private:
	std::shared_ptr<PantomirWindow>                           m_window;
	std::vector<std::function<void(const KeyEvent&)>>         m_keyCallbacks;
	std::vector<std::function<void(const MouseButtonEvent&)>> m_mouseButtonCallbacks;
	std::vector<std::function<void(const MouseMoveEvent&)>>   m_mouseMoveCallbacks;
	std::vector<std::function<void(const ScrollEvent&)>>      m_scrollCallbacks;

	// Current state
	std::unordered_map<int, int>                              m_keyStates;
	std::unordered_map<int, int>                              m_mouseButtonStates;
	glm::vec2                                                 m_mousePosition;
	float                                                     m_scrollDelta = 0.0f;

	// Static callback functions for GLFW
	static void                                               KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void                                               MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void                                               MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
	static void                                               ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	// Internal event dispatchers
	void                                                      DispatchKeyEvent(const KeyEvent& event);
	void                                                      DispatchMouseButtonEvent(const MouseButtonEvent& event);
	void                                                      DispatchMouseMoveEvent(const MouseMoveEvent& event);
	void                                                      DispatchScrollEvent(const ScrollEvent& event);
};

#endif /*! INPUTMANAGER_H_ */