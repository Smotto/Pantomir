#include <PantomirEngine.h>

#include <chrono>
#include <thread>

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "LoggerMacros.h"
#include "VKPipelines.h"
#include "VkDescriptors.h"
#include "VkImages.h"
#include "VkInitializers.h"

#include "VkLoader.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

PantomirEngine::PantomirEngine() {
	InitSDLWindow();
	InitVulkan();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
	InitDescriptors();
	InitPipelines();
	InitImgui();
	InitMeshPipeline();
	InitDefaultData();
}

PantomirEngine::~PantomirEngine() {
	// make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(_device);

	for (auto& frame : _frames) {
		// already written from before
		vkDestroyCommandPool(_device, frame._commandPool, nullptr);

		// destroy sync objects
		vkDestroyFence(_device, frame._renderFence, nullptr);
		vkDestroySemaphore(_device, frame._renderSemaphore, nullptr);
		vkDestroySemaphore(_device, frame._swapchainSemaphore, nullptr);

		frame._deletionQueue.flush();
	}

	for (auto& mesh : _testMeshes) {
		DestroyBuffer(mesh->meshBuffers.indexBuffer);
		DestroyBuffer(mesh->meshBuffers.vertexBuffer);
	}

	_mainDeletionQueue.flush();
	DestroySwapchain();

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_device, nullptr);

	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
	vkDestroyInstance(_instance, nullptr);
	SDL_DestroyWindow(_window);
}

void PantomirEngine::InitSDLWindow() {
	constexpr SDL_InitFlags   initFlags   = SDL_INIT_VIDEO;
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

	SDL_Init(initFlags);

	SDL_DisplayID id = SDL_GetPrimaryDisplay();
	SDL_Rect      displayBounds {};
	if (!SDL_GetDisplayBounds(id, &displayBounds)) {
		LOG(Engine, Error, "Failed to get display bounds: {}", SDL_GetError());
		displayBounds = { 0, 0, static_cast<int>(_windowExtent.width), static_cast<int>(_windowExtent.height) }; // Fallback
	}

	int width     = displayBounds.w * _windowRatio;
	int height    = displayBounds.h * _windowRatio;

	_windowExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	_window       = SDL_CreateWindow("Vulkan Engine",
                               width,
                               height,
                               windowFlags);

	if (_window == nullptr) {
		LOG(Engine, Error, "SDL error when creating a window: {}", SDL_GetError());
	}
}

void PantomirEngine::InitVulkan() {
	// VK_KHR_surface, VK_KHR_win32_surface are extensions that we'll get from SDL for Windows. The minimum required.
	uint32_t           extensionCount = 0;
	const char* const* extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	if (!extensionNames) {
		throw std::runtime_error("Failed to get Vulkan extensions from SDL: " + std::string(SDL_GetError()));
	}
	std::vector<const char*>   extensions(extensionNames, extensionNames + extensionCount);

	vkb::InstanceBuilder       instanceBuilder;
	vkb::Result<vkb::Instance> instanceBuilderResult = instanceBuilder.set_app_name("Vulkan Initializer")
	                                                       .request_validation_layers(true) // For debugging
	                                                       .use_default_debug_messenger()   // For debugging
	                                                       .require_api_version(1, 3, 0)    // Using Vulkan 1.3
	                                                       .enable_extensions(extensions)   // Goes through available extensions on the physical device. If all the extensions I've chosen are available, then return true;
	                                                       .build();

	// After enabling extensions for the window, we enable features for a physical device.
	vkb::Instance builtInstance = instanceBuilderResult.value();
	_instance                   = builtInstance.instance;
	_debugMessenger             = builtInstance.debug_messenger;

	// Can now create a Vulkan surface for the window for image presentation.
	SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

	// Selecting and enabling specific features for each vulkan version we want to support.
	VkPhysicalDeviceVulkan13Features features_13 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features_13.dynamicRendering = true;
	features_13.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features_12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features_12.bufferDeviceAddress = true;
	features_12.descriptorIndexing  = true;

	vkb::PhysicalDeviceSelector physicalDeviceSelector { builtInstance };
	vkb::PhysicalDevice         selectedPhysicalDevice = physicalDeviceSelector.set_minimum_version(1, 3)
	                                                 .set_required_features_13(features_13)
	                                                 .set_required_features_12(features_12)
	                                                 .set_surface(_surface)
	                                                 .select()
	                                                 .value();

	vkb::DeviceBuilder logicalDeviceBuilder { selectedPhysicalDevice };
	vkb::Device        builtLogicalDevice = logicalDeviceBuilder.build().value();

	_device                               = builtLogicalDevice.device;
	_chosenGPU                            = selectedPhysicalDevice.physical_device;

	_graphicsQueue                        = builtLogicalDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily                  = builtLogicalDevice.get_queue_index(vkb::QueueType::graphics).value(); // ID Tag for GPU logical units; Useful for command pools, buffers, images.

	VmaAllocatorCreateInfo allocatorInfo  = {};
	allocatorInfo.physicalDevice          = _chosenGPU;
	allocatorInfo.device                  = _device;
	allocatorInfo.instance                = _instance;
	allocatorInfo.flags                   = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
	});
}

