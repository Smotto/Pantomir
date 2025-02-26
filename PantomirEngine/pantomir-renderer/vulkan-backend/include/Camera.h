#ifndef CAMERA_H_
#define CAMERA_H_

#include "InputManager.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class Camera {
public:
	Camera(const std::shared_ptr<InputManager>& inputManager);
	~Camera();
	void      PollKeyboardInputs();

	glm::vec3 m_cameraPos            = glm::vec3(2.0f, 2.0f, 2.0f);
	float     m_yaw                  = -135.0f;
	float     m_pitch                = -35.0f;
	float     m_radius               = 3.46f;
	glm::vec3 m_target               = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec2 m_lastMousePos         = glm::vec2(0.5f, 0.5f);
	float     m_moveSpeed            = 0.01f;
	bool      m_rightMouseWasPressed = false;

private:
};

#endif /*! CAMERA_H_ */