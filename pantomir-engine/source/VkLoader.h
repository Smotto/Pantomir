#ifndef VKLOADER_H_
#define VKLOADER_H_

#include "VkDescriptors.h"
#include "VkTypes.h"
#include <filesystem>
#include <unordered_map>

namespace fastgltf {
	struct Image;
	class Asset;
} // namespace fastgltf

struct GLTFMaterial {
	MaterialInstance data;
};

struct Bounds {
	glm::vec3 originPoint;
	float     sphereRadius;
	glm::vec3 extents;
};

struct GeoSurface {
	uint32_t                      startIndex;
	uint32_t                      count;
	Bounds                        bounds;
	std::shared_ptr<GLTFMaterial> material;
};

struct RenderObject {
	uint32_t          indexCount;
	uint32_t          firstIndex;
	VkBuffer          indexBuffer;

	MaterialInstance* material;
	Bounds            bounds;
	glm::mat4         transform;
	VkDeviceAddress   vertexBufferAddress;
};

struct MeshAsset {
	std::string             name;
	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers          meshBuffers;
};

class PantomirEngine;

struct LoadedGLTF final : IRenderable {
	// Storage for all the data on a given glTF file
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>>    _meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>>         _nodes;
	std::unordered_map<std::string, AllocatedImage>                _images;
	std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> _materials;

	// Nodes that don't have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>>                             _topNodes;
	std::vector<VkSampler>                                         _samplers;
	DescriptorAllocatorGrowable                                    _descriptorPool;
	AllocatedBuffer                                                _materialDataBuffer;
	PantomirEngine*                                                _enginePtr;

	~LoadedGLTF() override {
		ClearAll();
	};

	void FillDrawContext(const glm::mat4& topMatrix, DrawContext& drawContext) override;

private:
	void ClearAll();
};

struct LoadedHDRI {
	AllocatedImage              _allocatedImage;

	VkSampler                   _sampler;
	DescriptorAllocatorGrowable _descriptorPool;
	PantomirEngine*             _enginePtr;

	~LoadedHDRI() {
		ClearAll();
	}

private:
	void ClearAll();
};

// Free functions
std::optional<AllocatedImage>              LoadImage(PantomirEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(PantomirEngine* engine, const std::string_view& filePath);
std::optional<std::shared_ptr<LoadedHDRI>> LoadHDRI(PantomirEngine* engine, const std::string_view& filePath);

#endif /*! VKLOADER_H_ */
