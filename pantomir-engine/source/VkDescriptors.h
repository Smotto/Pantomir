#ifndef VKDESCRIPTORS_H_
#define VKDESCRIPTORS_H_

#include "VkTypes.h"

// ============================================================
// DescriptorPoolManager
// ============================================================
struct DescriptorPoolManager
{
	struct DescriptorTypeCountMultipliers
	{
		VkDescriptorType type;
		float setCountMultiplier;
	};

	void ClearPools(VkDevice device);
	void Init(VkDevice device, uint32_t setCount, std::span<DescriptorTypeCountMultipliers> descriptorTypeCountMultipliers);
	void DestroyPools(VkDevice device);

	VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, const void* pNext = nullptr);

private:
	VkDescriptorPool GetOrCreatePool(VkDevice device);
	VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, std::span<DescriptorTypeCountMultipliers> poolMultipliers);

	std::vector<DescriptorTypeCountMultipliers> _multipliers;
	std::vector<VkDescriptorPool> _fullPools;
	std::deque<VkDescriptorPool> _readyPools;
	uint32_t _setsPerPool = 0;
};

// ------------------------------------------------------------
// DescriptorLayoutBuilder
// ------------------------------------------------------------
struct DescriptorLayoutBuilder
{
	void AddBinding(uint32_t binding, VkDescriptorType type);
	VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

	void Clear();

private:
	std::vector<VkDescriptorSetLayoutBinding> _bindings;
};

// ------------------------------------------------------------
// DescriptorWriter
// ------------------------------------------------------------
struct DescriptorSetWriter
{
	void WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
	void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

	void Clear();
	void UpdateSet(VkDevice device, VkDescriptorSet set);

private:
	std::deque<VkDescriptorImageInfo> _imageInfos;
	std::deque<VkDescriptorBufferInfo> _bufferInfos;
	std::vector<VkWriteDescriptorSet> _writes;
};

#endif /*! VKDESCRIPTORS_H_ */
