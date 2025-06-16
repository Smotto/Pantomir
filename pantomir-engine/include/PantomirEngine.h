#ifndef PANTOMIR_ENGINE_H_
#define PANTOMIR_ENGINE_H_

#include "VkDescriptors.h"
#include "VkTypes.h"

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char*          name;

	VkPipeline           pipeline;
	VkPipelineLayout     layout;

	ComputePushConstants data;
};

struct DeletionQueue {
	// Stores a lambda
	// TODO: Store handles instead of functions instead.
	std::deque<std::function<void()>> _deletors;

	// Moves the function without copying, using r-value
	void                              push_function(std::function<void()>&& function) {
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
	VkCommandPool   _commandPool {};
	VkCommandBuffer _mainCommandBuffer {};
	VkSemaphore     _swapchainSemaphore {}, _renderSemaphore {};
	VkFence         _renderFence {};
	DeletionQueue   _deletionQueue;
};

constexpr unsigned int     FRAME_OVERLAP = 2;
inline DescriptorAllocator globalDescriptorAllocator;

class PantomirEngine {
public:
	VkPipelineLayout           _meshPipelineLayout;
	VkPipeline                 _meshPipeline;
	GPUMeshBuffers             _rectangle;

	VkPipelineLayout           _trianglePipelineLayout;
	VkPipeline                 _trianglePipeline;

	std::vector<ComputeEffect> backgroundEffects;
	int                        currentBackgroundEffect { 0 };

	// immediate submit structures
	VkFence                    _immediateFence;
	VkCommandBuffer            _immediateCommandBuffer;
	VkCommandPool              _immediateCommandPool;

	VkPipeline                 _gradientPipeline;
	VkPipelineLayout           _gradientPipelineLayout;
	VkDescriptorSet            _drawImageDescriptors;
	VkDescriptorSetLayout      _drawImageDescriptorLayout;

	AllocatedImage             _drawImage {};
	VkExtent2D                 _drawExtent {};
	VmaAllocator               _allocator {};
	DeletionQueue              _mainDeletionQueue;

	bool                       _isInitialized { false };
	int                        _frameNumber { 0 };
	bool                       _stopRendering { false };
	VkExtent2D                 _windowExtent { 1280, 720 };

	struct SDL_Window*         _window { nullptr };

	VkInstance                 _instance {};       // Vulkan Library Handle
	VkDebugUtilsMessengerEXT   _debugMessenger {}; // Vulkan debug output handle
	VkPhysicalDevice           _chosenGPU {};      // GPU chosen as the default device
	VkDevice                   _device {};         // Vulkan device for commands
	VkSurfaceKHR               _surface {};        // Vulkan window surface

	VkSwapchainKHR             _swapchain {};
	VkFormat                   _swapchainImageFormat;

	std::vector<VkImage>       _swapchainImages;
	std::vector<VkImageView>   _swapchainImageViews;
	VkExtent2D                 _swapchainExtent {};

	FrameData                  _frames[FRAME_OVERLAP];

	FrameData&                 get_current_frame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
	};

	VkQueue  _graphicsQueue {};
	uint32_t _graphicsQueueFamily {};

	PantomirEngine(const PantomirEngine&)                   = delete;
	PantomirEngine&        operator=(const PantomirEngine&) = delete;

	static PantomirEngine& GetInstance() {
		static PantomirEngine instance;
		return instance;
	}

	[[nodiscard]] int Start();
	void              MainLoop();
	void              Draw();
	void              DrawBackground(VkCommandBuffer commandBuffer);
	void              DrawGeometry(VkCommandBuffer commandBuffer);
	void              DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView);
	void              ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& anonymousFunction);

private:
	PantomirEngine();
	~PantomirEngine();

	void            InitSDLWindow();
	void            InitVulkan();
	void            InitSwapchain();
	void            InitCommands();
	void            InitSyncStructures();
	void            InitDescriptors();
	void            InitPipelines();
	void            InitBackgroundPipelines();
	void            InitImgui();
	void            InitTrianglePipeline();
	void            InitMeshPipeline();
	void            InitDefaultData();

	GPUMeshBuffers  UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void            CreateSwapchain(uint32_t width, uint32_t height);
	void            DestroySwapchain();
	void            DestroyBuffer(const AllocatedBuffer& buffer);
};

#endif /*! PANTOMIR_ENGINE_H_ */