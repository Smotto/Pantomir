#include "VKPipelines.h"
#include <fstream>

bool vkutil::LoadShaderModule(const char*     filePath,
                              VkDevice        device,
                              VkShaderModule* outShaderModule) {
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