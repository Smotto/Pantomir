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

std::optional<std::vector<std::shared_ptr<MeshAsset>>> VkLoader::LoadGltfMeshes(PantomirEngine* engine, const std::filesystem::path& filePath) {
	using namespace fastgltf;

	Parser parser(fastgltf::Extensions::KHR_mesh_quantization);
	Expected<GltfDataBuffer> dataBuffer = GltfDataBuffer::FromPath(filePath);
	if (!dataBuffer) {
		LOG(Temp, Warning, "Failed to load GLTF: {}", getErrorMessage(dataBuffer.error()));
		return std::nullopt;
	}

	constexpr Options gltfOptions = Options::LoadExternalBuffers;
	Expected<Asset> assetResult = parser.loadGltf(dataBuffer.get(), filePath.parent_path(), gltfOptions);
	if (!assetResult) {
		LOG(Temp, Warning, "Failed to parse GLTF: {}", getErrorMessage(assetResult.error()));
		return std::nullopt;
	}
	Asset& asset = assetResult.get();

	std::vector<std::shared_ptr<MeshAsset>> meshAssets;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (Mesh& mesh : asset.meshes) {
		vertices.clear();
		indices.clear();

		MeshAsset meshAsset;
		meshAsset.name = mesh.name;

		for (const Primitive& primitive : mesh.primitives) {
			if (!primitive.indicesAccessor.has_value()) continue;

			const Accessor& idxAccessor = asset.accessors[primitive.indicesAccessor.value()];

			GeoSurface surface;
			surface.startIndex = static_cast<uint32_t>(indices.size());
			surface.count = static_cast<uint32_t>(idxAccessor.count);

			size_t baseVertex = vertices.size();

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
			if (posIt == primitive.attributes.end()) continue;

			const Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
			size_t vertexStart = vertices.size();
			vertices.resize(vertexStart + posAccessor.count);

			iterateAccessorWithIndex<glm::vec3>(asset, posAccessor, [&](glm::vec3 v, size_t i) {
				Vertex& vert = vertices[vertexStart + i];
				vert.position = v;
				vert.normal = glm::vec3(1.0f, 0.0f, 0.0f); // default
				vert.color = glm::vec4(1.0f);
				vert.uv_x = 0.0f;
				vert.uv_y = 0.0f;
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
		constexpr bool OverrideColors = true;
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

