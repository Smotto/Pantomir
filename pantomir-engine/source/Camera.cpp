#include "Camera.h"

#include <SDL3/SDL_events.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

glm::mat4 Camera::GetViewMatrix() {
	// To create a correct model view, we need to move the world in opposite
	// direction to the camera so we will create the camera model matrix and invert
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), _position);
	glm::mat4 cameraRotation    = GetRotationMatrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::GetRotationMatrix() {
	glm::quat pitchRotation = glm::angleAxis(_pitch, glm::vec3 { 1.f, 0.f, 0.f });
	glm::quat yawRotation   = glm::angleAxis(_yaw, glm::vec3 { 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void Camera::Update(const float deltaTime) {
	glm::mat4 cameraRotation = GetRotationMatrix();
	_position += glm::vec3(cameraRotation * glm::vec4(_velocity * 0.5f, 0.f)) * deltaTime;
}

void Camera::ProcessSDLEvent(SDL_Event& event, SDL_Window* window) {
	static bool rightMouseHeld = false;

	switch (event.type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button == SDL_BUTTON_RIGHT && event.button.down) {
				rightMouseHeld = true;
				int lockX = event.button.x;
				int lockY = event.button.y;
				SDL_Rect lockRect = {lockX, lockY, 1, 1};
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

		case SDL_EVENT_KEY_DOWN:
			switch (event.key.key) {
				case SDLK_W:
					_velocity.z = -_speedMultiplier;
					break;
				case SDLK_S:
					_velocity.z = _speedMultiplier;
					break;
				case SDLK_A:
					_velocity.x = -_speedMultiplier;
					break;
				case SDLK_D:
					_velocity.x = _speedMultiplier;
					break;
			}
			break;

		case SDL_EVENT_KEY_UP:
			switch (event.key.key) {
				case SDLK_W:
				case SDLK_S:
					_velocity.z = 0;
					break;
				case SDLK_A:
				case SDLK_D:
					_velocity.x = 0;
					break;
			}
			break;

		default:
			break;
	}
}