void PantomirEngine::InitSwapchain() {
	CreateSwapchain(_windowExtent.width, _windowExtent.height);

	// Draw image size will match the window
	VkExtent3D drawImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	_drawImage._imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage._imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo       renderImageInfo      = vkinit::ImageCreateInfo(_drawImage._imageFormat, drawImageUsages, drawImageExtent);

	// For the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo renderImageAllocInfo = {};
	renderImageAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	renderImageAllocInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image
	vmaCreateImage(_allocator, &renderImageInfo, &renderImageAllocInfo, &_drawImage._image, &_drawImage._allocation, nullptr);

	// For the draw image to use for rendering
	VkImageViewCreateInfo renderViewInfo = vkinit::ImageviewCreateInfo(_drawImage._imageFormat, _drawImage._image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &renderViewInfo, nullptr, &_drawImage._imageView));

	_depthImage._imageFormat = VK_FORMAT_D32_SFLOAT;
	_depthImage._imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages {};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::ImageCreateInfo(_depthImage._imageFormat, depthImageUsages, drawImageExtent);

	vmaCreateImage(_allocator, &dimg_info, &renderImageAllocInfo, &_depthImage._image, &_depthImage._allocation, nullptr);

	// For the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::ImageviewCreateInfo(_depthImage._imageFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage._imageView));

	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, _drawImage._imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage._image, _drawImage._allocation);

		vkDestroyImageView(_device, _depthImage._imageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
	});
}

void PantomirEngine::InitCommands() {
	// Create a command pool for commands submitted to the graphics queue.
	// We also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(
	    _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// commands for ImGUI
	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immediateCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo commandAllocInfo = vkinit::CommandBufferAllocateInfo(_immediateCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(_device, &commandAllocInfo, &_immediateCommandBuffer));

	_mainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
	});

	for (auto& frame : _frames) {
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &frame._commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo commandAllocInfo = vkinit::CommandBufferAllocateInfo(frame._commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &commandAllocInfo, &frame._mainCommandBuffer));
	}
}

void PantomirEngine::InitSyncStructures() {
	// Create synchronization structures
	// one fence to control when the gpu has finished rendering the frame,
	// and 2 semaphores to synchronize rendering with swapchain
	// we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo     fenceCreateInfo     = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediateFence));
	_mainDeletionQueue.push_function([=]() { vkDestroyFence(_device, _immediateFence, nullptr); });
}

void PantomirEngine::InitDescriptors() {
	// Create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	globalDescriptorAllocator.Init(_device, 10, sizes);

	// make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// make the descriptor set layout for our scene draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_gpuSceneDataDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	// make the descriptor set layout for textures
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_singleImageDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	// allocate a descriptor set for our draw image
	_drawImageDescriptors = globalDescriptorAllocator.Allocate(_device, _drawImageDescriptorLayout);

	DescriptorWriter writer;
	writer.WriteImage(0, _drawImage._imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(_device, _drawImageDescriptors);

	_mainDeletionQueue.push_function([&]() {
		globalDescriptorAllocator.DestroyPools(_device);

		vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
	});

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		_frames[i]._frameDescriptors = DescriptorAllocatorGrowable {};
		_frames[i]._frameDescriptors.Init(_device, 1000, frame_sizes);

		_mainDeletionQueue.push_function([&, i]() {
			_frames[i]._frameDescriptors.DestroyPools(_device);
		});
	}
}

