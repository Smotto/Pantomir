#include "VkLoader.h"

#include "LoggerMacros.h"

#include "PantomirEngine.h"
#include "VkTypes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

// This will make stb_image add the definitions of the functions into this cpp file for linking
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <unordered_set>

/* Free Functions */
VkFilter ExtractFilter(fastgltf::Filter filter) {
	switch (filter) {
		// Nearest samplers
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

		// Linear samplers
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

std::optional<AllocatedImage> LoadImage(PantomirEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
	AllocatedImage newImage {};
	int            width                 = 0;
	int            height                = 0;
	int            channelCount          = 0;

	constexpr int  desiredChannelCount   = 4;

	const auto     CreateImageFromMemory = [&](const stbi_uc* sourceData, int dataSize, bool forceStaging = false) -> bool {
        unsigned char* imageData = stbi_load_from_memory(sourceData, dataSize, &width, &height, &channelCount, desiredChannelCount);
        if (imageData == nullptr) {
            LOG(Engine, Error, "stbi_load_from_memory failed: {}", stbi_failure_reason());
            return false;
        }

        VkExtent3D imageExtent {};
        imageExtent.width  = static_cast<uint32_t>(width);
        imageExtent.height = static_cast<uint32_t>(height);
        imageExtent.depth  = 1;

        newImage           = engine->CreateImage(imageData, imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, forceStaging);
        stbi_image_free(imageData);
        return true;
	};

	const auto CreateImageFromFile = [&](const std::string& filePath) -> bool {
		unsigned char* imageData = stbi_load(filePath.c_str(), &width, &height, &channelCount, desiredChannelCount);
		if (imageData == nullptr) {
			LOG(Engine, Error, "stbi_load failed for file '{}': {}", filePath, stbi_failure_reason());
			return false;
		}

		VkExtent3D imageExtent {};
		imageExtent.width  = static_cast<uint32_t>(width);
		imageExtent.height = static_cast<uint32_t>(height);
		imageExtent.depth  = 1;

		newImage           = engine->CreateImage(imageData, imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
		stbi_image_free(imageData);
		return true;
	};

	std::visit(fastgltf::visitor {
	               // URI Case
	               [&](fastgltf::sources::URI& uriSource) {
		               if (!uriSource.uri.isLocalPath()) {
			               LOG(Engine, Warning, "Image URI is not a local path. Skipping.");
			               return;
		               }
		               if (uriSource.fileByteOffset != 0) {
			               LOG(Engine, Warning, "Non-zero fileByteOffset not supported in image URI. Skipping.");
			               return;
		               }

		               const std::string filePath(uriSource.uri.path().begin(), uriSource.uri.path().end());
		               CreateImageFromFile(filePath);
	               },

	               // Embedded raw bytes
	               [&](fastgltf::sources::Vector& vectorSource) {
		               const stbi_uc* imageBytes = reinterpret_cast<const stbi_uc*>(vectorSource.bytes.data());
		               int            byteSize   = static_cast<int>(vectorSource.bytes.size());
		               CreateImageFromMemory(imageBytes, byteSize);
	               },

	               // BufferView image data
	               [&](fastgltf::sources::BufferView& bufferViewSource) {
		               fastgltf::BufferView& bufferView = asset.bufferViews[bufferViewSource.bufferViewIndex];
		               fastgltf::Buffer&     buffer     = asset.buffers[bufferView.bufferIndex];
		               std::visit(fastgltf::visitor {
		                              [&](fastgltf::sources::Array& arraySource) {
			                              const stbi_uc* dataPointer = reinterpret_cast<const stbi_uc*>(arraySource.bytes.data() + bufferView.byteOffset);
			                              int            dataSize    = static_cast<int>(bufferView.byteLength);
			                              CreateImageFromMemory(dataPointer, dataSize);
		                              },
		                              [&](fastgltf::sources::Vector& vectorSource) {
			                              const stbi_uc* dataPointer = reinterpret_cast<const stbi_uc*>(vectorSource.bytes.data() + bufferView.byteOffset);
			                              int            dataSize    = static_cast<int>(bufferView.byteLength);
			                              CreateImageFromMemory(dataPointer, dataSize);
		                              },
		                              [&](fastgltf::sources::ByteView& byteViewSource) {
			                              const stbi_uc* dataPointer = reinterpret_cast<const stbi_uc*>(byteViewSource.bytes.data() + bufferView.byteOffset);
			                              int            dataSize    = static_cast<int>(bufferView.byteLength);
			                              // For ByteView, we force staging to true
			                              CreateImageFromMemory(dataPointer, dataSize, true);
		                              },
		                              [&](fastgltf::sources::URI&) {
			                              LOG(Engine, Warning, "BufferView uses URI source. This may not be supported in binary GLB format.");
		                              },
		                              [&](auto&) {
			                              LOG(Engine, Error, "Unhandled buffer source type in BufferView.");
		                              } },
		                          buffer.data);
	               },
	               [&](auto&) {
		               LOG(Engine, Error, "Unhandled image data variant type.");
	               },
	           },
	           image.data);

	if (newImage.image == VK_NULL_HANDLE) {
		return std::nullopt;
	}

	return newImage;
}

std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(PantomirEngine* engine, const std::string_view& filePath) {
	LOG(Engine, Info, "Loading GLTF: {}", filePath);
	fastgltf::Parser                             parser { fastgltf::Extensions::KHR_materials_emissive_strength | fastgltf::Extensions::KHR_materials_specular };
	constexpr fastgltf::Options                  gltfParserOptions { fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers };
	std::filesystem::path                        path(filePath);
	fastgltf::Expected<fastgltf::GltfDataBuffer> dataBufferResult = fastgltf::GltfDataBuffer::FromPath(path);
	if (!dataBufferResult) {
		LOG(Engine, Error, "Failed to read GLTF file: {}", getErrorMessage(dataBufferResult.error()));
		return std::nullopt;
	}
	fastgltf::GltfDataBuffer            oneGiantDataBuffer = std::move(dataBufferResult.get());
	fastgltf::Expected<fastgltf::Asset> assetResult        = parser.loadGltf(oneGiantDataBuffer, path.parent_path(), gltfParserOptions);
	if (!assetResult) {
		LOG(Engine, Error, "Failed to load GLTF: {}", getErrorMessage(assetResult.error()));
		return std::nullopt;
	}
	fastgltf::Asset                                         gltfAsset = std::move(assetResult.get());

	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> poolSizeRatios { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		                                                                     { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		                                                                     { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	std::shared_ptr<LoadedGLTF>                             currentGLTFPointer = std::make_shared<LoadedGLTF>();
	currentGLTFPointer->_enginePtr                                             = engine;
	LoadedGLTF& currentGLTF                                                    = *currentGLTFPointer;
	currentGLTF._descriptorPool.Init(engine->_logicalGPU, gltfAsset.materials.size(), poolSizeRatios);

	std::vector<std::shared_ptr<MeshAsset>>    meshes;
	std::vector<std::shared_ptr<Node>>         nodes;
	std::vector<AllocatedImage>                images;
	std::vector<std::shared_ptr<GLTFMaterial>> materials;

	// Load samplers
	for (fastgltf::Sampler& sampler : gltfAsset.samplers) {
		VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		samplerCreateInfo.maxLod              = VK_LOD_CLAMP_NONE; // Vulkan will use all mips down to the lowest
		samplerCreateInfo.minLod              = 0;                 // Only allow LODs >= 0 (highest quality level)
		samplerCreateInfo.magFilter           = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		samplerCreateInfo.minFilter           = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
		samplerCreateInfo.mipmapMode          = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(engine->_logicalGPU, &samplerCreateInfo, nullptr, &newSampler);

		currentGLTF._samplers.push_back(newSampler);
	}

	// Load all textures
	for (fastgltf::Image& image : gltfAsset.images) {
		std::optional<AllocatedImage> optionalAllocatedImage = LoadImage(engine, gltfAsset, image);

		if (optionalAllocatedImage.has_value()) {
			images.push_back(*optionalAllocatedImage);
			currentGLTF._images[image.name.c_str()] = *optionalAllocatedImage;
		} else {
			// We failed to load, so lets give the slot a default texture to not completely break loading
			images.push_back(engine->_errorCheckerboardImage);
			LOG(Engine, Warning, "glTF failed to load texture {}", image.name);
		}
	}

	// Create buffer to hold the material data
	currentGLTF._materialDataBuffer                                   = engine->CreateBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltfAsset.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	int                                        dataIndex              = 0;
	GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = static_cast<GLTFMetallic_Roughness::MaterialConstants*>(currentGLTF._materialDataBuffer.info.pMappedData);

	for (fastgltf::Material& material : gltfAsset.materials) {
		std::shared_ptr<GLTFMaterial> currentMaterial = std::make_shared<GLTFMaterial>();
		materials.push_back(currentMaterial);
		currentGLTF._materials[material.name.c_str()] = currentMaterial;

		GLTFMetallic_Roughness::MaterialConstants constants;
		constants.colorFactors.x      = material.pbrData.baseColorFactor[0];
		constants.colorFactors.y      = material.pbrData.baseColorFactor[1];
		constants.colorFactors.z      = material.pbrData.baseColorFactor[2];
		constants.colorFactors.w      = material.pbrData.baseColorFactor[3];

		constants.metalRoughFactors.x = material.pbrData.metallicFactor;
		constants.metalRoughFactors.y = material.pbrData.roughnessFactor;

		constants.emissiveFactors.x   = material.emissiveFactor[0];
		constants.emissiveFactors.y   = material.emissiveFactor[1];
		constants.emissiveFactors.z   = material.emissiveFactor[2];

		// Extensions
		constants.emissiveStrength    = material.emissiveStrength;
		if (material.specular) {
			constants.specularFactor = material.specular->specularFactor;
		} else {
			constants.specularFactor = 1.0f;
		}

		if (material.alphaMode == fastgltf::AlphaMode::Blend) {
			constants.alphaMode = 2;
		} else if (material.alphaMode == fastgltf::AlphaMode::Mask) {
			constants.alphaMode   = 1;
			constants.alphaCutoff = material.alphaCutoff;
		} else {
			constants.alphaMode = 0;
		}

		// Write material parameters to buffer
		sceneMaterialConstants[dataIndex] = constants;

		GLTFMetallic_Roughness::MaterialResources materialResources;

		// Default the material textures
		materialResources.colorImage        = engine->_whiteImage;
		materialResources.colorSampler      = engine->_defaultSamplerLinear;
		materialResources.metalRoughImage   = engine->_greyImage;
		materialResources.metalRoughSampler = engine->_defaultSamplerLinear;
		materialResources.emissiveImage     = engine->_whiteImage;
		materialResources.emissiveSampler   = engine->_defaultSamplerLinear;
		materialResources.normalImage       = engine->_whiteImage;
		materialResources.normalSampler     = engine->_defaultSamplerLinear;
		materialResources.specularImage     = engine->_whiteImage;
		materialResources.specularSampler   = engine->_defaultSamplerLinear;

		// Set the uniform buffer for the material data
		materialResources.dataBuffer        = currentGLTF._materialDataBuffer.buffer;
		materialResources.dataBufferOffset  = dataIndex * sizeof(GLTFMetallic_Roughness::MaterialConstants);

		// Grab textures from gltf file
		if (material.pbrData.baseColorTexture.has_value()) {
			size_t texIndex                = material.pbrData.baseColorTexture->textureIndex;
			size_t image                   = gltfAsset.textures[texIndex].imageIndex.value();
			size_t sampler                 = gltfAsset.textures[texIndex].samplerIndex.value();

			materialResources.colorImage   = images[image];
			materialResources.colorSampler = currentGLTF._samplers[sampler];
		}

		if (material.pbrData.metallicRoughnessTexture.has_value()) {
			size_t texIndex                     = material.pbrData.metallicRoughnessTexture->textureIndex;
			size_t image                        = gltfAsset.textures[texIndex].imageIndex.value();
			size_t sampler                      = gltfAsset.textures[texIndex].samplerIndex.value();

			materialResources.metalRoughImage   = images[image];
			materialResources.metalRoughSampler = currentGLTF._samplers[sampler];
		}

		if (material.emissiveTexture.has_value()) {
			size_t texIndex                   = material.emissiveTexture->textureIndex;
			size_t imageIndex                 = gltfAsset.textures[texIndex].imageIndex.value();
			size_t samplerIndex               = gltfAsset.textures[texIndex].samplerIndex.value();

			materialResources.emissiveImage   = images[imageIndex];
			materialResources.emissiveSampler = currentGLTF._samplers[samplerIndex];
		}

		if (material.normalTexture.has_value()) {
			size_t texIndex                 = material.normalTexture->textureIndex;
			size_t imageIndex               = gltfAsset.textures[texIndex].imageIndex.value();
			size_t samplerIndex             = gltfAsset.textures[texIndex].samplerIndex.value();

			materialResources.normalImage   = images[imageIndex];
			materialResources.normalSampler = currentGLTF._samplers[samplerIndex];
		}

		if (material.specular && material.specular->specularTexture.has_value()) {
			size_t texIndex                   = material.specular->specularTexture->textureIndex;
			size_t imageIndex                 = gltfAsset.textures[texIndex].imageIndex.value();
			size_t samplerIndex               = gltfAsset.textures[texIndex].samplerIndex.value();

			materialResources.specularImage   = images[imageIndex];
			materialResources.specularSampler = currentGLTF._samplers[samplerIndex];
		}

		MaterialPass passType = MaterialPass::Opaque;
		if (material.alphaMode == fastgltf::AlphaMode::Blend) {
			passType = MaterialPass::AlphaBlend;
		}
		if (material.alphaMode == fastgltf::AlphaMode::Mask) {
			passType = MaterialPass::AlphaMask;
		}
		VkCullModeFlagBits cullMode = material.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;

		currentMaterial->data       = engine->_metalRoughMaterial.WriteMaterial(engine->_logicalGPU, passType, cullMode, materialResources, currentGLTF._descriptorPool);

		dataIndex++;
	}

	// Use the same vectors for all meshes so that the memory doesn't reallocate as often
	std::vector<uint32_t> indices;
	std::vector<Vertex>   vertices;

	for (fastgltf::Mesh& mesh : gltfAsset.meshes) {
		std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
		meshes.push_back(newMesh);
		currentGLTF._meshes[mesh.name.c_str()] = newMesh;
		newMesh->name                          = mesh.name;

		// Clear the mesh arrays each mesh, we don't want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& primitive : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = static_cast<uint32_t>(indices.size());
			newSurface.count      = static_cast<uint32_t>(gltfAsset.accessors[primitive.indicesAccessor.value()].count);

			size_t initialVertex  = vertices.size();

			// Load indexes
			{
				fastgltf::Accessor& indexAccessor = gltfAsset.accessors[primitive.indicesAccessor.value()];
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltfAsset, indexAccessor, [&](std::uint32_t index) {
					indices.push_back(index + initialVertex);
				});
			}

			// Load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltfAsset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfAsset, posAccessor, [&](glm::vec3 vertex, size_t index) {
					Vertex newVertex;
					newVertex.position              = vertex;
					newVertex.normal                = { 1, 0, 0 };
					newVertex.color                 = glm::vec4 { 1.f };
					newVertex.uv_x                  = 0;
					newVertex.uv_y                  = 0;
					vertices[initialVertex + index] = newVertex;
				});
			}

			// Load vertex normals
			fastgltf::Attribute* normals = primitive.findAttribute("NORMAL");
			if (normals != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfAsset, gltfAsset.accessors[normals->accessorIndex], [&](glm::vec3 vertex, size_t index) {
					vertices[initialVertex + index].normal = vertex;
				});
			}

			// Load vertex tangents
			fastgltf::Attribute* tangents = primitive.findAttribute("TANGENT");
			if (tangents != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltfAsset, gltfAsset.accessors[tangents->accessorIndex], [&](glm::vec4 tangent, size_t index) {
					vertices[initialVertex + index].tangent = tangent;
				});
			}

			// Load UVs
			fastgltf::Attribute* uv = primitive.findAttribute("TEXCOORD_0");
			if (uv != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltfAsset, gltfAsset.accessors[uv->accessorIndex], [&](glm::vec2 vertex, size_t index) {
					vertices[initialVertex + index].uv_x = vertex.x;
					vertices[initialVertex + index].uv_y = vertex.y;
				});
			}

			// Load vertex colors
			fastgltf::Attribute* colors = primitive.findAttribute("COLOR_0");
			if (colors != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltfAsset, gltfAsset.accessors[colors->accessorIndex], [&](glm::vec4 vertex, size_t index) {
					vertices[initialVertex + index].color = vertex;
				});
			}

			if (primitive.materialIndex.has_value()) {
				newSurface.material = materials[primitive.materialIndex.value()];
			} else {
				newSurface.material = materials[0];
			}

			// Loop the vertices of this surface, find min/max bounds
			glm::vec3 minPosition = vertices[initialVertex].position;
			glm::vec3 maxPosition = vertices[initialVertex].position;
			for (int index = initialVertex; index < vertices.size(); index++) {
				minPosition = glm::min(minPosition, vertices[index].position);
				maxPosition = glm::max(maxPosition, vertices[index].position);
			}

			// Calculate origin and extents from the min/max, use extent length for radius
			newSurface.bounds.originPoint  = (maxPosition + minPosition) / 2.f;
			newSurface.bounds.extents      = (maxPosition - minPosition) / 2.f;
			newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

			newMesh->surfaces.push_back(newSurface);
		}

		newMesh->meshBuffers = engine->UploadMesh(indices, vertices);
	}

	// Load all nodes and their meshes
	for (fastgltf::Node& node : gltfAsset.nodes) {
		std::shared_ptr<Node> newNode;

		// Find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
		if (node.meshIndex.has_value()) {
			newNode                                      = std::make_shared<MeshNode>();
			static_cast<MeshNode*>(newNode.get())->_mesh = meshes[*node.meshIndex];
		} else {
			newNode = std::make_shared<Node>();
		}

		nodes.push_back(newNode);
		currentGLTF._nodes[node.name.c_str()];

		std::visit(fastgltf::visitor {
		               [&](const fastgltf::math::fmat4x4& matrix) {
			               memcpy(&newNode->_localTransform, matrix.data(), sizeof(matrix));
		               },
		               [&](const fastgltf::TRS& transform) {
			               glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
			               glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
			               glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

			               glm::mat4 tm             = glm::translate(glm::mat4(1.f), tl);
			               glm::mat4 rm             = glm::toMat4(rot);
			               glm::mat4 sm             = glm::scale(glm::mat4(1.f), sc);

			               newNode->_localTransform = tm * rm * sm;
		               } },
		           node.transform);
	}

	// Run loop again to setup transform hierarchy
	for (int i = 0; i < gltfAsset.nodes.size(); i++) {
		fastgltf::Node&        node      = gltfAsset.nodes[i];
		std::shared_ptr<Node>& sceneNode = nodes[i];

		for (auto& c : node.children) {
			sceneNode->_children.push_back(nodes[c]);
			nodes[c]->_parent = sceneNode;
		}
	}

	// Find the top nodes, with no parents
	for (auto& node : nodes) {
		if (node->_parent.lock() == nullptr) {
			currentGLTF._topNodes.push_back(node);
			node->RefreshTransform(glm::mat4 { 1.f });
		}
	}

	return currentGLTFPointer;
}

void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& drawContext) {
	// Create renderables from the sceneNodes
	for (auto& n : _topNodes) {
		n->Draw(topMatrix, drawContext);
	}
}

void LoadedGLTF::ClearAll() {
	VkDevice device = _enginePtr->_logicalGPU;

	_descriptorPool.DestroyPools(device);
	_enginePtr->DestroyBuffer(_materialDataBuffer);

	for (auto& [key, value] : _meshes) {
		_enginePtr->DestroyBuffer(value->meshBuffers.indexBuffer);
		_enginePtr->DestroyBuffer(value->meshBuffers.vertexBuffer);
	}

	for (auto& [key, value] : _images) {
		if (value.image == _enginePtr->_errorCheckerboardImage.image) {
			// Dont destroy the default images
			continue;
		}
		_enginePtr->DestroyImage(value);
	}

	for (auto& sampler : _samplers) {
		vkDestroySampler(device, sampler, nullptr);
	}
}