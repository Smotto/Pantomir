#ifndef PANTOMIR_ENGINE_H_
#define PANTOMIR_ENGINE_H_

#include "Camera.h"
#include "VkDescriptors.h"
#include "VkTypes.h"

class InputSystem;
struct RenderObject;
struct LoadedGLTF;
struct MeshAsset;

struct EngineStats {
	float frametime;
	int   triangle_count;
	int   drawcall_count;
	float scene_update_time;
	float mesh_draw_time;
};

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
	std::vector<RenderObject> TransparentSurfaces;
};

struct MeshNode : public Node {
	std::shared_ptr<MeshAsset> mesh;

	virtual void               Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; // w for sun power
	glm::vec4 sunlightColor;
};

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
	// Stores a lambda pointer
	std::deque<std::shared_ptr<std::function<void()>>> _deletors;

	void                                               PushFunction(std::function<void()>&& function) {
        _deletors.push_back(MakeDeletionTask(std::forward<decltype(function)>(function)));
	}

	void Flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = _deletors.rbegin(); it != _deletors.rend(); ++it) {
			(**it)(); // Call functors
		}

		_deletors.clear();
	}

private:
	inline std::shared_ptr<std::function<void()>> MakeDeletionTask(auto&& lambda) {
		return std::make_shared<std::function<void()>>(std::forward<decltype(lambda)>(lambda));
	}
};

struct FrameData {
	VkSemaphore                 _swapchainSemaphore {}, _renderSemaphore {};
	VkFence                     _renderFence {};

	VkCommandPool               _commandPool {};
	VkCommandBuffer             _mainCommandBuffer {};

	DeletionQueue               _deletionQueue;
	DescriptorAllocatorGrowable _frameDescriptors;
};

constexpr unsigned int             FRAME_OVERLAP = 2;
inline DescriptorAllocatorGrowable globalDescriptorAllocator;

class PantomirEngine;

struct GLTFMetallic_Roughness {
	MaterialPipeline      _opaquePipeline;
	MaterialPipeline      _transparentPipeline;

	VkDescriptorSetLayout _materialLayout;
	VkPipelineLayout      _pipelineLayout;

	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		// Padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler      colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler      metalRoughSampler;
		VkBuffer       dataBuffer;
		uint32_t       dataBufferOffset;
	};

	DescriptorWriter _writer;

	void             BuildPipelines(PantomirEngine* engine);
	void             ClearResources(VkDevice device);

	MaterialInstance WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

class PantomirEngine {
public:
	std::shared_ptr<InputSystem>                           _inputSystem;

	bool                                                   bUseValidationLayers = true;

	EngineStats                                            _stats;

	Camera                                                 _mainCamera;

	VkPipelineLayout                                       _meshPipelineLayout {};
	VkPipeline                                             _meshPipeline {};

	std::vector<ComputeEffect>                             _backgroundEffects;
	int                                                    _currentBackgroundEffect { 0 };

	// immediate submit structures
	VkFence                                                _immediateFence {};
	VkCommandBuffer                                        _immediateCommandBuffer {};
	VkCommandPool                                          _immediateCommandPool {};

	VkPipeline                                             _gradientPipeline {};
	VkPipelineLayout                                       _gradientPipelineLayout {};
	VkDescriptorSet                                        _drawImageDescriptors {};
	VkDescriptorSetLayout                                  _drawImageDescriptorLayout {};

	GPUSceneData                                           _sceneData {};
	VkDescriptorSetLayout                                  _gpuSceneDataDescriptorLayout {};

	DrawContext                                            _mainDrawContext;
	std::unordered_map<std::string, std::shared_ptr<Node>> _loadedNodes;

	AllocatedImage                                         _drawImage {};
	AllocatedImage                                         _depthImage {};
	VkExtent2D                                             _drawExtent {};
	VmaAllocator                                           _allocator {};
	DeletionQueue                                          _mainDeletionQueue;

	bool                                                   _isInitialized { false };
	int                                                    _frameNumber { 0 };
	bool                                                   _stopRendering { false };
	bool                                                   _resizeRequested { false };
	VkExtent2D                                             _windowExtent { 1280, 720 };

	struct SDL_Window*                                     _window { nullptr };
	float                                                  _windowRatio = 0.8;
	float                                                  _renderScale = 1.f;

	VkInstance                                             _instance {};       // Vulkan Library Handle
	VkDebugUtilsMessengerEXT                               _debugMessenger {}; // Vulkan debug output handle
	VkPhysicalDevice                                       _chosenGPU {};      // GPU chosen as the default device
	VkDevice                                               _device {};         // Vulkan device for commands
	VkSurfaceKHR                                           _surface {};        // Vulkan window surface

	VkSwapchainKHR                                         _swapchain {};
	VkFormat                                               _swapchainImageFormat;

	std::vector<VkImage>                                   _swapchainImages;
	std::vector<VkImageView>                               _swapchainImageViews;
	VkExtent2D                                             _swapchainExtent {};

	FrameData                                              _frames[FRAME_OVERLAP];
	FrameData&                                             GetCurrentFrame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
	};

	VkQueue                                                      _graphicsQueue {};
	uint32_t                                                     _graphicsQueueFamily {};

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;
	MaterialInstance                                             _defaultData {};
	GLTFMetallic_Roughness                                       _metalRoughMaterial;

	AllocatedImage                                               _whiteImage {};
	AllocatedImage                                               _blackImage {};
	AllocatedImage                                               _greyImage {};
	AllocatedImage                                               _errorCheckerboardImage {};

	VkSampler                                                    _defaultSamplerLinear {};
	VkSampler                                                    _defaultSamplerNearest {};

	VkDescriptorSetLayout                                        _singleImageDescriptorLayout {};

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

	GPUMeshBuffers    UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	AllocatedImage    CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage    CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void              DestroyImage(const AllocatedImage& img);

	AllocatedBuffer   CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void              DestroyBuffer(const AllocatedBuffer& buffer);

private:
	float _deltaTime = 0.0f;

	PantomirEngine();
	~PantomirEngine();

	void InitSDLWindow();
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitBackgroundPipelines();
	void InitImgui();
	void InitMeshPipeline();
	void InitDefaultData();

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void ResizeSwapchain();

	void UpdateScene();
};

#endif /*! PANTOMIR_ENGINE_H_ */