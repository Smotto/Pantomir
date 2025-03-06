#ifndef PANTOMIRWINDOW_H_
#define PANTOMIRWINDOW_H_

#include <string>

struct GLFWwindow;

class PantomirWindow {
public:
	PantomirWindow(int width, int height, const std::string& title);
	~PantomirWindow();

	[[nodiscard]] GLFWwindow* GetNativeWindow() const {
		return m_window;
	}

	bool               m_framebufferResized = false;

	[[nodiscard]] bool ShouldClose() const;
	void               PollEvents() const;
	void               GetFramebufferSize(int& width, int& height) const;

private:
	GLFWwindow* m_window;
	std::string m_title;
	int         m_width;
	int         m_height;

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};
#endif /*! PANTOMIRWINDOW_H_ */
