#ifndef PANTOMIR_CAMERA_H_
#define PANTOMIR_CAMERA_H_

#include "VkTypes.h"

#include <SDL3/SDL_video.h>

union SDL_Event;

class Camera {
public:
	glm::vec3 _velocity;
	glm::vec3 _position;
	float     _speedMultiplier      = 3.0f;
	float     _shiftSpeedMultiplier = 2.0f;

	float     _pitch { 0.f }; // Vertical rotation
	float     _yaw { 0.f };   // Horizontal rotation

	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] glm::mat4 GetRotationMatrix() const;

	void      ProcessSDLEvent(SDL_Event& event, SDL_Window* window);
	void      UpdateMovement();
	void      Update(const float deltaTime);
};

#endif /*! PANTOMIR_CAMERA_H_ */