void PantomirEngine::InitPipelines() {
	InitBackgroundPipelines();
	_metalRoughMaterial.BuildPipelines(this);
}

void PantomirEngine::InitBackgroundPipelines() {
	VkPipelineLayoutCreateInfo computeLayout {};
	computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext          = nullptr;
	computeLayout.pSetLayouts    = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant {};
	pushConstant.offset                  = 0;
	pushConstant.size                    = sizeof(ComputePushConstants);
	pushConstant.stageFlags              = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges    = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

	VkShaderModule gradientShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/gradient_color.comp.spv", _device, &gradientShader)) {
		LOG(Engine_Renderer, Error, "Error when building the compute shader.");
	}

	VkShaderModule skyShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/sky.comp.spv", _device, &skyShader)) {
		LOG(Engine_Renderer, Error, "Error when building the compute shader.");
	}

	VkPipelineShaderStageCreateInfo stageInfo {};
	stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.pNext  = nullptr;
	stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = gradientShader;
	stageInfo.pName  = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo {};
	computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext  = nullptr;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage  = stageInfo;

	ComputeEffect gradient {};
	gradient.layout     = _gradientPipelineLayout;
	gradient.name       = "gradient";
	gradient.data       = {};

	// default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

	// change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky {};
	sky.layout     = _gradientPipelineLayout;
	sky.name       = "sky";
	sky.data       = {};
	// default sky parameters
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

	// add the 2 background effects into the array
	_backgroundEffects.push_back(gradient);
	_backgroundEffects.push_back(sky);

	// destroy structures properly
	vkDestroyShaderModule(_device, gradientShader, nullptr);
	vkDestroyShaderModule(_device, skyShader, nullptr);
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(_device, sky.pipeline, nullptr);
		vkDestroyPipeline(_device, gradient.pipeline, nullptr);
	});
}

void PantomirEngine::InitImgui() {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize       pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		                                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info    = {};
	pool_info.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags                         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets                       = 1000;
	pool_info.poolSizeCount                 = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes                    = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info                           = {};
	init_info.Instance                                            = _instance;
	init_info.PhysicalDevice                                      = _chosenGPU;
	init_info.Device                                              = _device;
	init_info.Queue                                               = _graphicsQueue;
	init_info.DescriptorPool                                      = imguiPool;
	init_info.MinImageCount                                       = 3;
	init_info.ImageCount                                          = 3;
	init_info.UseDynamicRendering                                 = true;

	// dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo                         = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;

	init_info.MSAASamples                                         = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// Add the destroy imgui created structures
	_mainDeletionQueue.push_function([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	});
}

void PantomirEngine::InitMeshPipeline() {
	VkShaderModule triangleFragShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/tex_image.frag.spv", _device, &triangleFragShader)) {
		LOG(Engine, Error, "Error when building the triangle fragment shader module");
	} else {
		LOG(Engine, Info, "Triangle fragment shader successfully loaded");
	}

	VkShaderModule triangleMeshVertexShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/colored_triangle_mesh.vert.spv", _device, &triangleMeshVertexShader)) {
		LOG(Engine, Error, "Error when building the triangle vertex shader module");
	} else {
		LOG(Engine, Info, "Triangle mesh vertex shader successfully loaded");
	}

	VkPushConstantRange bufferRange {};
	bufferRange.offset                            = 0;
	bufferRange.size                              = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags                        = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::PipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges        = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount     = 1;
	pipelineLayoutInfo.pSetLayouts                = &_singleImageDescriptorLayout;
	pipelineLayoutInfo.setLayoutCount             = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_meshPipelineLayout));

	PipelineBuilder pipelineBuilder;

	// Use the triangle layout we created
	pipelineBuilder._pipelineLayout = _meshPipelineLayout;
	// Connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.SetShaders(triangleMeshVertexShader, triangleFragShader);
	// It will draw triangles
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	// Filled triangles
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	// No Backface culling
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	// No Multisampling
	pipelineBuilder.SetMultisamplingNone();
	// Blending On/Off
	pipelineBuilder.EnableBlendingAdditive();
	//		pipelineBuilder.DisableBlending();

	pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// connect the image format we will draw into, from draw image
	pipelineBuilder.SetColorAttachmentFormat(_drawImage._imageFormat);
	pipelineBuilder.SetDepthFormat(_depthImage._imageFormat);

	// Finally build the pipeline
	_meshPipeline = pipelineBuilder.BuildPipeline(_device);

	// Clean structures
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleMeshVertexShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
		vkDestroyPipeline(_device, _meshPipeline, nullptr);
	});
}

