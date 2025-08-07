#include <PantomirEngine.h>

#include <chrono>
#include <cstddef>
#include <thread>

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "LoggerMacros.h"
#include "VkDescriptors.h"
#include "VkImages.h"
#include "VkInitializers.h"
#include "VkPipelines.h"

#include "VkLoader.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include "Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "PantomirFunctionLibrary.h"
#include "VkPushConstants.h"

bool IsVisible(const RenderObject& renderObject, const glm::mat4& viewProjection) {
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

	glm::vec3       clipSpaceMin            = glm::vec3 { 1.5 };
	glm::vec3       clipSpaceMax            = glm::vec3 { -1.5 };

	// Project each corner of the bounding box into clip space.
	for (const glm::vec3& localCorner : unitCubeCorners) {
		glm::vec3 worldCorner      = renderObject.bounds.originPoint + (localCorner * renderObject.bounds.extents);
		glm::vec4 clipSpaceCorner  = objectToClipSpaceMatrix * glm::vec4(worldCorner, 1.0f);

		// Divide by w to convert to normalized clip space (from -1 to 1)
		glm::vec3 normalizedCorner = glm::vec3(clipSpaceCorner) / clipSpaceCorner.w;

		// Expand the bounding box in clip space
		clipSpaceMin               = glm::min(clipSpaceMin, normalizedCorner);
		clipSpaceMax               = glm::max(clipSpaceMax, normalizedCorner);
	}

	constexpr glm::vec3 clipBoundsMin = { -1.0f, -1.0f, 0.0f };
	constexpr glm::vec3 clipBoundsMax = { 1.0f, 1.0f, 1.0f };

	// If the box is fully outside the clip space in any direction, it's not visible
	if (clipSpaceMin.x > clipBoundsMax.x || clipSpaceMax.x < clipBoundsMin.x ||
	    clipSpaceMin.y > clipBoundsMax.y || clipSpaceMax.y < clipBoundsMin.y ||
	    clipSpaceMin.z > clipBoundsMax.z || clipSpaceMax.z < clipBoundsMin.z) {
		return false;
	}

	return true;
}

void GLTFMetallic_Roughness::BuildPipelines(PantomirEngine* engine) {
	VkShaderModule meshVertexShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/mesh.vert.spv", engine->_logicalGPU, &meshVertexShader)) {
		LOG(Engine, Error, "Error when building the triangle vertex shader module");
	}

	VkShaderModule meshFragShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/mesh.frag.spv", engine->_logicalGPU, &meshFragShader)) {
		LOG(Engine, Error, "Error when building the triangle fragment shader module");
	}

	VkPushConstantRange matrixRange {};
	matrixRange.offset     = 0;
	matrixRange.size       = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder descriptorLayoutBuilder;
	descriptorLayoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	descriptorLayoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorLayoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorLayoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorLayoutBuilder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorLayoutBuilder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	_materialLayout                           = descriptorLayoutBuilder.Build(engine->_logicalGPU, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // Every binding will use these flags

	VkDescriptorSetLayout      layouts[]      = { engine->_gpuSceneDataDescriptorLayout, _materialLayout };

	VkPipelineLayoutCreateInfo meshLayoutInfo = vkinit::PipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount             = 2;
	meshLayoutInfo.pSetLayouts                = layouts; // Decays into pointer.
	meshLayoutInfo.pPushConstantRanges        = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount     = 1;

	VK_CHECK(vkCreatePipelineLayout(engine->_logicalGPU, &meshLayoutInfo, nullptr, &_pipelineLayout));

	// Build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;

	// Render format
	pipelineBuilder.SetColorAttachmentFormat(engine->_colorImage.imageFormat);
	pipelineBuilder.SetDepthFormat(engine->_depthImage.imageFormat);

	pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// Use the triangle layout we created
	pipelineBuilder._pipelineLayout        = _pipelineLayout;
	_opaquePipeline.layout                 = _pipelineLayout;
	_transparentPipeline.layout            = _pipelineLayout;
	_maskedPipeline.layout                 = _pipelineLayout;
	_opaqueDoubleSidedPipeline.layout      = _pipelineLayout;
	_transparentDoubleSidedPipeline.layout = _pipelineLayout;
	_maskedDoubleSidedPipeline.layout      = _pipelineLayout;

	// Building Pipelines
	_opaquePipeline.pipeline               = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	_opaqueDoubleSidedPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.EnableBlendingAlphablend();
	pipelineBuilder.EnableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	_transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	_transparentDoubleSidedPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	_maskedPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	_maskedDoubleSidedPipeline.pipeline = pipelineBuilder.BuildPipeline(engine->_logicalGPU);

	vkDestroyShaderModule(engine->_logicalGPU, meshFragShader, nullptr);
	vkDestroyShaderModule(engine->_logicalGPU, meshVertexShader, nullptr);
}

