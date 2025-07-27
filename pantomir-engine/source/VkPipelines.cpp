#include "VkPipelines.h"

#include "LoggerMacros.h"
#include "VkInitializers.h"

#include <fstream>

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device) {
	// Make viewport state from our stored viewport and scissor.
	// at the moment we won't support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState       = {};
	viewportState.sType                                   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext                                   = nullptr;

	viewportState.viewportCount                           = 1;
	viewportState.scissorCount                            = 1;

	VkPipelineColorBlendStateCreateInfo colorBlending     = {};
	colorBlending.sType                                   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext                                   = nullptr;

	colorBlending.logicOpEnable                           = VK_FALSE;
	colorBlending.logicOp                                 = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount                         = 1;
	colorBlending.pAttachments                            = &_colorBlendAttachment;

	// Completely clear VertexInputStateCreateInfo, as we have no need for it
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// Build the actual pipeline
	// We know use all of the info structs we have been writing into, into this one to create the pipeline
	VkGraphicsPipelineCreateInfo         pipelineInfo     = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// Connect the renderInfo to the pNext extension mechanism
	pipelineInfo.pNext                                    = &_renderInfo;

	pipelineInfo.stageCount                               = static_cast<uint32_t>(_shaderStages.size());
	pipelineInfo.pStages                                  = _shaderStages.data();
	pipelineInfo.pVertexInputState                        = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState                      = &_inputAssembly;
	pipelineInfo.pViewportState                           = &viewportState;
	pipelineInfo.pRasterizationState                      = &_rasterizer;
	pipelineInfo.pMultisampleState                        = &_multisampling;
	pipelineInfo.pColorBlendState                         = &colorBlending;
	pipelineInfo.pDepthStencilState                       = &_depthStencil;
	pipelineInfo.layout                                   = _pipelineLayout;

	VkDynamicState                   state[]              = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo          = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.pDynamicStates                            = &state[0];
	dynamicInfo.dynamicStateCount                         = 2;
	pipelineInfo.pDynamicState                            = &dynamicInfo;

	// It's easy to error out on create graphics pipeline, so we handle it a bit
	// Better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		LOG(Engine_Renderer, Error, "Failed to create pipeline.");
		return VK_NULL_HANDLE;
	} else {
		return newPipeline;
	}
}

void PipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
	_shaderStages.clear();
	_shaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
	_shaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology) {
	_inputAssembly.topology               = topology;
	// We are not going to use primitive restart
	_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::SetPolygonMode(VkPolygonMode mode) {
	_rasterizer.polygonMode = mode;
	_rasterizer.lineWidth   = 1.f;
}

void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
	_rasterizer.cullMode  = cullMode;
	_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::SetMultisamplingNone() {
	_multisampling.sampleShadingEnable   = VK_FALSE;
	// Multisampling defaulted to no multisampling (1 sample per pixel)
	_multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	_multisampling.minSampleShading      = 1.0f;
	_multisampling.pSampleMask           = nullptr;
	// No alpha to coverage either
	_multisampling.alphaToCoverageEnable = VK_FALSE;
	_multisampling.alphaToOneEnable      = VK_FALSE;
}

void PipelineBuilder::DisableBlending() {
	// Default write mask
	_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// No blending
	_colorBlendAttachment.blendEnable    = VK_FALSE;
}

void PipelineBuilder::SetColorAttachmentFormat(VkFormat format) {
	_colorAttachmentFormat              = format;
	// Connect the format to the renderInfo structure
	_renderInfo.colorAttachmentCount    = 1;
	_renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void PipelineBuilder::SetDepthFormat(VkFormat format) {
	_renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::EnableDepthtest(bool depthWriteEnable, VkCompareOp op) {
	_depthStencil.depthTestEnable       = VK_TRUE;
	_depthStencil.depthWriteEnable      = depthWriteEnable;
	_depthStencil.depthCompareOp        = op;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable     = VK_FALSE;
	_depthStencil.front                 = {};
	_depthStencil.back                  = {};
	_depthStencil.minDepthBounds        = 0.f;
	_depthStencil.maxDepthBounds        = 1.f;
}

void PipelineBuilder::DisableDepthtest() {
	_depthStencil.depthTestEnable       = VK_FALSE;
	_depthStencil.depthWriteEnable      = VK_FALSE;
	_depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable     = VK_FALSE;
	_depthStencil.front                 = {};
	_depthStencil.back                  = {};
	_depthStencil.minDepthBounds        = 0.f;
	_depthStencil.maxDepthBounds        = 1.f;
}

void PipelineBuilder::Clear() {
	_inputAssembly        = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	_rasterizer           = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	_colorBlendAttachment = {};
	_multisampling        = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	_pipelineLayout       = {};
	_depthStencil         = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	_renderInfo           = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	_shaderStages.clear();
}

void PipelineBuilder::EnableBlendingAdditive() {
	_colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_colorBlendAttachment.blendEnable         = VK_TRUE;
	_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	_colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void PipelineBuilder::EnableBlendingAlphablend() {
	_colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_colorBlendAttachment.blendEnable         = VK_TRUE;
	_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	_colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	_colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

bool vkutil::LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule) {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary); // Open file with cursor at the end.

	if (!file.is_open()) {
		return false;
	}

	// Find what the size of the wfile is by looking up the location of the cursor
	// (it will be that size in bytes)
	size_t                fileSize = (size_t)file.tellg();

	// SPIR-V expects the buffer to be on uint32, so make sure to reserve an int.
	// Vector big enough for the entire file.
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// Put file cursor at beginning.
	file.seekg(0);

	// Load the entire file into the buffer.
	file.read((char*)buffer.data(), fileSize);

	// Now that the file is loaded into the buffer, we can close it.
	file.close();

	// Create a new shader module, using the buffer we loaded.
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext                    = nullptr;

	// codeSize has to be in bytes, so multiply the ints in the buffer by size of
	//	int to know the real size of the buffer
	create_info.codeSize                 = buffer.size() * sizeof(uint32_t);
	create_info.pCode                    = buffer.data();

	// Check that the creation succeeds
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}

	*outShaderModule = shaderModule;

	return true;
}