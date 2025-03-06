#ifndef INPUTEVENTS_H_
#define INPUTEVENTS_H_

#include "glm/glm.hpp"

struct KeyEvent {
	int key;    // GLFW key code
	int action; // GLFW_PRESS, GLFW_RELEASE, etc.
};

struct MouseButtonEvent {
	int button; // GLFW mouse button code
	int action; // GLFW_PRESS, GLFW_RELEASE, etc.
};

struct MouseMoveEvent {
	glm::vec2 position; // Mouse coordinates (x, y)
};

struct ScrollEvent {
	float delta; // Scroll amount
};

#endif /*! INPUTEVENTS_H_ */