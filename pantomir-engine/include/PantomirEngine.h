#ifndef PANTOMIR_ENGINE_H_
#define PANTOMIR_ENGINE_H_

#include "Camera.h"
#include "VkDescriptors.h"
#include "VkLoader.h"
#include "VkTypes.h"

struct RenderObject;
struct ComputeEffect;
struct LoadedHDRI;
struct LoadedGLTF;

struct EngineStats {
	float frameTime;
	int   triangleCount;
	int   drawcallCount;
	float sceneUpdateTime;
	float meshDrawTime;
};

struct DrawContext {
	std::vector<RenderObject> opaqueSurfaces;
	std::vector<RenderObject> transparentSurfaces;
	std::vector<RenderObject> maskedSurfaces;
};

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewProjection;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; // w for sun power
	glm::vec4 sunlightColor;
};

struct DeletionQueue {
	// Stores a lambda shared-pointer
	std::deque<std::shared_ptr<std::function<void()>>> _deletionQueue;

	void                                               PushFunction(std::function<void()>&& function) {
        _deletionQueue.push_back(MakeDeletionTask(std::forward<decltype(function)>(function)));
	}

	void Flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = _deletionQueue.rbegin(); it != _deletionQueue.rend(); ++it) {
			(**it)(); // Call functors
		}

		_deletionQueue.clear();
	}

private:
	std::shared_ptr<std::function<void()>> MakeDeletionTask(auto&& lambda) {
		return std::make_shared<std::function<void()>>(std::forward<decltype(lambda)>(lambda));
	}
};

struct FrameData {
	VkSemaphore                 swapchainSemaphore {}, renderSemaphore {};
	VkFence                     renderFence {};

	VkCommandPool               commandPool {};
	VkCommandBuffer             mainCommandBuffer {};

	DeletionQueue               deletionQueue;
	DescriptorAllocatorGrowable frameDescriptors;
};

class PantomirEngine;
constexpr unsigned int             FRAME_OVERLAP = 2;
inline DescriptorAllocatorGrowable globalDescriptorAllocator;

struct GLTFMetallic_Roughness {
	MaterialPipeline      _opaquePipeline;
	MaterialPipeline      _transparentPipeline;
	MaterialPipeline      _maskedPipeline;
	// TODO: Probably want to do some bindless approach to dynamically switch between single/double sided culling methods
	MaterialPipeline      _opaqueDoubleSidedPipeline;
	MaterialPipeline      _transparentDoubleSidedPipeline;
	MaterialPipeline      _maskedDoubleSidedPipeline;

	VkDescriptorSetLayout _materialLayout;
	VkPipelineLayout      _pipelineLayout;

