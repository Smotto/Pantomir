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

std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(PantomirEngine* engine, std::string_view filePath) {
	LOG(Engine, Info, "Loading GLTF: {}", filePath);

	std::shared_ptr<LoadedGLTF> currentGLTFPointer = std::make_shared<LoadedGLTF>();
	currentGLTFPointer->_enginePtr                 = engine;
	LoadedGLTF&      currentGLTF                   = *currentGLTFPointer;

	fastgltf::Parser parser {};

	constexpr auto   gltfParserOptions = fastgltf::Options::DontRequireValidAssetMember |
	                             fastgltf::Options::AllowDouble |
	                             fastgltf::Options::LoadExternalBuffers;

	std::filesystem::path path(filePath);
	auto                  dataBufferResult = fastgltf::GltfDataBuffer::FromPath(path);
	if (!dataBufferResult) {
		LOG(Engine, Error, "Failed to read GLTF file: {}", getErrorMessage(dataBufferResult.error()));
		return std::nullopt;
	}

	// Move out the data buffer (as a value) from the Expected
	fastgltf::GltfDataBuffer dataBuffer  = std::move(dataBufferResult.get());

	auto                     assetResult = parser.loadGltf(dataBuffer, path.parent_path(), gltfParserOptions);
	if (!assetResult) {
		LOG(Engine, Error, "Failed to load GLTF: {}", getErrorMessage(assetResult.error()));
		return std::nullopt;
	}

	fastgltf::Asset                                         gltfAsset = std::move(assetResult.get());

	// We can estimate the descriptors we will need accurately
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes     = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		                                                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		                                                                  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	currentGLTF._descriptorPool.Init(engine->_logicalGPU, gltfAsset.materials.size(), sizes);

	// Load samplers
	for (fastgltf::Sampler& sampler : gltfAsset.samplers) {
		VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		samplerCreateInfo.maxLod              = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.minLod              = 0;

		samplerCreateInfo.magFilter           = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		samplerCreateInfo.minFilter           = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		samplerCreateInfo.mipmapMode          = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(engine->_logicalGPU, &samplerCreateInfo, nullptr, &newSampler);

		currentGLTF._samplers.push_back(newSampler);
	}

	std::vector<std::shared_ptr<MeshAsset>>    meshes;
	std::vector<std::shared_ptr<Node>>         nodes;
	std::vector<AllocatedImage>                images;
	std::vector<std::shared_ptr<GLTFMaterial>> materials;

	// Load all textures
	for (fastgltf::Image& image : gltfAsset.images) {
		std::optional<AllocatedImage> optionalAllocatedImage = LoadImage(engine, gltfAsset, image);

		if (optionalAllocatedImage.has_value()) {
			images.push_back(*optionalAllocatedImage);
			currentGLTF._images[image.name.c_str()] = *optionalAllocatedImage;
		} else {
			// We failed to load, so lets give the slot a default white texture to not completely break loading
			images.push_back(engine->_errorCheckerboardImage);
			LOG(Engine, Info, "gltf failed to load texture {}", image.name);
		}
	}

	// Create buffer to hold the material data
	currentGLTF._materialDataBuffer                                   = engine->CreateBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltfAsset.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	int                                        data_index             = 0;
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

		if (material.alphaMode == fastgltf::AlphaMode::Blend) {
			constants.alphaMode = 2;
		} else if (material.alphaMode == fastgltf::AlphaMode::Mask) {
			constants.alphaMode   = 1;
			constants.alphaCutoff = material.alphaCutoff;
		} else {
			constants.alphaMode = 0;
		}

		// Write material parameters to buffer
		sceneMaterialConstants[data_index] = constants;

		GLTFMetallic_Roughness::MaterialResources materialResources;

		// Default the material textures
		materialResources.colorImage        = engine->_whiteImage;
		materialResources.colorSampler      = engine->_defaultSamplerLinear;
		materialResources.metalRoughImage   = engine->_whiteImage;
		materialResources.metalRoughSampler = engine->_defaultSamplerLinear;
		materialResources.emissiveImage     = engine->_whiteImage;
		materialResources.emissiveSampler   = engine->_defaultSamplerLinear;
		materialResources.normalImage       = engine->_whiteImage;
		materialResources.normalSampler     = engine->_defaultSamplerLinear;

		// Set the uniform buffer for the material data
		materialResources.dataBuffer        = currentGLTF._materialDataBuffer.buffer;
		materialResources.dataBufferOffset  = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);

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

		MaterialPass passType = MaterialPass::Opaque;
		if (material.alphaMode == fastgltf::AlphaMode::Blend) {
			passType = MaterialPass::AlphaBlend;
		}
		if (material.alphaMode == fastgltf::AlphaMode::Mask) {
			passType = MaterialPass::AlphaMask;
		}

		VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
		if (material.doubleSided) {
			cullMode = VK_CULL_MODE_NONE;
		}

		currentMaterial->data = engine->_metalRoughMaterial.WriteMaterial(engine->_logicalGPU, passType, cullMode, materialResources, currentGLTF._descriptorPool);

		data_index++;
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

		for (auto&& p : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = static_cast<uint32_t>(indices.size());
			newSurface.count      = static_cast<uint32_t>(gltfAsset.accessors[p.indicesAccessor.value()].count);

			size_t initialVertex  = vertices.size();

			// Load indexes
			{
				fastgltf::Accessor& indexAccessor = gltfAsset.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltfAsset, indexAccessor, [&](std::uint32_t idx) {
					indices.push_back(idx + initialVertex);
				});
			}

			// Load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltfAsset.accessors[p.findAttribute("POSITION")->accessorIndex];
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
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfAsset, gltfAsset.accessors[(*normals).accessorIndex], [&](glm::vec3 vertex, size_t index) {
					vertices[initialVertex + index].normal = vertex;
				});
			}

			// Load vertex tangents
			auto tangents = p.findAttribute("TANGENT");
			if (tangents != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltfAsset, gltfAsset.accessors[(*tangents).accessorIndex], [&](glm::vec4 tangent, size_t index) {
					vertices[initialVertex + index].tangent = tangent;
				});
			}

			// Load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltfAsset, gltfAsset.accessors[(*uv).accessorIndex], [&](glm::vec2 vertex, size_t index) {
					vertices[initialVertex + index].uv_x = vertex.x;
					vertices[initialVertex + index].uv_y = vertex.y;
				});
			}

			// Load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltfAsset, gltfAsset.accessors[(*colors).accessorIndex], [&](glm::vec4 vertex, size_t index) {
					vertices[initialVertex + index].color = vertex;
				});
			}

			if (p.materialIndex.has_value()) {
				newSurface.material = materials[p.materialIndex.value()];
			} else {
				newSurface.material = materials[0];
			}

			// Loop the vertices of this surface, find min/max bounds
			glm::vec3 minpos = vertices[initialVertex].position;
			glm::vec3 maxpos = vertices[initialVertex].position;
			for (int i = initialVertex; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}

			// Calculate origin and extents from the min/max, use extent length for radius
			newSurface.bounds.originPoint  = (maxpos + minpos) / 2.f;
			newSurface.bounds.extents      = (maxpos - minpos) / 2.f;
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
	// Create renderables from the scenenodes
	for (auto& n : _topNodes) {
		n->Draw(topMatrix, drawContext);
	}
}

