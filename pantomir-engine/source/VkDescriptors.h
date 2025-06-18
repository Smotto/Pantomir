#ifndef VKDESCRIPTORS_H_
#define VKDESCRIPTORS_H_

#include "VkTypes.h"

struct DescriptorWriter {
	std::deque<VkDescriptorImageInfo>  _imageInfos;
	std::deque<VkDescriptorBufferInfo> _bufferInfos;
	std::vector<VkWriteDescriptorSet>  _writes;

	void                               WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
	void                               WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

	void                               Clear();
	void                               UpdateSet(VkDevice device, VkDescriptorSet set);
};

struct DescriptorAllocatorGrowable {
public:
	struct PoolSizeRatio {
		VkDescriptorType type;
		float            ratio;
	};

	void            Init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
	void            ClearPools(VkDevice device);
	void            DestroyPools(VkDevice device);

	VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
	VkDescriptorPool              GetPool(VkDevice device);
	VkDescriptorPool              CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio>    _ratios;
	std::vector<VkDescriptorPool> _fullPools;
	std::vector<VkDescriptorPool> _readyPools;
	uint32_t                      _setsPerPool;
};

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
	VkDescriptorSetLayout                     Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

	void                                      Clear();
	void                                      UpdateSet(VkDevice device, VkDescriptorSet set);
};

class VkDescriptors {
};

#endif /*! VKDESCRIPTORS_H_ */
