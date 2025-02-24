#include "ModelLoader.h"
#include "Vertex.h"

#include <stdexcept>
#include <tiny_obj_loader.h>
#include <unordered_map>

ModelLoader::RawModel ModelLoader::LoadModel(const std::string& path) {
	// TODO: Can have a vulkan load implementation with the staging buffer to optimize. Less portable though, but a good trade-off. 
	RawModel                         model;

	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;
	std::string                      warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices;
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};
			vertex.pos = {
			    attrib.vertices[3 * index.vertex_index + 0],
			    attrib.vertices[3 * index.vertex_index + 1],
			    attrib.vertices[3 * index.vertex_index + 2]};
			vertex.texCoord = {
			    attrib.texcoords[2 * index.texcoord_index + 0],
			    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
				model.vertices.push_back(vertex);
			}
			model.indices.push_back(uniqueVertices[vertex]);
		}
	}

	return model;
}