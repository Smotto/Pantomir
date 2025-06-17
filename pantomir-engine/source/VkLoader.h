#ifndef VKLOADER_H_
#define VKLOADER_H_

#include "VkTypes.h"
#include <unordered_map>
#include <filesystem>

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
};

struct MeshAsset {
	std::string name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
};

class PantomirEngine;

namespace VkLoader {
	std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(PantomirEngine* engine, const std::filesystem::path& filePath);
};

#endif /*! VKLOADER_H_ */
