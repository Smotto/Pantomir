#ifndef VKDESCRIPTORS_H_
#define VKDESCRIPTORS_H_

#include "VkTypes.h"

struct DescriptorAllocator {
	struct PoolSizeRatio {
		VkDescriptorType _type;
		float            _ratio;
	};

	VkDescriptorPool _pool;

	void             InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void             ClearDescriptors(VkDevice device);
	void             DestroyPool(VkDevice device);

	VkDescriptorSet  Allocate(VkDevice device, VkDescriptorSetLayout layout);
};

struct DescriptorLayoutBuilder {
	std::vector<VkDescriptorSetLayoutBinding> _bindings;

	void                                      AddBinding(uint32_t binding, VkDescriptorType type);
	void                                      Clear();
	VkDescriptorSetLayout                     Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

class VkDescriptors {
};

#endif /*! VKDESCRIPTORS_H_ */
