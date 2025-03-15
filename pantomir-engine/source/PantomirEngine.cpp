#include <PantomirEngine.h>

#include <chrono>
#include <iostream>
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

PantomirEngine::PantomirEngine() {
	InitSDLWindow();
	InitVulkan();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
	InitDescriptors();
	InitPipelines();
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
	_mainDeletionQueue.flush();
	DestroySwapchain();

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_device, nullptr);

	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
	vkDestroyInstance(_instance, nullptr);
	SDL_DestroyWindow(_window);
}

void PantomirEngine::InitSDLWindow() {
	constexpr SDL_InitFlags   init_flags   = SDL_INIT_VIDEO;
	constexpr SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;

	SDL_Init(init_flags);
	_window = SDL_CreateWindow("Vulkan Engine", _windowExtent.width, _windowExtent.height, window_flags);
	if (_window == nullptr) {
		LOG(Engine, Error, "SDL error when creating a window: {}", SDL_GetError());
	}
}

void PantomirEngine::InitVulkan() {
	// VK_KHR_surface, VK_KHR_win32_surface are extensions that we'll get from SDL for Windows. The minimum required.
	uint32_t           extension_count = 0;
	const char* const* extension_names = SDL_Vulkan_GetInstanceExtensions(&extension_count);
	if (!extension_names) {
		throw std::runtime_error("Failed to get Vulkan extensions from SDL: " + std::string(SDL_GetError()));
	}
	std::vector<const char*>   extensions(extension_names, extension_names + extension_count);

	vkb::InstanceBuilder       instance_builder;
	vkb::Result<vkb::Instance> instance_builder_result = instance_builder.set_app_name("Vulkan Initializer")
	                                                         .request_validation_layers(true) // For debugging
	                                                         .use_default_debug_messenger()   // For debugging
	                                                         .require_api_version(1, 3, 0)    // Using Vulkan 1.3
	                                                         .enable_extensions(extensions)   // Goes through available extensions on the physical device. If all the extensions I've chosen are available, then return true;
	                                                         .build();

	// After enabling extensions for the window, we enable features for a physical device.
	vkb::Instance built_instance = instance_builder_result.value();
	_instance                    = built_instance.instance;
	_debugMessenger              = built_instance.debug_messenger;

	// Can now create a Vulkan surface for the window for image presentation.
	SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

	// Selecting and enabling specific features for each vulkan version we want to support.
	VkPhysicalDeviceVulkan13Features features_13 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features_13.dynamicRendering = true;
	features_13.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features_12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features_12.bufferDeviceAddress = true;
	features_12.descriptorIndexing  = true;

	vkb::PhysicalDeviceSelector physical_device_selector { built_instance };
	vkb::PhysicalDevice         selected_physical_device = physical_device_selector.set_minimum_version(1, 3)
	                                                   .set_required_features_13(features_13)
	                                                   .set_required_features_12(features_12)
	                                                   .set_surface(_surface)
	                                                   .select()
	                                                   .value();

	vkb::DeviceBuilder logical_device_builder { selected_physical_device };
	vkb::Device        built_logical_device = logical_device_builder.build().value();

	_device                                 = built_logical_device.device;
	_chosen_GPU                             = selected_physical_device.physical_device;

	_graphicsQueue                          = built_logical_device.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily                    = built_logical_device.get_queue_index(vkb::QueueType::graphics).value(); // ID Tag for GPU logical units; Useful for command pools, buffers, images.

	VmaAllocatorCreateInfo allocatorInfo    = {};
	allocatorInfo.physicalDevice            = _chosen_GPU;
	allocatorInfo.device                    = _device;
	allocatorInfo.instance                  = _instance;
	allocatorInfo.flags                     = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
	});
}

void PantomirEngine::InitSwapchain() {
	CreateSwapchain(_windowExtent.width, _windowExtent.height);

	// Draw image size will match the window
	VkExtent3D draw_image_extent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	_drawImage._imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage._imageExtent = draw_image_extent;

	VkImageUsageFlags drawImageUsages {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo       render_image_info       = vkinit::ImageCreateInfo(_drawImage._imageFormat, drawImageUsages, draw_image_extent);

	// For the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo render_image_alloc_info = {};
	render_image_alloc_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	render_image_alloc_info.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	vmaCreateImage(_allocator, &render_image_info, &render_image_alloc_info, &_drawImage._image, &_drawImage._allocation, nullptr);

	// build an image-view for the draw image to use for rendering
	VkImageViewCreateInfo render_view_info = vkinit::ImageviewCreateInfo(_drawImage._imageFormat, _drawImage._image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &render_view_info, nullptr, &_drawImage._imageView));

	// add to deletion queues
	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, _drawImage._imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage._image, _drawImage._allocation);
	});
}

