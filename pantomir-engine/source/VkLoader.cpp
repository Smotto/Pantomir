#include "VkLoader.h"

#include "LoggerMacros.h"
#include "stb_image.h"

#include "PantomirEngine.h"
#include "VkInitializers.h"
#include "VkTypes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

/* Free Functions */
VkFilter ExtractFilter(fastgltf::Filter filter) {
	switch (filter) {
		// nearest samplers
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

		// linear samplers
		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode ExtractMipmapMode(fastgltf::Filter filter) {
	switch (filter) {
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

std::optional<std::shared_ptr<LoadedGLTF>> VkLoader::LoadGltf(PantomirEngine* engine, std::string_view filePath) {
	LOG(Engine, Info, "Loading GLTF: {}", filePath);

	std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
	scene->creator                    = engine;
	LoadedGLTF& file = *scene.get();

	fastgltf::Parser parser {};

	constexpr auto   gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
	                             fastgltf::Options::AllowDouble |
	                             fastgltf::Options::LoadExternalBuffers;

	std::filesystem::path path(filePath);
	auto                  dataBufferResult = fastgltf::GltfDataBuffer::FromPath(path);
	if (!dataBufferResult) {
		LOG(Engine, Error, "Failed to read GLTF file: {}", getErrorMessage(dataBufferResult.error()));
		return std::nullopt;
	}

	// Move out the data buffer (as a value) from the Expected
	fastgltf::GltfDataBuffer dataBuffer = std::move(dataBufferResult.get());

	auto                     type       = fastgltf::determineGltfFileType(dataBuffer);
	if (type == fastgltf::GltfType::Invalid) {
		LOG(Engine, Error, "Could not determine glTF file type.");
		return std::nullopt;
	}

	auto assetResult = parser.loadGltf(dataBuffer, path.parent_path(), gltfOptions);
	if (!assetResult) {
		LOG(Engine, Error, "Failed to load GLTF: {}", getErrorMessage(assetResult.error()));
		return std::nullopt;
	}

	fastgltf::Asset gltf = std::move(assetResult.get());

	// If LoadedGLTF should hold this asset, assign it here
	// scene->asset = std::move(gltf);

	// we can stimate the descriptors we will need accurately
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		                                                              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		                                                              { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	file.descriptorPool.Init(engine->_device, gltf.materials.size(), sizes);

	// load samplers
	for (fastgltf::Sampler& sampler : gltf.samplers) {

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode= ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);

		file.samplers.push_back(newSampler);
	}

	for (fastgltf::Image& image : gltf.images) {

		_images.push_back(engine->_errorCheckerboardImage);
	}

	return scene;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> VkLoader::LoadGltfMeshes(PantomirEngine* engine, const std::filesystem::path& filePath) {
	using namespace fastgltf;

	Parser                   parser(fastgltf::Extensions::KHR_mesh_quantization);
	Expected<GltfDataBuffer> dataBuffer = GltfDataBuffer::FromPath(filePath);
	if (!dataBuffer) {
		LOG(Temp, Warning, "Failed to load GLTF: {}", getErrorMessage(dataBuffer.error()));
		return std::nullopt;
	}

	constexpr Options gltfOptions = Options::LoadExternalBuffers;
	Expected<Asset>   assetResult = parser.loadGltf(dataBuffer.get(), filePath.parent_path(), gltfOptions);
	if (!assetResult) {
		LOG(Temp, Warning, "Failed to parse GLTF: {}", getErrorMessage(assetResult.error()));
		return std::nullopt;
	}
	Asset&                                  asset = assetResult.get();

	std::vector<std::shared_ptr<MeshAsset>> meshAssets;
	std::vector<Vertex>                     vertices;
	std::vector<uint32_t>                   indices;

	for (Mesh& mesh : asset.meshes) {
		vertices.clear();
		indices.clear();

		MeshAsset meshAsset;
		meshAsset.name = mesh.name;

		for (const Primitive& primitive : mesh.primitives) {
			if (!primitive.indicesAccessor.has_value())
				continue;

			const Accessor& idxAccessor = asset.accessors[primitive.indicesAccessor.value()];

			GeoSurface      surface;
			surface.startIndex = static_cast<uint32_t>(indices.size());
			surface.count      = static_cast<uint32_t>(idxAccessor.count);

			size_t baseVertex  = vertices.size();

			// Index data
			if (idxAccessor.componentType == ComponentType::UnsignedInt) {
				iterateAccessor<uint32_t>(asset, idxAccessor, [&](uint32_t i) {
					indices.push_back(i + static_cast<uint32_t>(baseVertex));
				});
			} else if (idxAccessor.componentType == ComponentType::UnsignedShort) {
				iterateAccessor<uint16_t>(asset, idxAccessor, [&](uint16_t i) {
					indices.push_back(static_cast<uint32_t>(i) + static_cast<uint32_t>(baseVertex));
				});
			} else if (idxAccessor.componentType == ComponentType::UnsignedByte) {
				iterateAccessor<uint8_t>(asset, idxAccessor, [&](uint8_t i) {
					indices.push_back(static_cast<uint32_t>(i) + static_cast<uint32_t>(baseVertex));
				});
			} else {
				LOG(Temp, Warning, "Unsupported index component type: {}", static_cast<int>(idxAccessor.componentType));
				continue;
			}

			// Vertex positions
			const Attribute* posIt = primitive.findAttribute("POSITION");
			if (posIt == primitive.attributes.end())
				continue;

			const Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
			size_t          vertexStart = vertices.size();
			vertices.resize(vertexStart + posAccessor.count);

			iterateAccessorWithIndex<glm::vec3>(asset, posAccessor, [&](glm::vec3 v, size_t i) {
				Vertex& vert  = vertices[vertexStart + i];
				vert.position = v;
				vert.normal   = glm::vec3(1.0f, 0.0f, 0.0f); // default
				vert.color    = glm::vec4(1.0f);
				vert.uv_x     = 0.0f;
				vert.uv_y     = 0.0f;
			});

			// Normals
			auto normIt = primitive.findAttribute("NORMAL");
			if (normIt != primitive.attributes.end()) {
				const Accessor& normAccessor = asset.accessors[normIt->accessorIndex];
				iterateAccessorWithIndex<glm::vec3>(asset, normAccessor, [&](glm::vec3 v, size_t i) {
					vertices[vertexStart + i].normal = v;
				});
			}

			// UVs
			auto uvIt = primitive.findAttribute("TEXCOORD_0");
			if (uvIt != primitive.attributes.end()) {
				const Accessor& uvAccessor = asset.accessors[uvIt->accessorIndex];
				iterateAccessorWithIndex<glm::vec2>(asset, uvAccessor, [&](glm::vec2 v, size_t i) {
					vertices[vertexStart + i].uv_x = v.x;
					vertices[vertexStart + i].uv_y = v.y;
				});
			}

			// Colors
			auto colorIt = primitive.findAttribute("COLOR_0");
			if (colorIt != primitive.attributes.end()) {
				const Accessor& colorAccessor = asset.accessors[colorIt->accessorIndex];
				iterateAccessorWithIndex<glm::vec4>(asset, colorAccessor, [&](glm::vec4 v, size_t i) {
					vertices[vertexStart + i].color = v;
				});
			}

			meshAsset.surfaces.push_back(surface);
		}

		// Debug override: visualize normals as color
		constexpr bool OverrideColors = false;
		if (OverrideColors) {
			for (Vertex& v : vertices) {
				v.color = glm::vec4(v.normal, 1.0f);
			}
		}

		meshAsset.meshBuffers = engine->UploadMesh(indices, vertices);
		meshAssets.emplace_back(std::make_shared<MeshAsset>(std::move(meshAsset)));
	}

	return meshAssets;
}

void LoadedGLTF::ClearAll() {
}