std::optional<AllocatedImage> LoadImage(PantomirEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
	AllocatedImage newImage {};
	int            width, height, nrChannels;

	std::visit(
	    fastgltf::visitor {
	        [](auto& arg) {
	        },
	        [&](fastgltf::sources::URI& filePath) {
		        assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
		        assert(filePath.uri.isLocalPath());   // We're only capable of loading
		                                              // local files.

		        const std::string path(filePath.uri.path().begin(),
		                               filePath.uri.path().end());
		        unsigned char*    data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
		        if (data) {
			        VkExtent3D imagesize;
			        imagesize.width  = width;
			        imagesize.height = height;
			        imagesize.depth  = 1;

			        newImage         = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

			        stbi_image_free(data);
		        }
	        },
	        [&](fastgltf::sources::Vector& vector) {
		        unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
		        if (data) {
			        VkExtent3D imagesize;
			        imagesize.width  = width;
			        imagesize.height = height;
			        imagesize.depth  = 1;

			        newImage         = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

			        stbi_image_free(data);
		        }
	        },
	        [&](fastgltf::sources::BufferView& view) {
		        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
		        auto& buffer     = asset.buffers[bufferView.bufferIndex];

		        std::visit(fastgltf::visitor {
		                       [&](fastgltf::sources::Array& array) {
			                       const stbi_uc* ptr  = reinterpret_cast<const stbi_uc*>(array.bytes.data() + bufferView.byteOffset);
			                       int            size = static_cast<int>(bufferView.byteLength);

			                       unsigned char* data = stbi_load_from_memory(ptr, size, &width, &height, &nrChannels, 4);
			                       if (data) {
				                       VkExtent3D imagesize { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
				                       newImage = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
				                       stbi_image_free(data);
			                       } else {
				                       LOG(Engine, Error, "stbi_load_from_memory failed for Array: {}", stbi_failure_reason());
			                       }
		                       },
		                       [&](fastgltf::sources::Vector& vector) {
			                       const stbi_uc* ptr  = reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset);
			                       int            size = static_cast<int>(bufferView.byteLength);

			                       unsigned char* data = stbi_load_from_memory(ptr, size, &width, &height, &nrChannels, 4);
			                       if (data) {
				                       VkExtent3D imagesize { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
				                       newImage = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
				                       stbi_image_free(data);
			                       } else {
				                       LOG(Engine, Error, "stbi_load_from_memory failed for Vector buffer image: {}", stbi_failure_reason());
			                       }
		                       },
		                       [&](fastgltf::sources::ByteView& byteView) {
			                       const stbi_uc* ptr  = reinterpret_cast<const stbi_uc*>(byteView.bytes.data() + bufferView.byteOffset);
			                       int            size = static_cast<int>(bufferView.byteLength);

			                       unsigned char* data = stbi_load_from_memory(ptr, size, &width, &height, &nrChannels, 4);
			                       if (data) {
				                       VkExtent3D imagesize { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
				                       newImage = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
				                       stbi_image_free(data);
			                       } else {
				                       LOG(Engine, Error, "stbi_load_from_memory failed for ByteView buffer image: {}", stbi_failure_reason());
			                       }
		                       },
		                       [&](fastgltf::sources::URI& uri) {
			                       LOG(Engine, Warning, "GLTF buffer view is a URI. This might not be supported in embedded GLBs.");
		                       },
		                       [](auto& arg) {
			                       LOG(Engine, Error, "Unhandled buffer.data type: {}", typeid(arg).name());
		                       } },
		                   buffer.data);
	        },
	    },
	    image.data);

	// If any of the attempts to load the data failed, we haven't written the image
	// so handle is null
	if (newImage.image == VK_NULL_HANDLE) {
		return {};
	} else {
		return newImage;
	}
}

void LoadedGLTF::ClearAll() {
	VkDevice dv = _enginePtr->_logicalGPU;

	_descriptorPool.DestroyPools(dv);
	_enginePtr->DestroyBuffer(_materialDataBuffer);

	for (auto& [k, v] : _meshes) {
		_enginePtr->DestroyBuffer(v->meshBuffers.indexBuffer);
		_enginePtr->DestroyBuffer(v->meshBuffers.vertexBuffer);
	}

	for (auto& [k, v] : _images) {
		if (v.image == _enginePtr->_errorCheckerboardImage.image) {
			// Dont destroy the default images
			continue;
		}
		_enginePtr->DestroyImage(v);
	}

	for (auto& sampler : _samplers) {
		vkDestroySampler(dv, sampler, nullptr);
	}
}