	// Make sure this is aligned properly.
	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metalRoughFactors;
		glm::vec3 emissiveFactors;
		float     emissiveStrength;
		float     specularFactor;
		float     alphaCutoff;
		int       alphaMode;
		float     padding1;
	};
	static_assert(sizeof(MaterialConstants) % 16 == 0, "UBO struct must be aligned to 16 bytes.");

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler      colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler      metalRoughSampler;
		AllocatedImage emissiveImage;
		VkSampler      emissiveSampler;
		AllocatedImage normalImage;
		VkSampler      normalSampler;
		AllocatedImage specularImage;
		VkSampler      specularSampler;

		VkBuffer       dataBuffer;
		uint32_t       dataBufferOffset;
	};

	DescriptorWriter _writer;

	void             BuildPipelines(PantomirEngine* engine);
	void             ClearResources(VkDevice device) const;

	MaterialInstance WriteMaterial(VkDevice device, MaterialPass pass, VkCullModeFlagBits cullMode, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

struct MeshAsset;
struct MeshNode final : Node {
	std::shared_ptr<MeshAsset> _mesh;

	void                       FillDrawContext(const glm::mat4& topMatrix, DrawContext& drawContext) override;
};

class PantomirEngine {
public:
	bool                       _bUseValidationLayers = true;

	EngineStats                _stats {};

	Camera                     _mainCamera {};

	VkPipelineLayout           _hdriPipelineLayout {};
	VkPipeline                 _hdriPipeline {};

	std::vector<ComputeEffect> _backgroundEffects {};
	int                        _currentBackgroundEffect { 0 };

	// Immediate submit structures
	VkFence                    _immediateFence {};
	VkCommandBuffer            _immediateCommandBuffer {};
	VkCommandPool              _immediateCommandPool {};

	VkPipeline                 _gradientPipeline {};
	VkPipelineLayout           _gradientPipelineLayout {};
	VkDescriptorSet            _drawImageDescriptorSet {};
	VkDescriptorSetLayout      _drawImageDescriptorLayout {};

	GPUSceneData               _sceneData {};
	VkDescriptorSetLayout      _gpuSceneDataDescriptorLayout {};

	DrawContext                _mainDrawContext {};

	AllocatedImage             _colorImage {};
	AllocatedImage             _depthImage {};
	VkExtent2D                 _drawExtent {};
	VmaAllocator               _vmaAllocator {};
	DeletionQueue              _shutdownDeletionQueue {};

	bool                       _isInitialized { false };
	int                        _frameNumber { 0 };
	bool                       _stopRendering { false };
	bool                       _resizeRequested { false };
	VkExtent2D                 _windowExtent { 1280, 720 };

	SDL_Window*                _window { nullptr };
	float                      _windowRatio = 0.8;
	float                      _renderScale = 1.f;

	VkInstance                 _instance {};       // Vulkan Library Handle
	VkDebugUtilsMessengerEXT   _debugMessenger {}; // Vulkan debug output handle
	VkPhysicalDevice           _physicalGPU {};    // GPU chosen as the default device
	VkDevice                   _logicalGPU {};     // Vulkan device for commands
	VkSurfaceKHR               _surface {};        // Vulkan window surface

	VkSwapchainKHR             _swapchain {};
	VkFormat                   _swapchainImageFormat;

	std::vector<VkImage>       _swapchainImages;
	std::vector<VkImageView>   _swapchainImageViews;
	VkExtent2D                 _swapchainExtent {};

	FrameData                  _frames[FRAME_OVERLAP];
	FrameData&                 GetCurrentFrame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
	};

	VkQueue                                                      _graphicsQueue {};
	uint32_t                                                     _graphicsQueueFamilyIndex {};

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;
	std::unordered_map<std::string, std::shared_ptr<LoadedHDRI>> _loadedHDRIs;

	MaterialInstance                                             _defaultData {};
	GLTFMetallic_Roughness                                       _metalRoughMaterial;

	AllocatedImage                                               _whiteImage {};
	AllocatedImage                                               _blackImage {};
	AllocatedImage                                               _greyImage {};
	AllocatedImage                                               _errorCheckerboardImage {};

	VkSampler                                                    _defaultSamplerLinear {};
	VkSampler                                                    _defaultSamplerNearest {};

	VkDescriptorSet                                              _singleImageDescriptorSet {};
	VkDescriptorSetLayout                                        _singleImageDescriptorLayout {};

	PantomirEngine(const PantomirEngine&)                   = delete;
	PantomirEngine&        operator=(const PantomirEngine&) = delete;

	static PantomirEngine& GetInstance() {
		static PantomirEngine instance;
		return instance;
	}

	[[nodiscard]] int             Start();
	void                          MainLoop();
	void                          ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& anonymousFunction) const;

	GPUMeshBuffers                UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) const;
	AllocatedImage                CreateHDRIImage(void* dataSource, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped = false) const;
	void                          WriteHDRIDescriptorSet();

	glm::mat4                     GetProjectionMatrix() const;

	AllocatedImage                CreateImage(void* dataSource, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped = false) const;
	void                          DestroyImage(const AllocatedImage& img) const;

	[[nodiscard]] AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;
	void                          DestroyBuffer(const AllocatedBuffer& buffer) const;

private:
	float _deltaTime = 0.0f;

	PantomirEngine();
	~PantomirEngine();

	void                         InitSDLWindow();
	void                         InitVulkan();
	void                         InitSwapchain();
	void                         InitCommands();
	void                         InitSyncStructures();
	void                         InitDescriptors();
	void                         InitPipelines();
	void                         InitBackgroundPipelines();
	void                         InitImgui();
	void                         InitMeshPipeline();
	void                         InitHDRIPipeline();
	void                         InitDefaultData();

	void                         CreateSwapchain(uint32_t width, uint32_t height);
	void                         DestroySwapchain() const;
	void                         ResizeSwapchain();

	[[nodiscard]] AllocatedImage CreateImage(const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const;

	void                         Draw();
	void                         DrawBackground(VkCommandBuffer commandBuffer);
	void                         DrawSkybox(VkCommandBuffer commandBuffer) const;
	void                         DrawGeometry(VkCommandBuffer commandBuffer);
	void                         DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView) const;

	void                         UpdateScene();
};

#endif /*! PANTOMIR_ENGINE_H_ */