#ifndef MODELLOADER_H_
#define MODELLOADER_H_

#include <string>
#include <vector>

struct Vertex;
class ModelLoader final {
public:
	struct RawModel final {
		std::vector<Vertex>   vertices;
		std::vector<uint32_t> indices;
	};

	ModelLoader() = default;
	RawModel LoadModel(const std::string& path);
};

#endif /*! MODELLOADER_H_ */