void PantomirEngine::InitDefaultData() {
	_testMeshes                           = VkLoader::LoadGltfMeshes(this, "Assets\\Models\\basicmesh.glb").value();

	// 3 default textures, white, grey, black. 1 pixel each
	uint32_t white                        = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage                           = CreateImage((void*)&white, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey                         = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	_greyImage                            = CreateImage((void*)&grey, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black                        = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage                           = CreateImage((void*)&black, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	// Checkerboard image
	uint32_t                      magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	_errorCheckerboardImage   = CreateImage(pixels.data(), VkExtent3D { 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampl.magFilter           = VK_FILTER_NEAREST;
	sampl.minFilter           = VK_FILTER_NEAREST;

	vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

	_mainDeletionQueue.push_function([&]() {
		vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
		vkDestroySampler(_device, _defaultSamplerLinear, nullptr);

		DestroyImage(_whiteImage);
		DestroyImage(_greyImage);
		DestroyImage(_blackImage);
		DestroyImage(_errorCheckerboardImage);
	});

	GLTFMetallic_Roughness::MaterialResources materialResources;
	// default the material textures
	materialResources.colorImage                                 = _whiteImage;
	materialResources.colorSampler                               = _defaultSamplerLinear;
	materialResources.metalRoughImage                            = _whiteImage;
	materialResources.metalRoughSampler                          = _defaultSamplerLinear;

	// set the uniform buffer for the material data
	AllocatedBuffer                            materialConstants = CreateBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// write the buffer
	GLTFMetallic_Roughness::MaterialConstants* sceneUniformData  = (GLTFMetallic_Roughness::MaterialConstants*)materialConstants.allocation->GetMappedData();
	sceneUniformData->colorFactors                               = glm::vec4 { 1, 1, 1, 1 };
	sceneUniformData->metal_rough_factors                        = glm::vec4 { 1, 0.5, 0, 0 };

	_mainDeletionQueue.push_function([=, this]() {
		DestroyBuffer(materialConstants);
	});

	materialResources.dataBuffer       = materialConstants.buffer;
	materialResources.dataBufferOffset = 0;

	_defaultData                       = _metalRoughMaterial.WriteMaterial(_device, MaterialPass::MainColor, materialResources, globalDescriptorAllocator);
}

GPUMeshBuffers PantomirEngine::UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) {
	const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t   indexBufferSize  = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	// Create vertex buffer
	newSurface.vertexBuffer = CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	// Find the address of the vertex buffer
	VkBufferDeviceAddressInfo deviceAddressInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAddressInfo);

	// Create index buffer
	newSurface.indexBuffer         = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging        = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void*           data           = staging.allocation->GetMappedData();

	// Copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// Copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy { 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size      = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy { 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size      = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	DestroyBuffer(staging);

	return newSurface;
}

AllocatedBuffer PantomirEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	// Allocate Buffer
	VkBufferCreateInfo bufferInfo        = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext                     = nullptr;
	bufferInfo.size                      = allocSize;

	bufferInfo.usage                     = usage;
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage                   = memoryUsage;
	vmaallocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// Allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

void PantomirEngine::CreateSwapchain(uint32_t width, uint32_t height) {
	vkb::SwapchainBuilder swapchainBuilder { _chosenGPU, _device, _surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain builtSwapchain =
	    swapchainBuilder
	        //.use_default_format_selection()
	        .set_desired_format(VkSurfaceFormatKHR {
	            .format     = _swapchainImageFormat,
	            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	        // use vsync present mode
	        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
	        .set_desired_extent(width, height)
	        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	        .build()
	        .value();

	_swapchainExtent     = builtSwapchain.extent;
	// Store swapchain and its related images
	_swapchain           = builtSwapchain.swapchain;
	_swapchainImages     = builtSwapchain.get_images().value();
	_swapchainImageViews = builtSwapchain.get_image_views().value();
}

void PantomirEngine::DestroySwapchain() {
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// Destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void PantomirEngine::DestroyBuffer(const AllocatedBuffer& buffer) {
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

int PantomirEngine::Start() {
	try {
		MainLoop();
	} catch (const std::exception& exception) {
		LOG(Engine, Error, "Exception: ", exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void PantomirEngine::Draw() {
	// CPU-GPU synchronization: wait until the gpu has finished rendering the last frame. Timeout of 1 second
	VK_CHECK(vkWaitForFences(_device, 1, &GetCurrentFrame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &GetCurrentFrame()._renderFence));

	GetCurrentFrame()._deletionQueue.flush();
	GetCurrentFrame()._frameDescriptors.ClearPools(_device);

	uint32_t swapchainImageIndex;
	VkResult acquireNextImageKhr = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, GetCurrentFrame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (acquireNextImageKhr == VK_ERROR_OUT_OF_DATE_KHR) {
		_resizeRequested = true;
		return;
	}

	VkCommandBuffer commandBuffer = GetCurrentFrame()._mainCommandBuffer;

	// Now that we are sure that the commands finished executing, we can safely
	// Reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

	// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo command_buffer_begin_info = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	_drawExtent.height                                 = std::min(_swapchainExtent.height, _drawImage._imageExtent.height) * _renderScale;
	_drawExtent.width                                  = std::min(_swapchainExtent.width, _drawImage._imageExtent.width) * _renderScale;

	/* Start Recording */
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &command_buffer_begin_info));

	// Transition our main draw image into general layout so we can write into it
	// We will overwrite it all so we don't care about what was the older layout
	vkutil::TransitionImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(commandBuffer);

	vkutil::TransitionImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkutil::TransitionImage(commandBuffer, _depthImage._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	DrawGeometry(commandBuffer);

	// Transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::TransitionImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); // Make the swapchain optimal for transfer

	// Execute a copy from the draw image into the swapchain
	vkutil::CopyImageToImage(commandBuffer, _drawImage._image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

	// Set swapchain image layout to Present so we can show it on the screen
	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); // Set the swapchain layout back, now presenting.

	DrawImgui(commandBuffer, _swapchainImageViews[swapchainImageIndex]);

	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
	/* End Recording */

	// Prepare the submission to the queue.
	// We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// We will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo commandBufferSubmitInfo = vkinit::CommandBufferSubmitInfo(commandBuffer);

	VkSemaphoreSubmitInfo     waitInfo                = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo     signalInfo              = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame()._renderSemaphore);

	VkSubmitInfo2             submitInfo              = vkinit::SubmitInfo(&commandBufferSubmitInfo, &signalInfo, &waitInfo);

	// Submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, GetCurrentFrame()._renderFence));

	// Prepare present
	//  this will put the image we just rendered to into the visible window.
	//  We want to wait on the _renderSemaphore for that,
	//  as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext              = nullptr;
	presentInfo.pSwapchains        = &_swapchain;
	presentInfo.swapchainCount     = 1;

	presentInfo.pWaitSemaphores    = &GetCurrentFrame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices      = &swapchainImageIndex;

	VkResult presentResult         = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		_resizeRequested = true;
	}

	// increase the number of frames drawn
	_frameNumber++;
}

void PantomirEngine::DrawBackground(VkCommandBuffer commandBuffer) {
	ComputeEffect& effect = _backgroundEffects[_currentBackgroundEffect];

	// bind the background compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(commandBuffer, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(commandBuffer, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void PantomirEngine::DrawGeometry(VkCommandBuffer commandBuffer) {
	// Allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = CreateBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Add it to the deletion queue of this frame so it gets deleted once it's been used
	GetCurrentFrame()._deletionQueue.push_function([=, this]() {
		DestroyBuffer(gpuSceneDataBuffer);
	});

	// Write the buffer
	GPUSceneData* sceneUniformData    = static_cast<GPUSceneData*>(gpuSceneDataBuffer.allocation->GetMappedData());
	*sceneUniformData                 = _sceneData;

	// Create a descriptor set that binds that buffer and update it
	VkDescriptorSet  globalDescriptor = GetCurrentFrame()._frameDescriptors.Allocate(_device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter uniformWriter;
	uniformWriter.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	uniformWriter.UpdateSet(_device, globalDescriptor);

	// Begin a render pass connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(_drawImage._imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depthAttachment = vkinit::DepthAttachmentInfo(_depthImage._imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	VkRenderingInfo           renderInfo      = vkinit::RenderingInfo(_drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(commandBuffer, &renderInfo);

	// Set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x          = 0;
	viewport.y          = 0;
	viewport.width      = _drawExtent.width;
	viewport.height     = _drawExtent.height;
	viewport.minDepth   = 0.f;
	viewport.maxDepth   = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor      = {};
	scissor.offset.x      = 0;
	scissor.offset.y      = 0;
	scissor.extent.width  = _drawExtent.width;
	scissor.extent.height = _drawExtent.height;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Bind mesh pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

	VkDescriptorSet imageSet = GetCurrentFrame()._frameDescriptors.Allocate(_device, _singleImageDescriptorLayout);
	assert(imageSet != VK_NULL_HANDLE);
	{
		DescriptorWriter imageWriter;
		imageWriter.WriteImage(0, _errorCheckerboardImage._imageView, _defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		imageWriter.UpdateSet(_device, imageSet);
	}

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &imageSet, 0, nullptr);

	// Camera and transform setup
	glm::mat4 view       = glm::translate(glm::vec3 { 0, 0, -5 });
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)_drawExtent.width / (float)_drawExtent.height, 10000.f, 0.1f);
	projection[1][1] *= -1; // Flip Y for GLTF-style axis

	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix  = projection * view;
	push_constants.vertexBuffer = _testMeshes[2]->meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(commandBuffer, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(commandBuffer, _testMeshes[2]->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, _testMeshes[2]->surfaces[0].count, 1, _testMeshes[2]->surfaces[0].startIndex, 0, 0);

	vkCmdEndRendering(commandBuffer);
}

void PantomirEngine::DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView) {
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo           renderInfo      = vkinit::RenderingInfo(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void PantomirEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& anonymousFunction) {
	VK_CHECK(vkResetFences(_device, 1, &_immediateFence));
	VK_CHECK(vkResetCommandBuffer(_immediateCommandBuffer, 0));

	VkCommandBuffer          cmd          = _immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	anonymousFunction(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo commandInfo = vkinit::CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2             submit      = vkinit::SubmitInfo(&commandInfo, nullptr, nullptr);

	// Submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateFence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immediateFence, true, 9999999999));
}

void PantomirEngine::MainLoop() {
	SDL_Event e;
	bool      bQuit = false;

	while (!bQuit) {
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// Close the window when user alt-f4s or clicks the X button
			if (e.type == SDL_EVENT_QUIT) {
				bQuit = true;
			}

			if (e.type == SDL_EVENT_WINDOW_RESIZED) {
				_resizeRequested = true;
			}

			if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
				_stopRendering = true;
			}
			if (e.type == SDL_EVENT_WINDOW_RESTORED) {
				_stopRendering = false;
			}

			ImGui_ImplSDL3_ProcessEvent(&e);
		}

		// Do not draw if we are minimized
		if (_stopRendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (_resizeRequested) {
			ResizeSwapchain();
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("background")) {

			ComputeEffect& selected = _backgroundEffects[_currentBackgroundEffect];

			ImGui::SliderFloat("Render Scale", &_renderScale, 0.3f, 1.f);

			ImGui::Text("Selected effect: ", selected.name);

			ImGui::SliderInt("Effect Index", &_currentBackgroundEffect, 0, _backgroundEffects.size() - 1);

			ImGui::InputFloat4("data1", (float*)&selected.data.data1);
			ImGui::InputFloat4("data2", (float*)&selected.data.data2);
			ImGui::InputFloat4("data3", (float*)&selected.data.data3);
			ImGui::InputFloat4("data4", (float*)&selected.data.data4);
		}
		ImGui::End();

		ImGui::Render();

		Draw();
	}
}

void PantomirEngine::ResizeSwapchain() {
	vkDeviceWaitIdle(_device);
	DestroySwapchain();
	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width  = w;
	_windowExtent.height = h;
	CreateSwapchain(w, h);
	_resizeRequested = false;
}

AllocatedImage PantomirEngine::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
	AllocatedImage newImage;
	newImage._imageFormat      = format;
	newImage._imageExtent      = size;

	VkImageCreateInfo img_info = vkinit::ImageCreateInfo(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage._image, &newImage._allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build an image-view for the image
	VkImageViewCreateInfo view_info       = vkinit::ImageviewCreateInfo(format, newImage._image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage._imageView));

	return newImage;
}

AllocatedImage PantomirEngine::CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
	size_t          data_size    = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	ImmediateSubmit([&](VkCommandBuffer cmd) {
		vkutil::TransitionImage(cmd, new_image._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion               = {};
		copyRegion.bufferOffset                    = 0;
		copyRegion.bufferRowLength                 = 0;
		copyRegion.bufferImageHeight               = 0;

		copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel       = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount     = 1;
		copyRegion.imageExtent                     = size;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		vkutil::TransitionImage(cmd, new_image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DestroyBuffer(uploadbuffer);

	return new_image;
}

void PantomirEngine::DestroyImage(const AllocatedImage& img) {
	vkDestroyImageView(_device, img._imageView, nullptr);
	vmaDestroyImage(_allocator, img._image, img._allocation);
}

int main(int argc, char* argv[]) {
	return PantomirEngine::GetInstance().Start();
}

void GLTFMetallic_Roughness::BuildPipelines(PantomirEngine* engine) {
	VkShaderModule meshFragShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/mesh.frag.spv", engine->_device, &meshFragShader)) {
		LOG(Engine, Error, "Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
		LOG(Engine, Error, "Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange {};
	matrixRange.offset     = 0;
	matrixRange.size       = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	materialLayout                              = layoutBuilder.Build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout      layouts[]        = { engine->_gpuSceneDataDescriptorLayout,
		                                            materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::PipelineLayoutCreateInfo();
	mesh_layout_info.setLayoutCount             = 2;
	mesh_layout_info.pSetLayouts                = layouts;
	mesh_layout_info.pPushConstantRanges        = &matrixRange;
	mesh_layout_info.pushConstantRangeCount     = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

	opaquePipeline.layout      = newLayout;
	transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// render format
	pipelineBuilder.SetColorAttachmentFormat(engine->_drawImage._imageFormat);
	pipelineBuilder.SetDepthFormat(engine->_depthImage._imageFormat);

	// use the triangle layout we created
	pipelineBuilder._pipelineLayout = newLayout;

	// finally build the pipeline
	opaquePipeline.pipeline         = pipelineBuilder.BuildPipeline(engine->_device);

	// create the transparent variant
	pipelineBuilder.EnableBlendingAdditive();

	pipelineBuilder.EnableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_device);

	vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
	vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
}

void GLTFMetallic_Roughness::ClearResources(VkDevice device) {
}

MaterialInstance GLTFMetallic_Roughness::WriteMaterial(VkDevice device, MaterialPass pass, const GLTFMetallic_Roughness::MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator) {
	MaterialInstance matData {};

	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	} else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.Allocate(device, materialLayout);

	writer.Clear();
	writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, resources.colorImage._imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, resources.metalRoughImage._imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(device, matData.materialSet);

	return matData;
}
