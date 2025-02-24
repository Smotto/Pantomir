#ifndef QUEUEFAMILYINDICES_H_
#define QUEUEFAMILYINDICES_H_

#include <optional>
struct QueueFamilyIndices final {
	constexpr bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
};

#endif /*! QUEUEFAMILYINDICES_H_ */