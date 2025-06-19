#include "VkDescriptors.h"
#include "LoggerMacros.h"
#include "VkTypes.h"

void DescriptorWriter::Clear() {
	_imageInfos.clear();
	_writes.clear();
	_bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set) {
	for (VkWriteDescriptorSet& write : _writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device,
	                       static_cast<uint32_t>(_writes.size()),
	                       _writes.data(),
	                       0,
	                       nullptr);
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {
	VkDescriptorBufferInfo& info  = _bufferInfos.emplace_back(VkDescriptorBufferInfo {
	     .buffer = buffer,
	     .offset = offset,
	     .range  = size });

	VkWriteDescriptorSet    write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding              = binding;
	write.dstSet                  = VK_NULL_HANDLE; // left empty for now until we need to write it
	write.descriptorCount         = 1;
	write.descriptorType          = type;
	write.pBufferInfo             = &info;

	_writes.push_back(write);
}

void DescriptorWriter::WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
	VkDescriptorImageInfo& info  = _imageInfos.emplace_back(VkDescriptorImageInfo {
	     .sampler     = sampler,
	     .imageView   = image,
	     .imageLayout = layout });

	VkWriteDescriptorSet   write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding             = binding;
	write.dstSet                 = VK_NULL_HANDLE; // left empty for now until we need to write it
	write.descriptorCount        = 1;
	write.descriptorType         = type;
	write.pImageInfo             = &info;

	_writes.push_back(write);
}

void DescriptorAllocatorGrowable::Init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
	_ratios.clear();

	for (auto r : poolRatios) {
		_ratios.push_back(r);
	}

	VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

	_setsPerPool             = maxSets * 1.5; // grow it next allocation

	_readyPools.push_back(newPool);
}

VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext) {
	// Get or create a pool to allocate from
	VkDescriptorPool            poolToUse = GetPool(device);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext                       = pNext;
	allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool              = poolToUse;
	allocInfo.descriptorSetCount          = 1;
	allocInfo.pSetLayouts                 = &layout;

	VkDescriptorSet ds;
	VkResult        result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

	// Allocation failed. Try again
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

		_fullPools.push_back(poolToUse);

		poolToUse                = GetPool(device);
		allocInfo.descriptorPool = poolToUse;

		VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
	}

	_readyPools.push_back(poolToUse);
	return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device) {
	VkDescriptorPool newPool;
	if (_readyPools.size() != 0) {
		newPool = _readyPools.back();
		_readyPools.pop_back();
	} else {
		// need to create a new pool
		newPool      = CreatePool(device, _setsPerPool, _ratios);

		_setsPerPool = _setsPerPool * 1.5;
		if (_setsPerPool > 4092) {
			_setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize {
		    .type            = ratio.type,
		    .descriptorCount = uint32_t(ratio.ratio * setCount) });
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags                      = 0;
	pool_info.maxSets                    = setCount;
	pool_info.poolSizeCount              = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes                 = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
	return newPool;
}

void DescriptorAllocatorGrowable::ClearPools(VkDevice device) {
	for (auto p : _readyPools) {
		vkResetDescriptorPool(device, p, 0);
	}
	for (auto p : _fullPools) {
		vkResetDescriptorPool(device, p, 0);
		_readyPools.push_back(p);
	}
	_fullPools.clear();
}

void DescriptorAllocatorGrowable::DestroyPools(VkDevice device) {
	for (auto p : _readyPools) {
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	_readyPools.clear();
	for (auto p : _fullPools) {
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	_fullPools.clear();
}

void DescriptorAllocator::InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize {
		    .type            = ratio._type,
		    .descriptorCount = uint32_t(ratio._ratio * maxSets) });
	}

	VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags                      = 0;
	pool_info.maxSets                    = maxSets;
	pool_info.poolSizeCount              = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes                 = poolSizes.data();

	vkCreateDescriptorPool(device, &pool_info, nullptr, &_pool);
}

void DescriptorAllocator::ClearDescriptors(VkDevice device) {
	vkResetDescriptorPool(device, _pool, 0);
}

void DescriptorAllocator::DestroyPool(VkDevice device) {
	vkDestroyDescriptorPool(device, _pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout) {
	VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext                       = nullptr;
	allocInfo.descriptorPool              = _pool;
	allocInfo.descriptorSetCount          = 1;
	allocInfo.pSetLayouts                 = &layout;

	VkDescriptorSet descriptor_set;
	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptor_set));

	return descriptor_set;
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags) {
	for (auto& b : _bindings) {
		b.stageFlags |= shaderStages;
	}

	VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.pNext                           = pNext;

	info.pBindings                       = _bindings.data();
	info.bindingCount                    = static_cast<uint32_t>(_bindings.size());
	info.flags                           = flags;

	VkDescriptorSetLayout descriptor_set_layout;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptor_set_layout));

	return descriptor_set_layout;
}

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type) {
	VkDescriptorSetLayoutBinding newbind {};
	newbind.binding         = binding;
	newbind.descriptorCount = 1;
	newbind.descriptorType  = type;

	_bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear() {
	_bindings.clear();
}