void GLTFMetallic_Roughness::ClearResources(const VkDevice device) const {
	vkDestroyPipeline(device, _maskedPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _transparentPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _opaquePipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _maskedDoubleSidedPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _transparentDoubleSidedPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _opaqueDoubleSidedPipeline.pipeline, nullptr);

	vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, _materialLayout, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::WriteMaterial(VkDevice device, MaterialPass pass, VkCullModeFlagBits cullMode, const GLTFMetallic_Roughness::MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator) {
	MaterialInstance materialInstance {};

	materialInstance.passType = pass;
	materialInstance.cullMode = cullMode;

	// Per material, we choose a pipeline based on its properties.
	if (pass == MaterialPass::AlphaBlend) {
		if (cullMode == VK_CULL_MODE_NONE) {
			materialInstance.pipeline = &_transparentDoubleSidedPipeline;
		} else {
			materialInstance.pipeline = &_transparentPipeline;
		}
	} else if (pass == MaterialPass::AlphaMask) {
		if (cullMode == VK_CULL_MODE_NONE) {
			materialInstance.pipeline = &_maskedDoubleSidedPipeline;
		} else {
			materialInstance.pipeline = &_maskedPipeline;
		}
	} else if (pass == MaterialPass::Opaque) {
		if (cullMode == VK_CULL_MODE_NONE) {
			materialInstance.pipeline = &_opaqueDoubleSidedPipeline;
		} else {
			materialInstance.pipeline = &_opaquePipeline;
		}
	} else {
		materialInstance.pipeline = &_opaquePipeline;
	}

	materialInstance.descriptorSet = descriptorAllocator.Allocate(device, _materialLayout);

	_writer.Clear();
	_writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	_writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	_writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	_writer.WriteImage(3, resources.emissiveImage.imageView, resources.emissiveSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	_writer.WriteImage(4, resources.normalImage.imageView, resources.normalSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	_writer.WriteImage(5, resources.specularImage.imageView, resources.specularSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	_writer.UpdateSet(device, materialInstance.descriptorSet);

	return materialInstance;
}

void PantomirEngine::WriteHDRIDescriptorSet() {
	_singleImageDescriptorSet = globalDescriptorAllocator.Allocate(_logicalGPU, _singleImageDescriptorLayout);
	DescriptorWriter singleImageDescriptorWriter;
	singleImageDescriptorWriter.WriteImage(0, _loadedHDRIs["brown_photostudio_02_4k"]->_allocatedImage.imageView, _loadedHDRIs["brown_photostudio_02_4k"]->_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	singleImageDescriptorWriter.UpdateSet(_logicalGPU, _singleImageDescriptorSet);
}

void MeshNode::FillDrawContext(const glm::mat4& topMatrix, DrawContext& drawContext) {
	const glm::mat4 nodeMatrix = topMatrix * _worldTransform;

	for (const GeoSurface& geoSurface : _mesh->surfaces) {
		RenderObject renderObject;
		renderObject.indexCount          = geoSurface.count;
		renderObject.firstIndex          = geoSurface.startIndex;
		renderObject.indexBuffer         = _mesh->meshBuffers.indexBuffer.buffer;
		renderObject.material            = &geoSurface.material->data;
		renderObject.bounds              = geoSurface.bounds;
		renderObject.transform           = nodeMatrix;
		renderObject.vertexBufferAddress = _mesh->meshBuffers.vertexBufferAddress;

		if (geoSurface.material->data.passType == MaterialPass::AlphaBlend) {
			drawContext.transparentSurfaces.push_back(renderObject);
		} else if (geoSurface.material->data.passType == MaterialPass::AlphaMask) {
			drawContext.maskedSurfaces.push_back(renderObject);
		} else if (geoSurface.material->data.passType == MaterialPass::Opaque) {
			drawContext.opaqueSurfaces.push_back(renderObject);
		} else {
			drawContext.opaqueSurfaces.push_back(renderObject);
		}
	}

	Node::FillDrawContext(topMatrix, drawContext);
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

void PantomirEngine::MainLoop() {
	bool bQuit = false;

	while (!bQuit) {
		const auto start = std::chrono::steady_clock::now();

		SDL_Event  event;

		// Handle events on queue
		while (SDL_PollEvent(&event) != 0) {
			// Close the window when user alt-f4s or clicks the X button
			if (event.type == SDL_EVENT_QUIT) {
				bQuit = true;
			}

			_mainCamera.ProcessSDLEvent(event, _window);

			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				_resizeRequested = true;
			}
			if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
				_stopRendering = true;
			}
			if (event.type == SDL_EVENT_WINDOW_RESTORED) {
				_stopRendering = false;
			}

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		// Update camera movement based on current keyboard state
		_mainCamera.UpdateMovement();

		// Do not draw if we are minimized
		if (_stopRendering) {
			// Throttle loop speed
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (_resizeRequested) {
			ResizeSwapchain();
			// TODO: The images and views must also be replaced and updated for bindings.
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("background")) {

			ComputeEffect& selected = _backgroundEffects[_currentBackgroundEffect];

			ImGui::SliderFloat("Render Scale", &_renderScale, 0.3f, 1.f);

			ImGui::Text("Selected effect: ", selected.name);

			ImGui::SliderInt("Effect Index", &_currentBackgroundEffect, 0, _backgroundEffects.size() - 1);

			ImGui::InputFloat4("data1", reinterpret_cast<float*>(&selected.pushConstants.data1));
			ImGui::InputFloat4("data2", reinterpret_cast<float*>(&selected.pushConstants.data2));
			ImGui::InputFloat4("data3", reinterpret_cast<float*>(&selected.pushConstants.data3));
			ImGui::InputFloat4("data4", reinterpret_cast<float*>(&selected.pushConstants.data4));
		}

		ImGui::End();

		ImGui::Begin("Stats");
		ImGui::Text("frametime %f ms", _stats.frameTime);
		ImGui::Text("draw time %f ms", _stats.meshDrawTime);
		ImGui::Text("update time %f ms", _stats.sceneUpdateTime);
		ImGui::Text("triangles %i", _stats.triangleCount);
		ImGui::Text("draws %i", _stats.drawcallCount);
		ImGui::End();

		ImGui::Render();

		PantomirEngine::Draw();

		const std::chrono::time_point      end     = std::chrono::steady_clock::now();
		const std::chrono::duration<float> elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		_stats.frameTime                           = elapsed.count() / 1000.f;
		_deltaTime                                 = std::clamp(elapsed.count(), 0.0001f, 0.016f);
	}
}

void PantomirEngine::ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& anonymousFunction) const {
	VK_CHECK(vkResetFences(_logicalGPU, 1, &_immediateFence));
	VK_CHECK(vkResetCommandBuffer(_immediateCommandBuffer, 0));

	constexpr VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(_immediateCommandBuffer, &commandBufferBeginInfo));

	anonymousFunction(_immediateCommandBuffer);

	VK_CHECK(vkEndCommandBuffer(_immediateCommandBuffer));

	VkCommandBufferSubmitInfo commandInfo = vkinit::CommandBufferSubmitInfo(_immediateCommandBuffer);
	const VkSubmitInfo2       submit      = vkinit::SubmitInfo(&commandInfo, nullptr, nullptr);

	// Submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateFence));
	VK_CHECK(vkWaitForFences(_logicalGPU, 1, &_immediateFence, true, 9999999999));
}

GPUMeshBuffers PantomirEngine::UploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices) const {
	const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t   indexBufferSize  = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface {};

	// Create vertex buffer
	newSurface.vertexBuffer = CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	// Find the address of the vertex buffer on the GPU
	const VkBufferDeviceAddressInfo deviceAddressInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress      = vkGetBufferDeviceAddress(_logicalGPU, &deviceAddressInfo);

	// Create index buffer
	newSurface.indexBuffer              = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	const AllocatedBuffer stagingBuffer = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void*                 data          = stagingBuffer.allocation->GetMappedData();

	// Copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// Copy index buffer
	memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

	ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
		VkBufferCopy vertexCopy { 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size      = vertexBufferSize;

		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy { 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size      = indexBufferSize;

		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	DestroyBuffer(stagingBuffer);

	return newSurface;
}

AllocatedImage PantomirEngine::CreateHDRIImage(void* dataSource, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const {
	const size_t          dataSize        = static_cast<size_t>(size.width * size.height * size.depth) * PantomirFunctionLibrary::BytesPerPixelFromFormat(format);
	const AllocatedBuffer uploadBuffer    = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // Transfer src bit means it's usable by GPU for copying.

	void*                 dataDestination = uploadBuffer.info.pMappedData;
	memcpy(dataDestination, dataSource, dataSize);

	AllocatedImage newImage {};
	newImage.imageFormat              = format;
	newImage.imageExtent              = size;

	VkImageCreateInfo imageCreateInfo = vkinit::ImageCreateInfo(format, usage, size);
	if (mipmapped) {
		imageCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}
	// imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	// imageCreateInfo.arrayLayers = 6;

	// TODO: Implement the 6 layers properly

	// Always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
	vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocationCreateInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image
	VK_CHECK(vmaCreateImage(_vmaAllocator, &imageCreateInfo, &vmaAllocationCreateInfo, &newImage.image, &newImage.allocation, nullptr));

	// If the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// Build an image-view for the image
	VkImageViewCreateInfo viewInfo       = vkinit::ImageViewCreateInfo(format, newImage.image, aspectFlag);
	viewInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
	// viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	// viewInfo.subresourceRange.layerCount = 6;

	VK_CHECK(vkCreateImageView(_logicalGPU, &viewInfo, nullptr, &newImage.imageView));

	ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
		vkutil::TransitionImage(commandBuffer, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion               = {};
		copyRegion.bufferOffset                    = 0;
		copyRegion.bufferRowLength                 = 0;
		copyRegion.bufferImageHeight               = 0;

		copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel       = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount     = 1;
		copyRegion.imageExtent                     = size;

		// Copy the buffer into the image
		vkCmdCopyBufferToImage(commandBuffer, uploadBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		if (mipmapped) {
			vkutil::GenerateMipmaps(commandBuffer, newImage.image, VkExtent2D { newImage.imageExtent.width, newImage.imageExtent.height });
		} else {
			vkutil::TransitionImage(commandBuffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	});

	DestroyBuffer(uploadBuffer);

	return newImage;
}

glm::mat4 PantomirEngine::GetProjectionMatrix() const {
	glm::mat4 projection = glm::perspective(glm::radians(70.f), static_cast<float>(_windowExtent.width) / static_cast<float>(_windowExtent.height), 10000.f, 0.1f);
	projection[1][1] *= -1;
	// Invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
	return projection;
}

AllocatedImage PantomirEngine::CreateImage(void* dataSource, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const {
	const size_t          dataSize        = size.width * size.height * size.depth * PantomirFunctionLibrary::BytesPerPixelFromFormat(format);
	// Vulkan doesn't allow us to send pixel data straight to an image, it has to be sent to a buffer first.
	const AllocatedBuffer uploadBuffer    = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); // Transfer src bit means it's usable by GPU for copying.

	void*                 dataDestination = uploadBuffer.info.pMappedData;
	memcpy(dataDestination, dataSource, dataSize);

	AllocatedImage newImage = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
		vkutil::TransitionImage(commandBuffer, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion               = {};
		copyRegion.bufferOffset                    = 0;
		copyRegion.bufferRowLength                 = 0;
		copyRegion.bufferImageHeight               = 0;

		copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel       = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount     = 1;
		copyRegion.imageExtent                     = size;

		// Copy the buffer into the image
		vkCmdCopyBufferToImage(commandBuffer, uploadBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		if (mipmapped) {
			vkutil::GenerateMipmaps(commandBuffer, newImage.image, VkExtent2D { newImage.imageExtent.width, newImage.imageExtent.height });
		} else {
			vkutil::TransitionImage(commandBuffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	});

	DestroyBuffer(uploadBuffer);

	return newImage;
}

void PantomirEngine::DestroyImage(const AllocatedImage& img) const {
	vkDestroyImageView(_logicalGPU, img.imageView, nullptr);
	vmaDestroyImage(_vmaAllocator, img.image, img.allocation);
}

AllocatedBuffer PantomirEngine::CreateBuffer(const size_t allocSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) const {
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext              = nullptr;
	bufferInfo.size               = allocSize;
	bufferInfo.usage              = usage;

	VmaAllocationCreateInfo vmaAllocInfo {};
	vmaAllocInfo.usage = memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	VK_CHECK(vmaCreateBuffer(_vmaAllocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

void PantomirEngine::DestroyBuffer(const AllocatedBuffer& buffer) const {
	vmaDestroyBuffer(_vmaAllocator, buffer.buffer, buffer.allocation);
}

PantomirEngine::PantomirEngine() {
	InitSDLWindow();
	InitVulkan();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
	InitDescriptors();
	InitPipelines();
	InitImgui();
	InitDefaultData();
}

PantomirEngine::~PantomirEngine() {
	// Make sure the GPU has stopped doing work
	vkDeviceWaitIdle(_logicalGPU);

	_loadedScenes.clear();
	_loadedHDRIs.clear();

	for (auto& frame : _frames) {
		frame.deletionQueue.Flush();
	}

	_shutdownDeletionQueue.Flush();
	DestroySwapchain();

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_logicalGPU, nullptr);

	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
	vkDestroyInstance(_instance, nullptr);
	SDL_DestroyWindow(_window);
}

void PantomirEngine::InitSDLWindow() {
	constexpr SDL_InitFlags   initFlags   = SDL_INIT_VIDEO;
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

	SDL_Init(initFlags);

	const SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
	SDL_Rect            displayBounds {};
	if (!SDL_GetDisplayBounds(displayID, &displayBounds)) {
		LOG(Engine, Error, "Failed to get display bounds: {}", SDL_GetError());
		displayBounds = { 0, 0, static_cast<int>(_windowExtent.width), static_cast<int>(_windowExtent.height) }; // Fallback
	}

	int width     = displayBounds.w * _windowRatio;
	int height    = displayBounds.h * _windowRatio;
	_windowExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	_window       = SDL_CreateWindow(R"(Pantomir Engine)", width, height, windowFlags);
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
	                                                       .request_validation_layers(_bUseValidationLayers) // For debugging
	                                                       .use_default_debug_messenger()                    // For debugging
	                                                       .require_api_version(1, 3, 0)                     // Using Vulkan 1.3
	                                                       .enable_extensions(extensions)                    // Goes through available extensions on the physical device. If all the extensions chosen are available, then return true;
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
	                                                 .add_required_extension(VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME)
	                                                 .set_surface(_surface)
	                                                 .select()
	                                                 .value();

	vkb::DeviceBuilder logicalDeviceBuilder { selectedPhysicalDevice };
	vkb::Device        builtLogicalDevice = logicalDeviceBuilder.build().value();

	_logicalGPU                           = builtLogicalDevice.device;
	_physicalGPU                          = selectedPhysicalDevice.physical_device;

	_graphicsQueue                        = builtLogicalDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamilyIndex             = builtLogicalDevice.get_queue_index(vkb::QueueType::graphics).value(); // ID Tag for GPU logical units; Useful for command pools, buffers, images.

	VmaAllocatorCreateInfo allocatorInfo  = {};
	allocatorInfo.physicalDevice          = _physicalGPU;
	allocatorInfo.device                  = _logicalGPU;
	allocatorInfo.instance                = _instance;
	allocatorInfo.flags                   = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	vmaCreateAllocator(&allocatorInfo, &_vmaAllocator);

	_shutdownDeletionQueue.PushFunction([&]() {
		vmaDestroyAllocator(_vmaAllocator);
	});
}

void PantomirEngine::InitSwapchain() {
	CreateSwapchain(_windowExtent.width, _windowExtent.height);

	VkExtent3D drawImageExtent = { _windowExtent.width, _windowExtent.height, 1 };

	_colorImage.imageFormat    = VK_FORMAT_R16G16B16A16_SFLOAT;
	_colorImage.imageExtent    = drawImageExtent;

	VkImageUsageFlags drawImageUsages {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo       drawImageInfo = vkinit::ImageCreateInfo(_colorImage.imageFormat, drawImageUsages, drawImageExtent);

	// For the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo drawImageAllocInfo {};
	drawImageAllocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	drawImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image
	vmaCreateImage(_vmaAllocator, &drawImageInfo, &drawImageAllocInfo, &_colorImage.image, &_colorImage.allocation, nullptr);

	// For the draw image to use for rendering
	VkImageViewCreateInfo drawViewInfo = vkinit::ImageViewCreateInfo(_colorImage.imageFormat, _colorImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_logicalGPU, &drawViewInfo, nullptr, &_colorImage.imageView));

	_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	_depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages {};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo       depthImageInfo = vkinit::ImageCreateInfo(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	VmaAllocationCreateInfo depthImageAllocInfo {};
	depthImageAllocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	depthImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vmaCreateImage(_vmaAllocator, &depthImageInfo, &depthImageAllocInfo, &_depthImage.image, &_depthImage.allocation, nullptr);

	VkImageViewCreateInfo depthViewInfo = vkinit::ImageViewCreateInfo(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_logicalGPU, &depthViewInfo, nullptr, &_depthImage.imageView));

	_shutdownDeletionQueue.PushFunction([=, this]() {
		DestroyImage(_colorImage);
		DestroyImage(_depthImage);
	});
}

void PantomirEngine::InitCommands() {
	// Allow for resetting of individual command buffers, instead of only resetting the entire pool at once.
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(_graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	/* ImGUI */
	VK_CHECK(vkCreateCommandPool(_logicalGPU, &commandPoolInfo, nullptr, &_immediateCommandPool));
	VkCommandBufferAllocateInfo commandBufferAllocInfoImmediate = vkinit::CommandBufferAllocateInfo(_immediateCommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_logicalGPU, &commandBufferAllocInfoImmediate, &_immediateCommandBuffer)); /* Initial State */
	_shutdownDeletionQueue.PushFunction([=, this]() {
		vkDestroyCommandPool(_logicalGPU, _immediateCommandPool, nullptr);
	});

	/* Render Frames */
	for (FrameData& frame : _frames) {
		VK_CHECK(vkCreateCommandPool(_logicalGPU, &commandPoolInfo, nullptr, &frame.commandPool));
		VkCommandBufferAllocateInfo commandBufferAllocInfo = vkinit::CommandBufferAllocateInfo(frame.commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_logicalGPU, &commandBufferAllocInfo, &frame.mainCommandBuffer)); /* Initial State */
		_shutdownDeletionQueue.PushFunction([=, this]() {
			vkDestroyCommandPool(_logicalGPU, frame.commandPool, nullptr);
		});
	}
}

void PantomirEngine::InitSyncStructures() {
	// Create synchronization structures one fence to control when the gpu has finished rendering the frame, and 2 semaphores to synchronize rendering with swapchain.
	// We want the fence to start signaled so we can wait on it on the first frame
	VkFenceCreateInfo     fenceCreateInfo     = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

	for (FrameData& frame : _frames) {
		VK_CHECK(vkCreateFence(_logicalGPU, &fenceCreateInfo, nullptr, &frame.renderFence));
		VK_CHECK(vkCreateSemaphore(_logicalGPU, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_logicalGPU, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));
		_shutdownDeletionQueue.PushFunction([=]() {
			vkDestroyFence(_logicalGPU, frame.renderFence, nullptr);
			vkDestroySemaphore(_logicalGPU, frame.swapchainSemaphore, nullptr);
			vkDestroySemaphore(_logicalGPU, frame.renderSemaphore, nullptr);
		});
	}

	VK_CHECK(vkCreateFence(_logicalGPU, &fenceCreateInfo, nullptr, &_immediateFence));
	_shutdownDeletionQueue.PushFunction([=]() { vkDestroyFence(_logicalGPU, _immediateFence, nullptr); });
}

void PantomirEngine::InitDescriptors() {
	// Create a descriptor pool that will hold 10 sets
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
	};

	globalDescriptorAllocator.Init(_logicalGPU, 10, sizes);

	// Make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(_logicalGPU, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// Make the descriptor set layout for our scene draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_gpuSceneDataDescriptorLayout = builder.Build(_logicalGPU, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	// Make the descriptor set layout for textures
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_singleImageDescriptorLayout = builder.Build(_logicalGPU, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	// TODO: Since we don't have a sampler, why is this here? Other descriptor sets are made later.
	_drawImageDescriptorSet = globalDescriptorAllocator.Allocate(_logicalGPU, _drawImageDescriptorLayout);
	DescriptorWriter drawImageDescriptorWriter;
	drawImageDescriptorWriter.WriteImage(0, _colorImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	drawImageDescriptorWriter.UpdateSet(_logicalGPU, _drawImageDescriptorSet);

	_shutdownDeletionQueue.PushFunction([&]() {
		globalDescriptorAllocator.DestroyPools(_logicalGPU);

		vkDestroyDescriptorSetLayout(_logicalGPU, _drawImageDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(_logicalGPU, _gpuSceneDataDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(_logicalGPU, _singleImageDescriptorLayout, nullptr);
	});

	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		_frames[i].frameDescriptors = DescriptorAllocatorGrowable {};
		_frames[i].frameDescriptors.Init(_logicalGPU, 1000, frameSizes);

		_shutdownDeletionQueue.PushFunction([&, i]() {
			_frames[i].frameDescriptors.DestroyPools(_logicalGPU);
		});
	}
}

void PantomirEngine::InitPipelines() {
	InitBackgroundPipelines();
	_metalRoughMaterial.BuildPipelines(this);
	_shutdownDeletionQueue.PushFunction([&]() {
		_metalRoughMaterial.ClearResources(_logicalGPU);
	});
	InitHDRIPipeline();
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

	VK_CHECK(vkCreatePipelineLayout(_logicalGPU, &computeLayout, nullptr, &_gradientPipelineLayout));

	VkShaderModule gradientShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/gradient_color.comp.spv", _logicalGPU, &gradientShader)) {
		LOG(Engine_Renderer, Error, "Error when building the compute shader.");
	}

	VkShaderModule skyShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/sky.comp.spv", _logicalGPU, &skyShader)) {
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

	ComputeEffect backgroundGradient {};
	backgroundGradient.layout              = _gradientPipelineLayout;
	backgroundGradient.name                = "gradient";
	backgroundGradient.pushConstants       = {};

	// Default colors
	backgroundGradient.pushConstants.data1 = glm::vec4 { 0.8588f, 1.0f, 0.9725f, 1.0f };
	backgroundGradient.pushConstants.data2 = glm::vec4 { 1.0f, 0.8510f, 0.9333f, 0.0f };

	VK_CHECK(vkCreateComputePipelines(_logicalGPU, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &backgroundGradient.pipeline));

	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect backgroundSky {};
	backgroundSky.layout                       = _gradientPipelineLayout;
	backgroundSky.name                         = "sky";
	backgroundSky.pushConstants                = {};
	backgroundSky.pushConstants.data1          = glm::vec4(0.1, 0.2, 0.4, 0.97);
	backgroundSky.pushConstants.cameraPosition = _mainCamera._position;
	backgroundSky.pushConstants.cameraRotation = _mainCamera.GetRotationMatrix();

	VK_CHECK(vkCreateComputePipelines(_logicalGPU, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &backgroundSky.pipeline));

	// Add the 2 background effects into the array
	_backgroundEffects.push_back(backgroundGradient);
	_backgroundEffects.push_back(backgroundSky);

	// Destroy structures properly
	vkDestroyShaderModule(_logicalGPU, gradientShader, nullptr);
	vkDestroyShaderModule(_logicalGPU, skyShader, nullptr);

	_shutdownDeletionQueue.PushFunction([=]() {
		vkDestroyPipelineLayout(_logicalGPU, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(_logicalGPU, backgroundSky.pipeline, nullptr);
		vkDestroyPipeline(_logicalGPU, backgroundGradient.pipeline, nullptr);
	});
}

void PantomirEngine::InitImgui() {
	// 1: Create descriptor pool for IMGUI the size of the pool is very oversized, but it's copied from imgui demo itself.
	VkDescriptorPoolSize       poolSizes[]    = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
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

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets                    = 1000;
	poolCreateInfo.poolSizeCount              = static_cast<uint32_t>(std::size(poolSizes));
	poolCreateInfo.pPoolSizes                 = poolSizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_logicalGPU, &poolCreateInfo, nullptr, &imguiPool));

	// 2: Initialize imgui library
	// This initializes the core structures of imgui
	ImGui::CreateContext();

	// This initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(_window);

	// This initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info                           = {};
	init_info.Instance                                            = _instance;
	init_info.PhysicalDevice                                      = _physicalGPU;
	init_info.Device                                              = _logicalGPU;
	init_info.Queue                                               = _graphicsQueue;
	init_info.DescriptorPool                                      = imguiPool;
	init_info.MinImageCount                                       = 3;
	init_info.ImageCount                                          = 3;
	init_info.UseDynamicRendering                                 = true;
	// Dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo                         = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;
	init_info.MSAASamples                                         = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

	// Add the destroy imgui created structures
	_shutdownDeletionQueue.PushFunction([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_logicalGPU, imguiPool, nullptr);
	});
}

void PantomirEngine::InitHDRIPipeline() {
	VkShaderModule HDRIVertexShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/HDRI.vert.spv", _logicalGPU, &HDRIVertexShader)) {
		LOG(Engine, Error, "Error when building the triangle vertex shader module");
	} else {
		LOG(Engine, Info, "HDRI vertex shader successfully loaded");
	}

	VkShaderModule HDRIFragShader;
	if (!vkutil::LoadShaderModule("Assets/Shaders/HDRI.frag.spv", _logicalGPU, &HDRIFragShader)) {
		LOG(Engine, Error, "Error when building the triangle fragment shader module");
	} else {
		LOG(Engine, Info, "HDRI fragment shader successfully loaded");
	}

	VkPushConstantRange bufferRange {};
	bufferRange.offset                            = 0;
	bufferRange.size                              = sizeof(HDRIPushConstants);
	bufferRange.stageFlags                        = VK_SHADER_STAGE_VERTEX_BIT; // TODO: Maybe not just the vertex? Maybe the frag too?

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::PipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges        = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount     = 1;
	pipelineLayoutInfo.pSetLayouts                = &_singleImageDescriptorLayout;
	pipelineLayoutInfo.setLayoutCount             = 1;
	VK_CHECK(vkCreatePipelineLayout(_logicalGPU, &pipelineLayoutInfo, nullptr, &_hdriPipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder._pipelineLayout = _hdriPipelineLayout;
	pipelineBuilder.SetShaders(HDRIVertexShader, HDRIFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	pipelineBuilder.SetColorAttachmentFormat(_colorImage.imageFormat);
	pipelineBuilder.SetDepthFormat(_depthImage.imageFormat);
	_hdriPipeline = pipelineBuilder.BuildPipeline(_logicalGPU);

	// Clean structures
	vkDestroyShaderModule(_logicalGPU, HDRIFragShader, nullptr);
	vkDestroyShaderModule(_logicalGPU, HDRIVertexShader, nullptr);

	_shutdownDeletionQueue.PushFunction([&]() {
		vkDestroyPipelineLayout(_logicalGPU, _hdriPipelineLayout, nullptr);
		vkDestroyPipeline(_logicalGPU, _hdriPipeline, nullptr);
	});
}

void PantomirEngine::InitDefaultData() {
	_mainCamera._velocity                 = glm::vec3(0.f);
	_mainCamera._position                 = glm::vec3(0.f, 1.5f, 1.5f);
	_mainCamera._pitch                    = 0;
	_mainCamera._yaw                      = 0;

	// 3 default textures, white, grey, black. 1 pixel each
	uint32_t white                        = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage                           = CreateImage((void*)&white, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	uint32_t grey                         = glm::packUnorm4x8(glm::vec4(1.0f, 0.5f, 0.0f, 1));
	_greyImage                            = CreateImage((void*)&grey, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	uint32_t black                        = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage                           = CreateImage((void*)&black, VkExtent3D { 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	// Checkerboard image
	uint32_t                      magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16> pixels {}; // for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	_errorCheckerboardImage = CreateImage(pixels.data(), VkExtent3D { 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo samplerCreateInfo { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(_logicalGPU, &samplerCreateInfo, nullptr, &_defaultSamplerNearest);
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(_logicalGPU, &samplerCreateInfo, nullptr, &_defaultSamplerLinear);

	_shutdownDeletionQueue.PushFunction([this, device = _logicalGPU, defaultSamplerNearest = _defaultSamplerNearest, defaultSamplerLinear = _defaultSamplerLinear, whiteImage = _whiteImage, greyImage = _greyImage, blackImage = _blackImage, errorCheckerboardImage = _errorCheckerboardImage]() {
		vkDestroySampler(device, defaultSamplerNearest, nullptr);
		vkDestroySampler(device, defaultSamplerLinear, nullptr);

		DestroyImage(whiteImage);
		DestroyImage(greyImage);
		DestroyImage(blackImage);
		DestroyImage(errorCheckerboardImage);
	});

	GLTFMetallic_Roughness::MaterialResources materialResources {};
	// Default the material textures
	materialResources.colorImage                                 = _whiteImage;
	materialResources.colorSampler                               = _defaultSamplerLinear;
	materialResources.metalRoughImage                            = _greyImage;
	materialResources.metalRoughSampler                          = _defaultSamplerLinear;
	materialResources.emissiveImage                              = _whiteImage;
	materialResources.emissiveSampler                            = _defaultSamplerLinear;
	materialResources.normalImage                                = _whiteImage;
	materialResources.normalSampler                              = _defaultSamplerLinear;
	materialResources.specularImage                              = _whiteImage;
	materialResources.specularSampler                            = _defaultSamplerLinear;

	// Set the uniform buffer for the material data
	AllocatedBuffer                            materialConstants = CreateBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Write the buffer
	GLTFMetallic_Roughness::MaterialConstants* sceneUniformData  = static_cast<GLTFMetallic_Roughness::MaterialConstants*>(materialConstants.allocation->GetMappedData());
	sceneUniformData->colorFactors                               = glm::vec4 { 1, 1, 1, 1 };
	sceneUniformData->metalRoughFactors                          = glm::vec4 { 1, 1, 1, 0 };
	sceneUniformData->emissiveFactors                            = glm::vec3 { 0, 0, 0 };
	sceneUniformData->specularFactor                             = 0.F;

	materialResources.dataBuffer                                 = materialConstants.buffer;
	materialResources.dataBufferOffset                           = 0;

	_defaultData                                                 = _metalRoughMaterial.WriteMaterial(_logicalGPU, MaterialPass::Opaque, VK_CULL_MODE_NONE, materialResources, globalDescriptorAllocator);

	std::string                                modelPath         = { "Assets/Models/Echidna1.glb" };
	std::optional<std::shared_ptr<LoadedGLTF>> modelFile         = LoadGltf(this, modelPath);
	std::string                                hdriPath          = { "Assets/Textures/brown_photostudio_02_4k.hdr" };
	std::optional<std::shared_ptr<LoadedHDRI>> hdriFile          = LoadHDRI(this, hdriPath);

	assert(modelFile.has_value());
	assert(hdriFile.has_value());

	_loadedScenes["Echidna1"]                      = *modelFile;
	_loadedHDRIs["brown_photostudio_02_4k"] = *hdriFile;

	WriteHDRIDescriptorSet();

	_shutdownDeletionQueue.PushFunction([=, this]() {
		DestroyBuffer(materialConstants);
	});
}

void PantomirEngine::CreateSwapchain(const uint32_t width, const uint32_t height) {
	vkb::SwapchainBuilder swapchainBuilder { _physicalGPU, _logicalGPU, _surface };

	_swapchainImageFormat         = VK_FORMAT_B8G8R8A8_UNORM; // HDR is VK_FORMAT_A2B10G10R10_UNORM_PACK32

	vkb::Swapchain builtSwapchain = swapchainBuilder
	                                    .set_desired_format(VkSurfaceFormatKHR {
	                                        .format     = _swapchainImageFormat,
	                                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	                                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // V-Sync On
	                                    .set_desired_extent(width, height)
	                                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                                    .build()
	                                    .value();

	_swapchainExtent     = builtSwapchain.extent;
	_swapchain           = builtSwapchain.swapchain;
	_swapchainImages     = builtSwapchain.get_images().value();
	_swapchainImageViews = builtSwapchain.get_image_views().value();
}

void PantomirEngine::DestroySwapchain() const {
	vkDestroySwapchainKHR(_logicalGPU, _swapchain, nullptr);

	for (const VkImageView& _swapchainImageView : _swapchainImageViews) {
		vkDestroyImageView(_logicalGPU, _swapchainImageView, nullptr);
	}
}

void PantomirEngine::ResizeSwapchain() {
	vkDeviceWaitIdle(_logicalGPU);
	DestroySwapchain();
	int windowWidth;
	int windowHeight;
	SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
	_windowExtent.width  = windowWidth;
	_windowExtent.height = windowHeight;
	CreateSwapchain(windowWidth, windowHeight);
	_resizeRequested = false;
}

AllocatedImage PantomirEngine::CreateImage(const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const {
	AllocatedImage newImage {};
	newImage.imageFormat              = format;
	newImage.imageExtent              = size;

	VkImageCreateInfo imageCreateInfo = vkinit::ImageCreateInfo(format, usage, size);
	if (mipmapped) {
		imageCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// Always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
	vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocationCreateInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image
	VK_CHECK(vmaCreateImage(_vmaAllocator, &imageCreateInfo, &vmaAllocationCreateInfo, &newImage.image, &newImage.allocation, nullptr));

	// If the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// Build an image-view for the image
	VkImageViewCreateInfo viewInfo       = vkinit::ImageViewCreateInfo(format, newImage.image, aspectFlag);
	viewInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;

	VK_CHECK(vkCreateImageView(_logicalGPU, &viewInfo, nullptr, &newImage.imageView));

	return newImage;
}

void PantomirEngine::Draw() {
	UpdateScene();

	// CPU-GPU synchronization: wait until the gpu has finished rendering the last frame. Timeout of 1 second
	VK_CHECK(vkWaitForFences(_logicalGPU, 1, &GetCurrentFrame().renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_logicalGPU, 1, &GetCurrentFrame().renderFence));

	GetCurrentFrame().deletionQueue.Flush();
	GetCurrentFrame().frameDescriptors.ClearPools(_logicalGPU);

	uint32_t       swapchainImageIndex;
	const VkResult acquireNextImageKhr = vkAcquireNextImageKHR(_logicalGPU, _swapchain, 1000000000, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (acquireNextImageKhr == VK_ERROR_OUT_OF_DATE_KHR) {
		_resizeRequested = true;
		return;
	}

	const VkCommandBuffer& commandBuffer = GetCurrentFrame().mainCommandBuffer;

	/* Initial State */
	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

	// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	_drawExtent.height                              = _renderScale * std::min(_swapchainExtent.height, _colorImage.imageExtent.height);
	_drawExtent.width                               = _renderScale * std::min(_swapchainExtent.width, _colorImage.imageExtent.width);

	/* Recording State */
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	// Transition our main draw image into general layout so we can write into it.
	// We will overwrite it all so we don't care about what was the older layout.
	vkutil::TransitionImage(commandBuffer, _colorImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(commandBuffer);

	vkutil::TransitionImage(commandBuffer, _colorImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkutil::TransitionImage(commandBuffer, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	DrawHDRI(commandBuffer);
	DrawGeometry(commandBuffer);

	// Transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::TransitionImage(commandBuffer, _colorImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); // Make the swapchain optimal for transfer

	// Execute a copy from the draw image into the swapchain
	vkutil::CopyImageToImage(commandBuffer, _colorImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);
	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); // Set the swapchain layout back, now presenting.

	DrawImgui(commandBuffer, _swapchainImageViews[swapchainImageIndex]);

	// Set Swapchain Image Layout to Present so we can show it on the screen
	vkutil::TransitionImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	/* Executable State */
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	// Prepare the submission to the queue.
	// We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// We will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo commandBufferSubmitInfo = vkinit::CommandBufferSubmitInfo(commandBuffer);
	VkSemaphoreSubmitInfo     signalInfo              = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore);
	VkSemaphoreSubmitInfo     waitInfo                = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
	VkSubmitInfo2             submitInfo              = vkinit::SubmitInfo(&commandBufferSubmitInfo, &signalInfo, &waitInfo);

	// Submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution.
	/* Pending State */
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, GetCurrentFrame().renderFence));

	// Prepare present
	//  this will put the image we just rendered to into the visible window.
	//  We want to wait on the _renderSemaphore for that,
	//  as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext              = nullptr;
	presentInfo.pSwapchains        = &_swapchain;
	presentInfo.swapchainCount     = 1;

	presentInfo.pWaitSemaphores    = &GetCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices      = &swapchainImageIndex;

	VkResult presentResult         = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		_resizeRequested = true;
	}

	// Increase the number of frames drawn
	++_frameNumber;
}

void PantomirEngine::DrawBackground(VkCommandBuffer commandBuffer) {
	ComputeEffect& effect               = _backgroundEffects[_currentBackgroundEffect];

	// Update dynamic push constants (camera position & rotation)
	effect.pushConstants.cameraPosition = _mainCamera._position;
	effect.pushConstants.cameraRotation = _mainCamera.GetRotationMatrix();

	// Bind the background compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// Bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptorSet, 0, nullptr);

	// Update push constants (using camera transform)
	vkCmdPushConstants(commandBuffer, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.pushConstants);

	constexpr uint32_t WORKGROUP_SIZE_X = 16;
	constexpr uint32_t WORKGROUP_SIZE_Y = 16;

	const uint32_t     dispatchX        = (_drawExtent.width + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
	const uint32_t     dispatchY        = (_drawExtent.height + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

	vkCmdDispatch(commandBuffer, dispatchX, dispatchY, 1);
}

void PantomirEngine::DrawHDRI(const VkCommandBuffer commandBuffer) const {
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(_colorImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	const VkRenderingInfo     renderInfo      = vkinit::RenderingInfo(_drawExtent, &colorAttachment, nullptr);
	glm::mat4                 viewMatrix      = _mainCamera.GetViewMatrix();
	viewMatrix[3]                             = glm::vec4(0.0, 0.0, 0.0, 1.0);

	HDRIPushConstants constants {
		.viewMatrix       = viewMatrix,
		.projectionMatrix = GetProjectionMatrix()
	};

	vkCmdBeginRendering(commandBuffer, &renderInfo);
	VkViewport viewport = {};
	viewport.x          = 0;
	viewport.y          = 0;
	viewport.width      = static_cast<float>(_windowExtent.width);
	viewport.height     = static_cast<float>(_windowExtent.height);
	viewport.minDepth   = 0.f;
	viewport.maxDepth   = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor      = {};
	scissor.offset.x      = 0;
	scissor.offset.y      = 0;
	scissor.extent.width  = _windowExtent.width;
	scissor.extent.height = _windowExtent.height;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _hdriPipelineLayout, 0, 1, &_singleImageDescriptorSet, 0, nullptr);

	// TODO: Probably want to reuse the same GPUSceneBuffer data on the GPU, but just hacking this for now with push constants.
	vkCmdPushConstants(commandBuffer, _hdriPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(HDRIPushConstants), &constants);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRendering(commandBuffer);
}

void PantomirEngine::DrawGeometry(VkCommandBuffer commandBuffer) {
	// Visibility Culling
	std::vector<uint32_t> opaqueDraws;
	opaqueDraws.reserve(_mainDrawContext.opaqueSurfaces.size());
	for (uint32_t index = 0; index < _mainDrawContext.opaqueSurfaces.size(); ++index) {
		if (IsVisible(_mainDrawContext.opaqueSurfaces[index], _sceneData.viewProjection)) {
			opaqueDraws.push_back(index);
		}
	}
	std::vector<uint32_t> maskedDraws;
	maskedDraws.reserve(_mainDrawContext.maskedSurfaces.size());
	for (uint32_t index = 0; index < _mainDrawContext.maskedSurfaces.size(); ++index) {
		if (IsVisible(_mainDrawContext.maskedSurfaces[index], _sceneData.viewProjection)) {
			maskedDraws.push_back(index);
		}
	}
	std::vector<uint32_t> transparentDraws;
	transparentDraws.reserve(_mainDrawContext.transparentSurfaces.size());
	for (uint32_t index = 0; index < _mainDrawContext.transparentSurfaces.size(); ++index) {
		if (IsVisible(_mainDrawContext.transparentSurfaces[index], _sceneData.viewProjection)) {
			transparentDraws.push_back(index);
		}
	}

	// Sorting the opaque surfaces by material and mesh
	std::ranges::sort(opaqueDraws, [&](const uint32_t& iA, const uint32_t& iB) {
		const RenderObject& A = _mainDrawContext.opaqueSurfaces[iA];
		const RenderObject& B = _mainDrawContext.opaqueSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		return A.material < B.material;
	});
	std::ranges::sort(maskedDraws, [&](const uint32_t& iA, const uint32_t& iB) {
		const RenderObject& A = _mainDrawContext.maskedSurfaces[iA];
		const RenderObject& B = _mainDrawContext.maskedSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		return A.material < B.material;
	});
	std::ranges::sort(transparentDraws, [&](const uint32_t& iA, const uint32_t& iB) {
		const float distanceA = glm::length(_mainCamera._position - glm::vec3(_mainDrawContext.transparentSurfaces[iA].transform[3])); // Using squared distance is fine for sorting
		const float distanceB = glm::length(_mainCamera._position - glm::vec3(_mainDrawContext.transparentSurfaces[iB].transform[3]));
		return distanceA > distanceB; // Draw farthest first
	});

	_stats.drawcallCount                                                         = 0;
	_stats.triangleCount                                                         = 0;
	std::chrono::time_point<std::chrono::steady_clock> start                     = std::chrono::steady_clock::now();

	// 1 Uniform buffer, holds a struct of GPUSceneData
	AllocatedBuffer                                    uniformBufferGPUSceneData = CreateBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	GetCurrentFrame().deletionQueue.PushFunction([=, this]() {
		DestroyBuffer(uniformBufferGPUSceneData);
	});
	GPUSceneData* uniformBufferData   = static_cast<GPUSceneData*>(uniformBufferGPUSceneData.allocation->GetMappedData());
	*uniformBufferData                = _sceneData;
	VkDescriptorSet  globalDescriptor = GetCurrentFrame().frameDescriptors.Allocate(_logicalGPU, _gpuSceneDataDescriptorLayout);
	DescriptorWriter uniformWriter;
	uniformWriter.WriteBuffer(0, uniformBufferGPUSceneData.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	uniformWriter.UpdateSet(_logicalGPU, globalDescriptor);

	// Begin a render pass connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(_colorImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depthAttachment = vkinit::DepthAttachmentInfo(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	VkRenderingInfo           renderInfo      = vkinit::RenderingInfo(_drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(commandBuffer, &renderInfo);

	// Defined outside the draw function, this is the state we will try to skip
	MaterialPipeline* lastPipeline       = nullptr;
	MaterialInstance* lastMaterial       = nullptr;
	VkBuffer          lastIndexBuffer    = VK_NULL_HANDLE;

	// TODO: Need to make this easier to understand, because the Draw() function is gathering draw context, and not recording draws for Vulkan yet.
	auto              actualDrawFunction = [&](const RenderObject& renderObject) {
        // Step 1: Bind Pipeline
        if (renderObject.material != lastMaterial) {
            lastMaterial = renderObject.material;
            // Rebind pipeline and descriptors if the material changed
            if (renderObject.material->pipeline != lastPipeline) {
                lastPipeline = renderObject.material->pipeline;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
                // TODO: Maybe cache viewport and scissor. This works since we are dynamically changing pipelines.
                VkViewport viewport = {};
                viewport.x          = 0;
                viewport.y          = 0;
                viewport.width      = static_cast<float>(_windowExtent.width);
                viewport.height     = static_cast<float>(_windowExtent.height);
                viewport.minDepth   = 0.f;
                viewport.maxDepth   = 1.f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                VkRect2D scissor      = {};
                scissor.offset.x      = 0;
                scissor.offset.y      = 0;
                scissor.extent.width  = _windowExtent.width;
                scissor.extent.height = _windowExtent.height;
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            }
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->layout, 1, 1, &renderObject.material->descriptorSet, 0, nullptr);
        }
        // Step 2: Bind index buffer
        // Rebind index buffer if needed
        if (renderObject.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = renderObject.indexBuffer;
            vkCmdBindIndexBuffer(commandBuffer, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
        // Step 3: Calculate Mesh Matrix Transform, and then
        // Calculate final mesh matrix
        GPUDrawPushConstants gPUDrawPushConstants {};
        gPUDrawPushConstants.worldMatrix         = renderObject.transform;
        gPUDrawPushConstants.vertexBufferAddress = renderObject.vertexBufferAddress;
        vkCmdPushConstants(commandBuffer, renderObject.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &gPUDrawPushConstants);

        // THE ACTUAL DRAW CALL
        vkCmdDrawIndexed(commandBuffer, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);

        _stats.drawcallCount++;
        _stats.triangleCount += renderObject.indexCount / 3;
    };

	for (const uint32_t& renderIndex : opaqueDraws) {
		actualDrawFunction(_mainDrawContext.opaqueSurfaces[renderIndex]);
	}
	for (const uint32_t& renderIndex : maskedDraws) {
		actualDrawFunction(_mainDrawContext.maskedSurfaces[renderIndex]);
	}
	for (const uint32_t& renderIndex : transparentDraws) {
		actualDrawFunction(_mainDrawContext.transparentSurfaces[renderIndex]);
	}

	_mainDrawContext.opaqueSurfaces.clear();
	_mainDrawContext.maskedSurfaces.clear();
	_mainDrawContext.transparentSurfaces.clear();

	std::chrono::time_point<std::chrono::steady_clock> end     = std::chrono::steady_clock::now();
	std::chrono::microseconds                          elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.meshDrawTime                                        = elapsed.count() / 1000.f;

	vkCmdEndRendering(commandBuffer);
}

void PantomirEngine::DrawImgui(const VkCommandBuffer commandBuffer, const VkImageView targetImageView) const {
	VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const VkRenderingInfo     renderInfo      = vkinit::RenderingInfo(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(commandBuffer, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRendering(commandBuffer);
}

void PantomirEngine::UpdateScene() {
	const auto start = std::chrono::steady_clock::now();

	_mainCamera.Update(_deltaTime);

	const glm::mat4 view       = _mainCamera.GetViewMatrix();

	// Camera projection
	const glm::mat4 projection = GetProjectionMatrix();

	_sceneData.view            = view;
	_sceneData.proj            = projection;
	_sceneData.viewProjection  = projection * view;

	_mainDrawContext.opaqueSurfaces.clear();
	_mainDrawContext.maskedSurfaces.clear();
	_mainDrawContext.transparentSurfaces.clear();
	_loadedScenes["Echidna1"]->FillDrawContext(glm::mat4 { 1.f }, _mainDrawContext);

	// Some default lighting parameters
	_sceneData.ambientColor                                                   = glm::vec4(.1f);
	_sceneData.sunlightColor                                                  = glm::vec4(1.f);
	_sceneData.sunlightDirection                                              = glm::vec4(0, 0, -1.0, 1.f);

	const std::chrono::time_point<std::chrono::steady_clock> end              = std::chrono::steady_clock::now();
	const std::chrono::duration<float>                       elapsed          = std::chrono::duration<float>(end - start);
	const float                                              deltaTimeSeconds = elapsed.count();

	_stats.sceneUpdateTime                                                    = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	_deltaTime                                                                = std::clamp(deltaTimeSeconds, 0.0001f, 0.016f);
}

int main(int argc, char* argv[]) {
	return PantomirEngine::GetInstance().Start();
}
