#include "LoggerMacros.h"
#include "VkImages.h"
#include "VkInitializers.h"

#include <PantomirEngine.h>

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <chrono>
#include <iostream>
#include <thread>

PantomirEngine::PantomirEngine() {
	initSDLWindow();
	initVulkan();
	initSwapchain();
	initCommands();
	initSyncStructures();
}

PantomirEngine::~PantomirEngine() {
	// make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(_device);

	for (auto& frame : _frames) {
		// already written from before
		vkDestroyCommandPool(_device, frame._command_pool, nullptr);

		// destroy sync objects
		vkDestroyFence(_device, frame._render_fence, nullptr);
		vkDestroySemaphore(_device, frame._render_semaphore, nullptr);
		vkDestroySemaphore(_device, frame._swapchain_semaphore, nullptr);

		frame._deletion_queue.flush();
	}
	_main_deletion_queue.flush();
	destroySwapchain();

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_device, nullptr);

	vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
	vkDestroyInstance(_instance, nullptr);
	SDL_DestroyWindow(_window);
}

void PantomirEngine::initSDLWindow() {
	constexpr SDL_InitFlags   init_flags   = SDL_INIT_VIDEO;
	constexpr SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;

	SDL_Init(init_flags);
	_window = SDL_CreateWindow("Vulkan Engine", _window_extent.width, _window_extent.height, window_flags);
	if (_window == nullptr) {
		LOG(Engine, Error, "SDL error when creating a window: {}", SDL_GetError());
	}
}

void PantomirEngine::initVulkan() {
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
	_debug_messenger             = built_instance.debug_messenger;

	// Can now create a Vulkan surface for the window for image presentation.
	SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface); // TODO: Fill in allocator

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

	_graphics_queue                         = built_logical_device.get_queue(vkb::QueueType::graphics).value();
	_graphics_queue_family                  = built_logical_device.get_queue_index(vkb::QueueType::graphics).value(); // ID Tag for GPU logical units; Useful for command pools, buffers, images.

	VmaAllocatorCreateInfo allocatorInfo    = {};
	allocatorInfo.physicalDevice            = _chosen_GPU;
	allocatorInfo.device                    = _device;
	allocatorInfo.instance                  = _instance;
	allocatorInfo.flags                     = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_main_deletion_queue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
	});
}

void PantomirEngine::initSwapchain() {
	createSwapchain(_window_extent.width, _window_extent.height);

	// Draw image size will match the window
	VkExtent3D draw_image_extent = {
		_window_extent.width,
		_window_extent.height,
		1
	};

	_draw_image.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_draw_image.imageExtent = draw_image_extent;

	VkImageUsageFlags drawImageUsages {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo       render_image_info       = vkinit::image_create_info(_draw_image.imageFormat, drawImageUsages, draw_image_extent);

	// For the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo render_image_alloc_info = {};
	render_image_alloc_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	render_image_alloc_info.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	vmaCreateImage(_allocator, &render_image_info, &render_image_alloc_info, &_draw_image.image, &_draw_image.allocation, nullptr);

	// build an image-view for the draw image to use for rendering
	VkImageViewCreateInfo render_view_info = vkinit::imageview_create_info(_draw_image.imageFormat, _draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &render_view_info, nullptr, &_draw_image.imageView));

	// add to deletion queues
	_main_deletion_queue.push_function([=]() {
		vkDestroyImageView(_device, _draw_image.imageView, nullptr);
		vmaDestroyImage(_allocator, _draw_image.image, _draw_image.allocation);
	});
}

void PantomirEngine::initCommands() {
	// create a command pool for commands submitted to the graphics queue.
	// we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo command_pool_info = vkinit::command_pool_create_info(
	    _graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (auto& frame : _frames) {
		VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &frame._command_pool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo command_alloc_info =
		    vkinit::command_buffer_allocate_info(frame._command_pool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &command_alloc_info, &frame._main_command_buffer));
	}
}

void PantomirEngine::initSyncStructures() {
	// Create synchronization structures
	// one fence to control when the gpu has finished rendering the frame,
	// and 2 semaphores to synchronize rendering with swapchain
	// we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo     fence_create_info     = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));
	}
}

void PantomirEngine::createSwapchain(uint32_t width, uint32_t height) {
	vkb::SwapchainBuilder swapchain_builder { _chosen_GPU, _device, _surface };

	_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain built_swapchain =
	    swapchain_builder
	        //.use_default_format_selection()
	        .set_desired_format(VkSurfaceFormatKHR {
	            .format     = _swapchain_image_format,
	            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	        // use vsync present mode
	        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
	        .set_desired_extent(width, height)
	        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	        .build()
	        .value();

	_swapchain_extent      = built_swapchain.extent;
	// store swapchain and its related images
	_swapchain             = built_swapchain.swapchain;
	_swapchain_images      = built_swapchain.get_images().value();
	_swapchain_image_views = built_swapchain.get_image_views().value();
}

void PantomirEngine::destroySwapchain() {
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchain_image_views.size(); i++) {

		vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
	}
}

int PantomirEngine::start() {
	try {
		mainLoop();
	} catch (const std::exception& exception) {
		LOG(Engine, Error, "Exception: ", exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void PantomirEngine::draw() {
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._render_fence));

	get_current_frame()._deletion_queue.flush();

	uint32_t swapchain_image_index;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, nullptr, &swapchain_image_index));

	VkCommandBuffer command_buffer = get_current_frame()._main_command_buffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(command_buffer, 0));

	// begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo command_buffer_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	_draw_extent.width                                 = _draw_image.imageExtent.width;
	_draw_extent.height                                = _draw_image.imageExtent.height;

	VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transition_image(command_buffer, _draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	drawBackground(command_buffer);

	// transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::transition_image(command_buffer, _draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(command_buffer, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::copy_image_to_image(command_buffer, _draw_image.image, _swapchain_images[swapchain_image_index], _draw_extent, _swapchain_extent);

	// set swapchain image layout to Present so we can show it on the screen
	vkutil::transition_image(command_buffer, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(command_buffer));

	// prepare the submission to the queue.
	// we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo command_buffer_submit_info = vkinit::command_buffer_submit_info(command_buffer);

	VkSemaphoreSubmitInfo     wait_info                   = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchain_semaphore);
	VkSemaphoreSubmitInfo     signal_info                 = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._render_semaphore);

	VkSubmitInfo2             submit_info                     = vkinit::submit_info(&command_buffer_submit_info, &signal_info, &wait_info);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit_info, get_current_frame()._render_fence));

	// prepare present
	//  this will put the image we just rendered to into the visible window.
	//  we want to wait on the _renderSemaphore for that,
	//  as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext              = nullptr;
	presentInfo.pSwapchains        = &_swapchain;
	presentInfo.swapchainCount     = 1;

	presentInfo.pWaitSemaphores    = &get_current_frame()._render_semaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices      = &swapchain_image_index;

	VK_CHECK(vkQueuePresentKHR(_graphics_queue, &presentInfo));

	// increase the number of frames drawn
	_frame_number++;
}

void PantomirEngine::drawBackground(VkCommandBuffer command_buffer) {
	VkClearColorValue clear_value;
	float             flash            = std::abs(std::sin(_frame_number / 120.f));
	clear_value                         = { { 0.94901960784313725490196078431373f, 0.5921568627450980392156862745098f, flash, 1.0f } };

	VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	vkCmdClearColorImage(command_buffer, _draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
}

void PantomirEngine::mainLoop() {
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

		draw();
	}
}

int main(int argc, char* argv[]) {
	return PantomirEngine::GetInstance().start();
}
