#include "ModelLoader.h"
#include "Vertex.h"

#include <iostream>
#include <stdexcept>
#include <tiny_obj_loader.h>

ModelLoader::RawModel ModelLoader::LoadModel(const std::string& relativePath) {
    RawModel model;

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./Assets"; // Point to the Assets folder in the root

    tinyobj::ObjReader reader;
    std::string fullPath = relativePath; // Use the relative path as-is
    std::cout << "Attempting to load: " << fullPath << "\n";
    if (!reader.ParseFromFile(fullPath, reader_config)) {
        std::cerr << "TinyObjReader Error: " << reader.Error() << "\n";
        throw std::runtime_error("Failed to load OBJ file: " + fullPath);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader Warning: " << reader.Warning();
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = static_cast<size_t>(shapes[s].mesh.num_face_vertices[f]);
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0],
                    attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1],
                    attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2]
                };
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0],
                        1.0f - attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1]
                    };
                    vertex.color = {1.0f, 1.0f, 1.0f};
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }
                if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                    vertex.normal = {
                        attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0],
                        attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1],
                        attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2]
                    };
                }
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                    model.vertices.push_back(vertex);
                }
                model.indices.push_back(uniqueVertices[vertex]);
            }
            index_offset += fv;
        }
    }

    std::cout << "Loaded " << shapes.size() << " shapes, "
              << model.vertices.size() << " unique vertices, "
              << model.indices.size() << " indices\n";

    return model;
}