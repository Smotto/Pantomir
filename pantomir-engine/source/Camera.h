#ifndef PANTOMIR_CAMERA_H_
#define PANTOMIR_CAMERA_H_

#include "VkTypes.h"

union SDL_Event;

class Camera {
public:
	glm::vec3 _velocity;
	glm::vec3 _position;

	float     _pitch { 0.f }; // Vertical rotation
	float     _yaw { 0.f }; // Horizontal rotation

	glm::mat4 GetViewMatrix();
	glm::mat4 GetRotationMatrix();

	void      ProcessSDLEvent(SDL_Event& e);

	void      Update();
};

#endif /*! PANTOMIR_CAMERA_H_ */
