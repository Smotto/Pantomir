#ifndef VKPIPELINES_H_
#define VKPIPELINES_H_

#include "VkTypes.h"

class PipelineBuilder
{
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipelineRenderingCreateInfo _renderInfo;
	VkFormat _colorAttachmentFormat;

	PipelineBuilder()
	{
		Clear();
	}
	void Clear();
	VkPipeline BuildPipeline(VkDevice device);
	void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	void SetInputTopology(VkPrimitiveTopology topology);
	void SetPolygonMode(VkPolygonMode mode);
	void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void SetMultisamplingNone();
	void DisableBlending();
	void EnableBlendingAdditive();
	void EnableBlendingAlphablend();
	void SetColorAttachmentFormat(VkFormat format);
	void SetDepthFormat(VkFormat format);
	void EnableDepthtest(bool depthWriteEnable, VkCompareOp op);
	void DisableDepthtest();
};

namespace vkutil
{
	bool LoadShaderModule(const char* filePath,
	                      VkDevice device,
	                      VkShaderModule* outShaderModule);
}

#endif /*! VKPIPELINES_H_ */
