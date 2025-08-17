#include "Camera.h"

#include <SDL3/SDL_events.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

glm::mat4 Camera::GetViewMatrix() const {
	// To create a correct model view, we need to move the world in opposite
	// direction to the camera so we will create the camera model matrix and invert
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), _position);
	glm::mat4 cameraRotation = GetRotationMatrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::GetRotationMatrix() const {
	const glm::vec3 right { 1.f, 0.f, 0.f };
	const glm::vec3 down { 0.f, -1.f, 0.f };
	const glm::quat pitchRotation = glm::angleAxis(_pitch, right);
	const glm::quat yawRotation = glm::angleAxis(_yaw, down);
	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void Camera::Update(const float deltaTime) {
	const glm::mat4 cameraRotation = GetRotationMatrix();
	const glm::vec4 cameraDelta = glm::vec4(_velocity * 0.5f, 0.f);
	_position += glm::vec3(cameraRotation * cameraDelta) * deltaTime;
}

void Camera::ProcessSDLEvent(SDL_Event& event, SDL_Window* window) {
	static bool rightMouseHeld = false;

	switch (event.type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button == SDL_BUTTON_RIGHT && event.button.down) {
				rightMouseHeld = true;
				int      lockX = static_cast<int>(event.button.x);
				int      lockY = static_cast<int>(event.button.y);
				SDL_Rect lockRect = { lockX, lockY, 1, 1 };
				SDL_SetWindowMouseRect(window, &lockRect);
				SDL_SetWindowRelativeMouseMode(window, true);
			}
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button == SDL_BUTTON_RIGHT && !event.button.down) {
				rightMouseHeld = false;
				SDL_SetWindowMouseRect(window, nullptr);
				SDL_SetWindowRelativeMouseMode(window, false);
			}
			break;

		case SDL_EVENT_MOUSE_MOTION:
			if (rightMouseHeld) {
				_yaw += event.motion.xrel / 200.0f;
				_pitch -= event.motion.yrel / 200.0f;
			}
			break;

		default:
			break;
	}
}

void Camera::UpdateMovement() {
	const bool* keyState = SDL_GetKeyboardState(nullptr);

	// Check if shift is held for speed multiplier
	bool        shiftHeld = keyState[SDL_SCANCODE_LSHIFT] || keyState[SDL_SCANCODE_RSHIFT];
	float       currentSpeed = shiftHeld ? _speedMultiplier * _shiftSpeedMultiplier : _speedMultiplier;

	// Reset velocity
	_velocity = glm::vec3(0.0f);

	// Check movement keys
	if (keyState[SDL_SCANCODE_W]) {
		_velocity.z = -currentSpeed; // Forward RHS -Z Forward
	}
	if (keyState[SDL_SCANCODE_S]) {
		_velocity.z = currentSpeed; // Backwards RHS +Z Backwards
	}
	if (keyState[SDL_SCANCODE_A]) {
		_velocity.x = -currentSpeed; // Left RHS -X
	}
	if (keyState[SDL_SCANCODE_D]) {
		_velocity.x = currentSpeed; // Right RHS +X
	}
	if (keyState[SDL_SCANCODE_Q]) {
		_velocity.y = currentSpeed; // Up RHS +Y
	}
	if (keyState[SDL_SCANCODE_Z]) {
		_velocity.y = -currentSpeed; // Down RHS -Y
	}
}
