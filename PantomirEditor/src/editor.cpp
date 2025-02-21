#include "editor.h"
#include "PantomirEngine.h"

#include <memory>

int main() {
	std::unique_ptr<PantomirEngine> engine = std::make_unique<PantomirEngine>();
	engine->Start();

	return EXIT_SUCCESS;
}
