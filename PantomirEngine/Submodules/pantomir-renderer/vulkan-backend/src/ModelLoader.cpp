#include "ModelLoader.h"
#include "Vertex.h"

#include <iostream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

ModelLoader::RawModel ModelLoader::LoadModel(const std::filesystem::path& relativePath) {
    RawModel model;

    std::cout << "Attempting to load: " << relativePath.string() << "\n";

    Assimp::Importer importer;

    // Import the model with some common post-processing flags
    const aiScene* scene = importer.ReadFile(relativePath.string(),
        aiProcess_Triangulate |           // Ensure all faces are triangles
        aiProcess_GenSmoothNormals |      // Generate normals if not present
        aiProcess_FlipUVs |               // Flip texture coordinates (if needed)
        aiProcess_JoinIdenticalVertices | // Optimize mesh
        aiProcess_CalcTangentSpace);      // Calculate tangent space for normal mapping

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp error: " + std::string(importer.GetErrorString()));
    }

    // Process all meshes in the scene
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];

            // Process each vertex in the face
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int vertexIndex = face.mIndices[j];

                Vertex vertex{};

                // Position
                if (mesh->HasPositions()) {
                    const aiVector3D& pos = mesh->mVertices[vertexIndex];
                    vertex.pos = {pos.x, pos.y, pos.z};
                }

                // Texture coordinates
                if (mesh->HasTextureCoords(0)) {
                    const aiVector3D& tex = mesh->mTextureCoords[0][vertexIndex];
                    vertex.texCoord = {tex.x, tex.y};
                    vertex.color = {1.0f, 1.0f, 1.0f}; // Default white color
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }

                // Normals
                if (mesh->HasNormals()) {
                    const aiVector3D& normal = mesh->mNormals[vertexIndex];
                    vertex.normal = {normal.x, normal.y, normal.z};
                }

                // Vertex colors
                if (mesh->HasVertexColors(0)) {
                    const aiColor4D& color = mesh->mColors[0][vertexIndex];
                    vertex.color = {color.r, color.g, color.b};
                }

                // Check if this is a unique vertex
                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                    model.vertices.push_back(vertex);
                }

                // Add the index
                model.indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    std::cout << "Loaded " << scene->mNumMeshes << " meshes, "
              << model.vertices.size() << " unique vertices, "
              << model.indices.size() << " indices\n";

    // Importer will automatically clean up the scene when it goes out of scope
    return model;
}