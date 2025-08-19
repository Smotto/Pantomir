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
	// Stores a lambda shared-pointer
	std::deque<std::shared_ptr<std::function<void()>>> _deletionQueue;

	std::shared_ptr<std::function<void()>> MakeDeletionTask(auto&& lambda) {
		return std::make_shared<std::function<void()>>(std::forward<decltype(lambda)>(lambda));
	}
};

struct FrameData {
	VkSemaphore           swapchainSemaphore {}, renderSemaphore {};
	VkFence               renderFence {};

	VkCommandPool         commandPool {};
	VkCommandBuffer       mainCommandBuffer {};

	DeletionQueue         deletionQueue;
	DescriptorPoolManager descriptorPoolManager;
};

class PantomirEngine;
constexpr unsigned int FRAME_OVERLAP = 2;

struct GLTFMetallic_Roughness {
	MaterialPipeline      _opaquePipeline;
	MaterialPipeline      _transparentPipeline;
	MaterialPipeline      _maskedPipeline;
	// TODO: Probably want to do some bindless approach to dynamically switch between single/double sided culling methods
	MaterialPipeline      _opaqueDoubleSidedPipeline;
	MaterialPipeline      _transparentDoubleSidedPipeline;
	MaterialPipeline      _maskedDoubleSidedPipeline;

	VkDescriptorSetLayout _materialDescriptorSetLayout;
	VkPipelineLayout      _pipelineLayout;

	// Make sure this is aligned properly.
	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metalRoughFactors;
		glm::vec3 emissiveFactors;
		float     emissiveStrength;
		float     specularFactor;
		float     alphaCutoff;
		alignas(8) int alphaMode;
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

	DescriptorSetWriter _writer;

	void                BuildPipelines(PantomirEngine* engine);
	void                ClearResources(VkDevice device) const;

	MaterialInstance    WriteMaterial(VkDevice device, MaterialPass passType, VkCullModeFlagBits cullMode, const MaterialResources& resources, DescriptorPoolManager& descriptorPoolManager);
};

inline bool IsVisible(const RenderObject& renderObject, const glm::mat4& viewProjection) {
	constexpr std::array<glm::vec3, 8> unitCubeCorners {
		glm::vec3 { 1, 1, 1 },
		glm::vec3 { 1, 1, -1 },
		glm::vec3 { 1, -1, 1 },
		glm::vec3 { 1, -1, -1 },
		glm::vec3 { -1, 1, 1 },
		glm::vec3 { -1, 1, -1 },
		glm::vec3 { -1, -1, 1 },
		glm::vec3 { -1, -1, -1 },
	};

	const glm::mat4 objectToClipSpaceMatrix = viewProjection * renderObject.transform;

	glm::vec3       clipSpaceMin = glm::vec3 { 1.5F };
	glm::vec3       clipSpaceMax = glm::vec3 { -1.5F };

	// Project each corner of the bounding box into clip space.
	for (const glm::vec3& localCorner : unitCubeCorners) {
		glm::vec3 worldCorner = renderObject.bounds.originPoint + (localCorner * renderObject.bounds.extents);
		glm::vec4 clipSpaceCorner = objectToClipSpaceMatrix * glm::vec4(worldCorner, 1.0F);

		// Divide by w to convert to normalized clip space (from -1 to 1)
		glm::vec3 normalizedCorner = glm::vec3(clipSpaceCorner) / clipSpaceCorner.w;

		// Expand the bounding box in clip space
		clipSpaceMin = glm::min(clipSpaceMin, normalizedCorner);
		clipSpaceMax = glm::max(clipSpaceMax, normalizedCorner);
	}

	constexpr glm::vec3 clipBoundsMin = { -1.0F, -1.0F, 0.0F };
	constexpr glm::vec3 clipBoundsMax = { 1.0F, 1.0F, 1.0F };

	// If the box is fully outside the clip space in any direction, it's not visible
	bool                outOfBounds =
	    clipSpaceMin.x > clipBoundsMax.x || clipSpaceMax.x < clipBoundsMin.x ||
	    clipSpaceMin.y > clipBoundsMax.y || clipSpaceMax.y < clipBoundsMin.y ||
	    clipSpaceMin.z > clipBoundsMax.z || clipSpaceMax.z < clipBoundsMin.z;

	return !outOfBounds;
}

inline void BuildDrawListByMaterialMesh(const std::vector<RenderObject>& surfaces,
                                        const glm::mat4&                 viewProjection,
                                        std::vector<uint32_t>&           out_indices) {
	out_indices.clear();
	out_indices.reserve(surfaces.size());

	// Visibility culling
	for (uint32_t index = 0; index < static_cast<uint32_t>(surfaces.size()); ++index) {
		if (IsVisible(surfaces[index], viewProjection)) {
			out_indices.push_back(index);
		}
	}

	// Sort by material, then by mesh index
	std::ranges::sort(out_indices, [&](const uint32_t& a, const uint32_t& b) {
		const RenderObject& A = surfaces[a];
		const RenderObject& B = surfaces[b];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		return A.material < B.material; });
}

