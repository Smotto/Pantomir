#include "VkDescriptors.h"
#include "LoggerMacros.h"
#include "PantomirFunctionLibrary.h"
#include "VkTypes.h"

// ============================================================
// DescriptorPoolManager
// ============================================================
void DescriptorPoolManager::Init(VkDevice device, uint32_t setCount, std::span<DescriptorTypeCountMultipliers> descriptorTypeCountMultipliers)
{
	_multipliers.clear();
	for (const DescriptorTypeCountMultipliers multiplier : descriptorTypeCountMultipliers)
	{
		_multipliers.push_back(multiplier);
	}
	const VkDescriptorPool newPool = CreatePool(device, setCount, descriptorTypeCountMultipliers);
	_setsPerPool = setCount * PantomirFunctionLibrary::GOLDEN_RATIO; // Next allocation grows the pool.
	_readyPools.push_back(newPool);
}

void DescriptorPoolManager::ClearPools(const VkDevice device)
{
	for (const VkDescriptorPool readyPool : _readyPools)
	{
		vkResetDescriptorPool(device, readyPool, 0);
	}
	for (VkDescriptorPool fullPool : _fullPools)
	{
		vkResetDescriptorPool(device, fullPool, 0);
		_readyPools.push_back(fullPool);
	}
	_fullPools.clear();
}

void DescriptorPoolManager::DestroyPools(VkDevice device)
{
	for (VkDescriptorPool readyPool : _readyPools)
	{
		vkDestroyDescriptorPool(device, readyPool, nullptr);
	}
	_readyPools.clear();
	for (VkDescriptorPool fullPool : _fullPools)
	{
		vkDestroyDescriptorPool(device, fullPool, nullptr);
	}
	_fullPools.clear();
}

VkDescriptorSet DescriptorPoolManager::Allocate(const VkDevice device, const VkDescriptorSetLayout layout, const void* pNext)
{
	// Get or create a pool to allocate from
	VkDescriptorPool poolToUse = GetOrCreatePool(device);

	VkDescriptorSetAllocateInfo allocateInfoWithPool = {};
	allocateInfoWithPool.pNext = pNext;
	allocateInfoWithPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfoWithPool.descriptorPool = poolToUse;
	allocateInfoWithPool.descriptorSetCount = 1;
	allocateInfoWithPool.pSetLayouts = &layout;

	VkDescriptorSet descriptorSet;
	const VkResult allocationResult = vkAllocateDescriptorSets(device, &allocateInfoWithPool, &descriptorSet);

	// Allocation failed. Try again
	if (allocationResult == VK_ERROR_OUT_OF_POOL_MEMORY || allocationResult == VK_ERROR_FRAGMENTED_POOL)
	{
		_fullPools.push_back(poolToUse);
		poolToUse = GetOrCreatePool(device);
		allocateInfoWithPool.descriptorPool = poolToUse;
		VK_CHECK(vkAllocateDescriptorSets(device, &allocateInfoWithPool, &descriptorSet));
	}

	_readyPools.push_back(poolToUse);
	return descriptorSet;
}

VkDescriptorPool DescriptorPoolManager::GetOrCreatePool(VkDevice device)
{
	VkDescriptorPool newPool;
	if (!_readyPools.empty())
	{
		newPool = _readyPools.back();
		_readyPools.pop_back();
	}
	else
	{
		// No pools available from ready - Need to create a new pool
		newPool = CreatePool(device, _setsPerPool, _multipliers);

		_setsPerPool = _setsPerPool * PantomirFunctionLibrary::GOLDEN_RATIO;
		if (_setsPerPool > 4092)
		{
			_setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorPoolManager::CreatePool(const VkDevice device, const uint32_t setCount, const std::span<DescriptorTypeCountMultipliers> poolMultipliers)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const auto [type, multiplier] : poolMultipliers)
	{
		poolSizes.push_back(VkDescriptorPoolSize {
		    .type = type,
		    .descriptorCount = static_cast<uint32_t>(multiplier * setCount) });
	}

	VkDescriptorPoolCreateInfo poolCreateInfo {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = 0;
	poolCreateInfo.maxSets = setCount;
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCreateInfo.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &newPool);

	return newPool;
}

// ============================================================
// DescriptorLayoutBuilder
// ============================================================
VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	for (VkDescriptorSetLayoutBinding& binding : _bindings)
	{
		binding.stageFlags |= shaderStages;
	}

	VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.pNext = pNext;

	info.pBindings = _bindings.data();
	info.bindingCount = static_cast<uint32_t>(_bindings.size());
	info.flags = flags;

	VkDescriptorSetLayout descriptor_set_layout;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptor_set_layout));

	return descriptor_set_layout;
}

void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding newDescriptorSetLayoutBinding {};
	newDescriptorSetLayoutBinding.binding = binding;
	newDescriptorSetLayoutBinding.descriptorCount = 1;
	newDescriptorSetLayoutBinding.descriptorType = type;

	_bindings.push_back(newDescriptorSetLayoutBinding);
}

void DescriptorLayoutBuilder::Clear()
{
	_bindings.clear();
}

// ============================================================
// DescriptorWriter
// ============================================================
void DescriptorSetWriter::Clear()
{
	_imageInfos.clear();
	_writes.clear();
	_bufferInfos.clear();
}

void DescriptorSetWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
	for (VkWriteDescriptorSet& write : _writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device,
	                       static_cast<uint32_t>(_writes.size()),
	                       _writes.data(),
	                       0,
	                       nullptr);
}

void DescriptorSetWriter::WriteBuffer(const int binding, const VkBuffer buffer, const size_t size, const size_t offset, const VkDescriptorType type)
{
	const VkDescriptorBufferInfo& descriptorBufferInfo = _bufferInfos.emplace_back(VkDescriptorBufferInfo {
	    .buffer = buffer,
	    .offset = offset,
	    .range = size });

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &descriptorBufferInfo;

	_writes.push_back(write);
}

void DescriptorSetWriter::WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
	VkDescriptorImageInfo& info = _imageInfos.emplace_back(VkDescriptorImageInfo {
	    .sampler = sampler,
	    .imageView = image,
	    .imageLayout = layout });

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &info;

	_writes.push_back(write);
}