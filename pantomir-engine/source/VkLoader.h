#ifndef VKLOADER_H_
#define VKLOADER_H_

#include "VkDescriptors.h"
#include "VkTypes.h"
#include <filesystem>
#include <unordered_map>

struct GLTFMaterial {
	MaterialInstance data;
};

struct GeoSurface {
	uint32_t                      startIndex;
	uint32_t                      count;

	std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
	std::string             name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers          meshBuffers;
};

class PantomirEngine;

struct LoadedGLTF : public IRenderable {
	// storage for all the data on a given glTF file
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>>    meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>>         nodes;
	std::unordered_map<std::string, AllocatedImage>                images;
	std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

	// Nodes that don't have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>>                             topNodes;

	std::vector<VkSampler>                                         samplers;

	DescriptorAllocatorGrowable                                    descriptorPool;

	AllocatedBuffer                                                materialDataBuffer;

	PantomirEngine*                                                creator;

	~LoadedGLTF() {
		ClearAll();
	};

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:
	void ClearAll();
};

namespace VkLoader {
	std::optional<std::shared_ptr<LoadedGLTF>>             LoadGltf(PantomirEngine* engine, std::string_view filePath);
	std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(PantomirEngine* engine, const std::filesystem::path& filePath);
}; // namespace VkLoader

#endif /*! VKLOADER_H_ */
