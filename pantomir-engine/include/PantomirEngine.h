#ifndef PANTOMIR_ENGINE_H
#define PANTOMIR_ENGINE_H

#include <VkTypes.h>
#include <memory>

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void                              push_function(std::function<void()>&& function) {
        deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // call functors
		}

		deletors.clear();
	}
};

struct FrameData {
	VkCommandPool   _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	VkSemaphore     _swapchainSemaphore, _renderSemaphore;
	VkFence         _renderFence;
	DeletionQueue   _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class PantomirEngine {
public:
	AllocatedImage           _drawImage;
	VkExtent2D               _drawExtent;
	VmaAllocator             _allocator;
	DeletionQueue            _mainDeletionQueue;

	bool                     _isInitialized{false};
	int                      _frameNumber{0};
	bool                     stop_rendering{false};
	VkExtent2D               _windowExtent{1700, 900};

	struct SDL_Window*       _window{nullptr};

	VkInstance               _instance;        // Vulkan Library Handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice         _chosenGPU;       // GPU chosen as the default device
	VkDevice                 _device;          // Vulkan device for commands
	VkSurfaceKHR             _surface;         // Vulkan window surface

	VkSwapchainKHR           _swapchain;
	VkFormat                 _swapchainImageFormat;

	std::vector<VkImage>     _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D               _swapchainExtent;

	FrameData                _frames[FRAME_OVERLAP];

	FrameData&               get_current_frame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
	};

	VkQueue  _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	PantomirEngine(const PantomirEngine&)                   = delete;
	PantomirEngine&        operator=(const PantomirEngine&) = delete;

	static PantomirEngine& GetInstance() {
		static PantomirEngine instance;
		return instance;
	}

	[[nodiscard]] int Start();
	void              MainLoop();
	void              draw();
	void              draw_background(VkCommandBuffer cmd);

private:
	PantomirEngine();
	~PantomirEngine();

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
};

#endif // PANTOMIR_ENGINE_H