inline void BuildDrawListTransparent(const std::vector<RenderObject>& surfaces,
                                     const glm::mat4&                 viewProjection,
                                     const glm::vec3&                 cameraPos,
                                     std::vector<uint32_t>&           out_indices) {
	out_indices.clear();
	out_indices.reserve(surfaces.size());

	// Visibility culling
	for (uint32_t index = 0; index < static_cast<uint32_t>(surfaces.size()); ++index) {
		if (IsVisible(surfaces[index], viewProjection)) {
			out_indices.push_back(index);
		}
	}

	// Sort farthest to nearest (squared distance for speed)
	std::ranges::sort(out_indices, [&](const uint32_t& a, const uint32_t& b) {
		const glm::vec3 posA = glm::vec3(surfaces[a].transform[3]);
		const glm::vec3 posB = glm::vec3(surfaces[b].transform[3]);
		const float     distA2 = glm::dot(cameraPos - posA, cameraPos - posA);
		const float     distB2 = glm::dot(cameraPos - posB, cameraPos - posB);
		return distA2 > distB2; // farthest first
	});
}

class PantomirEngine {
public:
	bool                     _bUseValidationLayers = true;

	EngineStats              _stats {};

	Camera                   _mainCamera {};

	VkPipelineLayout         _hdriPipelineLayout {};
	VkPipeline               _hdriPipeline {};

	VkFence                  _immediateFence {};
	VkCommandBuffer          _immediateCommandBuffer {};
	VkCommandPool            _immediateCommandPool {};

	GPUSceneData             _sceneData {};
	VkDescriptorSetLayout    _gpuSceneDataDescriptorSetLayout {};
	VkDescriptorSetLayout    _hdriDescriptorSetLayout {};

	DrawContext              _mainDrawContext {};

	AllocatedImage           _colorImage {};
	AllocatedImage           _depthImage {};
	VkExtent2D               _drawExtent {};
	VmaAllocator             _vmaAllocator {};
	DeletionQueue            _shutdownDeletionQueue {};

	int                      _frameNumber { 0 };
	bool                     _stopRendering { false };
	bool                     _resizeRequested { false };
	VkExtent2D               _windowExtent { 1280, 720 };

	SDL_Window*              _window { nullptr };
	float                    _windowRatio = 0.8F;
	float                    _renderScale = 1.F;

	VkInstance               _instance {};       // Vulkan Library Handle
	VkDebugUtilsMessengerEXT _debugMessenger {}; // Vulkan debug output handle
	VkPhysicalDevice         _physicalGPU {};    // GPU chosen as the default device
	VkDevice                 _logicalGPU {};     // Vulkan device for commands
	VkSurfaceKHR             _surface {};        // Vulkan window surface

	VkSwapchainKHR           _swapchain {};
	VkFormat                 _swapchainImageFormat {};

	std::vector<VkImage>     _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D               _swapchainExtent {};

	FrameData                _frames[FRAME_OVERLAP];
	FrameData&               GetCurrentFrame() {
        return _frames[_frameNumber % FRAME_OVERLAP];
	};

	VkQueue                                                      _graphicsQueue {};
	uint32_t                                                     _graphicsQueueFamilyIndex {};

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;
	std::unordered_map<std::string, std::shared_ptr<LoadedHDRI>> _loadedHDRIs;
	std::shared_ptr<LoadedHDRI>                                  _currentHDRI;

	GLTFMetallic_Roughness                                       _metalRoughMaterial;
	MaterialInstance                                             _defaultMaterialInstance {};

	AllocatedImage                                               _whiteImage {};
	AllocatedImage                                               _blackImage {};
	AllocatedImage                                               _greyImage {};
	AllocatedImage                                               _errorCheckerboardImage {};

	VkSampler                                                    _defaultSamplerLinear {};
	VkSampler                                                    _defaultSamplerNearest {};

	PantomirEngine(const PantomirEngine&) = delete;
	PantomirEngine&        operator=(const PantomirEngine&) = delete;

	static PantomirEngine& GetInstance() {
		static PantomirEngine instance;
		return instance;
	}

	[[nodiscard]] int             Start();
	void                          MainLoop();
	void                          ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& anonymousFunction) const;

	[[nodiscard]] GPUMeshBuffers  UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) const;

	[[nodiscard]] glm::mat4       GetProjectionMatrix() const;

	AllocatedImage                CreateImage(void* dataSource, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped = false) const;
	void                          DestroyImage(const AllocatedImage& img) const;

	[[nodiscard]] AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) const;
	void                          DestroyBuffer(const AllocatedBuffer& buffer) const;

private:
	float _deltaTime = 0.0F;
	float _minDeltaTimeClamp = 0.0001F;
	float _maxDeltaTimeClamp = 0.016F;

	PantomirEngine();
	~PantomirEngine();

	void InitSDLWindow();
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitImgui();
	void InitHDRIPipeline();
	void InitDefaultData();

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain() const;
	void ResizeSwapchain();

	void SetViewport(const VkCommandBuffer& commandBuffer);
	void SetScissor(const VkCommandBuffer& commandBuffer);

	bool AcquireSwapchainImage(uint32_t& swapchainImageIndex);

	void Draw();
	void DrawHDRI(VkCommandBuffer commandBuffer);
	void DrawGeometry(VkCommandBuffer commandBuffer);
	void DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView) const;

	void PresentSwapchainImage(const uint32_t swapchainImageIndex);

	void UpdateScene();
};

#endif /*! PANTOMIR_ENGINE_H_ */