void PantomirEngine::InitCommands() {
	// create a command pool for commands submitted to the graphics queue.
	// we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo command_pool_info = vkinit::CommandPoolCreateInfo(
	    _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (auto& frame : _frames) {
		VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &frame._commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo command_alloc_info =
		    vkinit::CommandBufferAllocateInfo(frame._commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &command_alloc_info, &frame._mainCommandBuffer));
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
}

void PantomirEngine::InitDescriptors() {
	// create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	globalDescriptorAllocator.InitPool(_device, 10, sizes);

	// make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// allocate a descriptor set for our draw image
	_drawImageDescriptors = globalDescriptorAllocator.Allocate(_device, _drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo {};
	imgInfo.imageLayout                 = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView                   = _drawImage._imageView;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext                = nullptr;

	drawImageWrite.dstBinding           = 0;
	drawImageWrite.dstSet               = _drawImageDescriptors;
	drawImageWrite.descriptorCount      = 1;
	drawImageWrite.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo           = &imgInfo;

	vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

	_mainDeletionQueue.push_function([&]() {
		globalDescriptorAllocator.DestroyPool(_device);

		vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
	});
}

void PantomirEngine::InitPipelines() {
	InitBackgroundPipelines();
}

void PantomirEngine::InitBackgroundPipelines() {
	VkPipelineLayoutCreateInfo computeLayout {};
	computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext          = nullptr;
	computeLayout.pSetLayouts    = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

	VkShaderModule computeDrawShader;

	if (!vkutil::LoadShaderModule("Assets/Shaders/gradient.comp.spv", _device, &computeDrawShader)) {
		LOG(Engine_Renderer, Debug, "Error when building the compute shader.");
	}

	VkPipelineShaderStageCreateInfo stageinfo {};
	stageinfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext  = nullptr;
	stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName  = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo {};
	computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext  = nullptr;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage  = stageinfo;

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));

	vkDestroyShaderModule(_device, computeDrawShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(_device, _gradientPipeline, nullptr);
	});
}

void PantomirEngine::CreateSwapchain(uint32_t width, uint32_t height) {
	vkb::SwapchainBuilder swapchain_builder { _chosen_GPU, _device, _surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain built_swapchain =
	    swapchain_builder
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

	_swapchainExtent     = built_swapchain.extent;
	// store swapchain and its related images
	_swapchain           = built_swapchain.swapchain;
	_swapchainImages     = built_swapchain.get_images().value();
	_swapchainImageViews = built_swapchain.get_image_views().value();
}

void PantomirEngine::DestroySwapchain() {
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
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
	// CPU-GPU synchronization
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	get_current_frame()._deletionQueue.flush();

	uint32_t swapchain_image_index;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchain_image_index));

	VkCommandBuffer command_buffer = get_current_frame()._mainCommandBuffer;

	// Now that we are sure that the commands finished executing, we can safely
	// Reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(command_buffer, 0));

	// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo command_buffer_begin_info = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	_drawExtent.width                                  = _drawImage._imageExtent.width;
	_drawExtent.height                                 = _drawImage._imageExtent.height;

	/* Start Recording */
	VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

	// Transition our main draw image into general layout so we can write into it
	// We will overwrite it all so we don't care about what was the older layout
	vkutil::TransitionImage(command_buffer, _drawImage._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(command_buffer);

	// Transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::TransitionImage(command_buffer, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::TransitionImage(command_buffer, _swapchainImages[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	vkutil::CopyImageToImage(command_buffer, _drawImage._image, _swapchainImages[swapchain_image_index], _drawExtent, _swapchainExtent);

	// Set swapchain image layout to Present so we can show it on the screen
	vkutil::TransitionImage(command_buffer, _swapchainImages[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(command_buffer));
	/* End Recording */

	// Prepare the submission to the queue.
	// We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// We will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo command_buffer_submit_info = vkinit::CommandBufferSubmitInfo(command_buffer);

	VkSemaphoreSubmitInfo     wait_info                  = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo     signal_info                = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

	VkSubmitInfo2             submit_info                = vkinit::SubmitInfo(&command_buffer_submit_info, &signal_info, &wait_info);

	// Submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit_info, get_current_frame()._renderFence));

	// Prepare present
	//  this will put the image we just rendered to into the visible window.
	//  We want to wait on the _renderSemaphore for that,
	//  as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext              = nullptr;
	presentInfo.pSwapchains        = &_swapchain;
	presentInfo.swapchainCount     = 1;

	presentInfo.pWaitSemaphores    = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices      = &swapchain_image_index;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	// increase the number of frames drawn
	_frameNumber++;
}

void PantomirEngine::DrawBackground(VkCommandBuffer command_buffer) {
	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(command_buffer, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
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
		}

		// Do not draw if we are minimized
		if (_stop_rendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		Draw();
	}
}

int main(int argc, char* argv[]) {
	return PantomirEngine::GetInstance().Start();
}
