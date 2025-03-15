#include "VkDescriptors.h"
#include "VkTypes.h"

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