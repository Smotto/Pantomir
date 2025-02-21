#ifndef PANTOMIRWINDOW_H_
#define PANTOMIRWINDOW_H_

#include <string>
#include <vulkan/vulkan.h>

struct GLFWwindow;

class PantomirWindow {
public:
	PantomirWindow(int width, int height, const std::string& title);
	~PantomirWindow();

	GLFWwindow* GetNativeWindow() const {
		return m_window;
	}

	bool     m_framebufferResized = false;

	bool     ShouldClose() const;
	void     PollEvents() const;
	void     GetFramebufferSize(int& width, int& height);
	VkResult CreateWindowSurface(VkInstance                   instance,
	                             const VkAllocationCallbacks* allocator,
	                             VkSurfaceKHR*                surface);

private:
	GLFWwindow* m_window;
	std::string m_title;
	int         m_width;
	int         m_height;

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};
#endif /*! PANTOMIRWINDOW_H_ */
