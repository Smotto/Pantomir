#include "Camera.h"

#include "GLFW/glfw3.h"
#include "InputManager.h"

Camera::Camera(InputManager* inputManager) {
	glm::vec3 targetToCameraVector = m_cameraPos - m_target;
	// Pythagorean theorem in 3D = sqrt(a^2 + b^2 + c^2)
	m_radius                       = glm::length(targetToCameraVector);

	// Resetting pitch and yaw.
	if (m_radius > 0.0f) {
		// Calculate pitch (elevation angle)
		m_pitch = glm::degrees(asin(targetToCameraVector.z / m_radius));
		// Calculate yaw (azimuthal angle)
		m_yaw   = glm::degrees(atan2(targetToCameraVector.y, targetToCameraVector.x));
	} else {
		// Handle the case where camera is at the target
		m_pitch = 0.0f;
		m_yaw   = 0.0f;
	}

	inputManager->RegisterMouseMoveCallback([this, inputManager](const MouseMoveEvent& event) {
		// Get the current mouse position
		glm::vec2 currentPos = event.position;

		// Calculate the change (delta) from the last position
		glm::vec2 delta      = currentPos - m_lastMousePos;

		// Always update the last position, whether the button is pressed or not
		m_lastMousePos       = currentPos;

		// Only update the camera if the right mouse button is pressed
		if (inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
			// Adjust yaw (horizontal rotation) and pitch (vertical rotation)
			m_yaw -= delta.x * 0.1f; // Sensitivity factor: 0.1 degrees per pixel
			m_pitch += delta.y * 0.1f;

			// Clamp pitch to prevent flipping (between -89° and 89°)
			m_pitch       = glm::clamp(m_pitch, -89.0f, 89.0f);

			// Update camera position based on spherical coordinates
			m_cameraPos.x = m_target.x + m_radius * cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
			m_cameraPos.y = m_target.y + m_radius * cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));
			m_cameraPos.z = m_target.z + m_radius * sin(glm::radians(m_pitch));
		}
	});

	inputManager->RegisterMouseButtonCallback([this, inputManager](const MouseButtonEvent& event) {
		if (inputManager->IsMouseButtonReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
		}
	});

	inputManager->RegisterScrollCallback([this](const ScrollEvent& event) {
		m_radius -= event.delta * 0.1f;           // Zoom in/out
		m_radius      = glm::max(m_radius, 0.1f); // Prevent going through target
		// Recalculate position with new radius
		m_cameraPos.x = m_target.x + m_radius * cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
		m_cameraPos.y = m_target.y + m_radius * cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));
		m_cameraPos.z = m_target.z + m_radius * sin(glm::radians(m_pitch));
	});

	// If I passed &inputManager into the lambdas' capture block, the &'d input manager would hit here and the pointer would die early.
}

Camera::~Camera() = default;