#ifndef VKLOADER_H_
#define VKLOADER_H_

#include "VkDescriptors.h"
#include "VkTypes.h"
#include <filesystem>
#include <unordered_map>

namespace fastgltf {
	struct Image;
	class Asset;
}
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
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>>    _meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>>         _nodes;
	std::unordered_map<std::string, AllocatedImage>                _images;
	std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> _materials;

	// Nodes that don't have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>>                             _topNodes;
	std::vector<VkSampler>                                         _samplers;
	DescriptorAllocatorGrowable                                    _descriptorPool;
	AllocatedBuffer                                                _materialDataBuffer;
	PantomirEngine*                                                _creator;

	~LoadedGLTF() {
		ClearAll();
	};

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:
	void ClearAll();
};

std::optional<AllocatedImage> LoadImage(PantomirEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(PantomirEngine* engine, std::string_view filePath);

#endif /*! VKLOADER_H_ */
