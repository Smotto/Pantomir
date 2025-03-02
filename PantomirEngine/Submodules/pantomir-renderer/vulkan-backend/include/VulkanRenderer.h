#ifndef VULKANRENDERER_H_
#define VULKANRENDERER_H_

#include "Camera.h"
#include "InputEvents.h"
#include "Vertex.h"
#include "VulkanBufferManager.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class InputManager;
class VulkanResourceManager;
class VulkanDeviceManager;
struct GLFWwindow;

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR        capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>   presentModes;
};

class VulkanRenderer final {
public:
	VulkanRenderer(GLFWwindow* window, InputManager* inputManager, const VulkanDeviceManager* deviceManager, VulkanResourceManager* resourceManager);
	~VulkanRenderer();

	void DrawFrame();
	void DeviceWaitIdle();
	void UpdateUniformBuffers(uint32_t currentImage);

private:
	GLFWwindow*                         m_window;
	InputManager*                       m_inputManager;
	const VulkanDeviceManager*          m_deviceManager;
	VulkanResourceManager*              m_resourceManager;

	VulkanBufferManager::RenderModel*   m_model;

	// TODO: Abstract this out later.
	std::shared_ptr<Camera>             m_camera;

	// Swap chain and related
	VkSwapchainKHR                      m_swapChain;
	std::vector<VkImage>                m_swapChainImages;
	VkFormat                            m_swapChainImageFormat;
	VkExtent2D                          m_swapChainExtent;
	std::vector<VkImageView>            m_swapChainImageViews;
	std::vector<VkFramebuffer>          m_swapChainFramebuffers;

	// Pipeline and render pass
	VkRenderPass                        m_renderPass;
	VkDescriptorSetLayout               m_descriptorSetLayout;
	VkDescriptorPool                    m_descriptorPool;
	std::vector<VkDescriptorSet>        m_descriptorSets;
	VkPipelineLayout                    m_pipelineLayout;
	VkPipeline                          m_graphicsPipeline;

	// Uniform Buffer
	std::vector<VkBuffer>               m_uniformBuffers;
	std::vector<VkDeviceMemory>         m_uniformBuffersMemory;
	std::vector<void*>                  m_uniformBuffersMapped;

	// Command pool and command buffers
	std::vector<VkCommandBuffer>        m_commandBuffers;

	// Staging Buffer
	VkBuffer                            m_stagingBuffer;
	VkDeviceMemory                      m_stagingBufferMemory;

	// Texture Image
	VkImage                             m_textureImage;
	VkDeviceMemory                      m_textureImageMemory;
	VkImageView                         m_textureImageView;
	VkSampler                           m_textureSampler;

	// MIP
	uint32_t                            m_mipLevels;

	// Color
	VkImage                             m_colorImage;
	VkDeviceMemory                      m_colorImageMemory;
	VkImageView                         m_colorImageView;

	// Depth
	VkImage                             m_depthImage;
	VkDeviceMemory                      m_depthImageMemory;
	VkImageView                         m_depthImageView;

	// Synchronization
	std::vector<VkSemaphore>            m_imageAvailableSemaphores;
	std::vector<VkSemaphore>            m_renderFinishedSemaphores;
	std::vector<VkFence>                m_inFlightFences;
	uint32_t                            m_currentFrame       = 0;
	bool                                m_framebufferResized = false;

	// Functions
	void                                CreateSwapChain();
	void                                RecreateSwapChain();
	void                                CreateImageViews();
	void                                CreateRenderPass();
	void                                CreateDescriptorSetLayout();
	void                                CreateGraphicsPipeline();
	void                                CreateColorResources();
	void                                CreateDepthResources();
	void                                CreateFramebuffers();
	void                                CreateTextureImage();
	void                                CreateTextureImageView();
	void                                CreateTextureSampler();
	void                                CreateUniformBuffers();
	void                                CreateDescriptorPool();
	void                                CreateDescriptorSets();
	void                                CreateCommandBuffers();
	void                                CreateSyncObjects();

	void                                CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView                         CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void                                CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkShaderModule                      CreateShaderModule(const std::vector<char>& code);

	void                                CleanupSwapChain();

	[[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;
	[[nodiscard]] uint32_t              FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	SwapChainSupportDetails             QuerySwapChainSupport(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR                  ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR                    ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D                          ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void                                GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void                                TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	VkCommandBuffer                     BeginSingleTimeCommands();
	void                                EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void                                RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void                                CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void                                CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	static std::vector<char>            ReadFile(const std::string& filename);
	VkFormat                            FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat                            FindDepthFormat();
	bool                                HasStencilComponent(VkFormat format);
};
#endif /*! VULKANRENDERER_H_ */
