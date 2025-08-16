#ifndef VKPUSHCONSTANTS_H_
#define VKPUSHCONSTANTS_H_

struct ComputePushConstants
{
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
	alignas(16) glm::vec3 cameraPosition;
	glm::mat4 cameraRotation;
};

struct ComputeEffect
{
	const char* name;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	ComputePushConstants pushConstants;
};

// Push constants for our mesh object draws
struct GPUDrawPushConstants
{
	glm::mat4 worldSpaceTransform;
	VkDeviceAddress vertexBufferAddress;
};

struct HDRIPushConstants
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

#endif /* VKPUSHCONSTANTS_H_ */
