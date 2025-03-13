#ifndef PANTOMIR_ENGINE_H_
#define PANTOMIR_ENGINE_H_

#include <VkTypes.h>

struct DeletionQueue {
	// Stores a lambda
	// TODO: Store handles instead of functions instead.
	std::deque<std::function<void()>> _deletors;

	// Moves the function without copying, using r-value
	void push_function(std::function<void()>&& function) {
        _deletors.push_back(function);
	}

	void flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = _deletors.rbegin(); it != _deletors.rend(); ++it) {
			(*it)(); // Call functors
		}

		_deletors.clear();
	}
};

struct FrameData {
	VkCommandPool   _command_pool;
	VkCommandBuffer _main_command_buffer;
	VkSemaphore     _swapchain_semaphore, _render_semaphore;
	VkFence         _render_fence;
	DeletionQueue   _deletion_queue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class PantomirEngine {
public:
	AllocatedImage           _draw_image;
	VkExtent2D               _draw_extent;
	VmaAllocator             _allocator;
	DeletionQueue            _main_deletion_queue;

	bool                     _is_initialized { false };
	int                      _frame_number { 0 };
	bool                     _stop_rendering { false };
	VkExtent2D               _window_extent { 1700, 900 };

	struct SDL_Window*       _window { nullptr };

	VkInstance               _instance;        // Vulkan Library Handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice         _chosen_GPU;       // GPU chosen as the default device
	VkDevice                 _device;          // Vulkan device for commands
	VkSurfaceKHR             _surface;         // Vulkan window surface

	VkSwapchainKHR           _swapchain;
	VkFormat                 _swapchain_image_format;

	std::vector<VkImage>     _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;
	VkExtent2D               _swapchain_extent;

	FrameData                _frames[FRAME_OVERLAP];

	FrameData&               get_current_frame() {
        return _frames[_frame_number % FRAME_OVERLAP];
	};

	VkQueue  _graphics_queue;
	uint32_t _graphics_queue_family;

	PantomirEngine(const PantomirEngine&)                   = delete;
	PantomirEngine&        operator=(const PantomirEngine&) = delete;

	static PantomirEngine& GetInstance() {
		static PantomirEngine instance;
		return instance;
	}

	[[nodiscard]] int start();
	void              mainLoop();
	void              draw();
	void              drawBackground(VkCommandBuffer command_buffer);

private:
	PantomirEngine();
	~PantomirEngine();

	void initSDLWindow();
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initSyncStructures();

	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();
};

#endif /*! PANTOMIR_ENGINE_H_ */