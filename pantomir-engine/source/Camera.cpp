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

void Camera::ProcessSDLEvent(SDL_Event& e) {
	// Key pressed
	if (e.type == SDL_EVENT_KEY_DOWN) {
		if (e.key.key == SDLK_W) {
			_velocity.z = -1 * _speedMultiplier;
		}
		if (e.key.key == SDLK_S) {
			_velocity.z = 1 * _speedMultiplier;
		}
		if (e.key.key == SDLK_A) {
			_velocity.x = -1 * _speedMultiplier;
		}
		if (e.key.key == SDLK_D) {
			_velocity.x = 1 * _speedMultiplier;
		}
	}

	// Key released
	if (e.type == SDL_EVENT_KEY_UP) {
		if (e.key.key == SDLK_W) {
			_velocity.z = 0;
		}
		if (e.key.key == SDLK_S) {
			_velocity.z = 0;
		}
		if (e.key.key == SDLK_A) {
			_velocity.x = 0;
		}
		if (e.key.key == SDLK_D) {
			_velocity.x = 0;
		}
	}

	if (e.type == SDL_EVENT_MOUSE_MOTION) {
		_yaw += (float)e.motion.xrel / 200.f;
		_pitch -= (float)e.motion.yrel / 200.f;
	}
}