#ifndef MODELLOADER_H_
#define MODELLOADER_H_

#include <filesystem>
#include <vector>

struct Vertex;

class ModelLoader final {
public:
	struct RawModel final {
		std::vector<Vertex>   vertices;
		std::vector<uint32_t> indices;
	};

	ModelLoader() = default;
	static RawModel LoadModel(const std::filesystem::path& path);
};


#endif /*! MODELLOADER_H_ */