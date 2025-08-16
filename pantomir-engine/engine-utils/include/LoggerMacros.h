#ifndef LOGGERMACROS_H_
#define LOGGERMACROS_H_

#include "Logger.h"
#include <string>

/*
================================================================================================

    Macros for PantomirLogger

================================================================================================
*/

// Define log categories as global constants
namespace LogCategory
{
	inline const std::string Engine = "Engine";
	inline const std::string Engine_Platform = "Engine::Platform";
	inline const std::string Engine_Renderer = "Engine::Renderer";
	inline const std::string Engine_Utils = "Engine::Utils";

	inline const std::string Editor = "Editor";
	inline const std::string Tools = "Tools";

	inline const std::string Temp = "Temp";
} // namespace LogCategory

namespace Pantomir
{
	// Helper to create custom category strings
	inline std::string MakeLogCategory(const std::initializer_list<std::string>& categories)
	{
		if (categories.begin() == categories.end())
			return "Unknown";

		const std::string* it = categories.begin();
		std::string        result = *it;
		++it;

		for (; it != categories.end(); ++it)
		{
			result += "::" + *it;
		}

		return result;
	}
} // namespace Pantomir

// Single LOG macro with category and level parameters
#define LOG(category, level, formatStr, ...) \
	Pantomir::Logger::GetInstance().Log(     \
	    LogCategory::category,               \
	    Pantomir::LogLevel::level,           \
	    formatStr,                           \
	    ##__VA_ARGS__)

// Custom category helper macro
#define LOG_CUSTOM(categories, level, formatStr, ...) \
	Pantomir::Logger::GetInstance().Log(              \
	    Pantomir::MakeLogCategory(categories),        \
	    Pantomir::LogLevel::level,                    \
	    formatStr,                                    \
	    ##__VA_ARGS__)

#endif /* LOGGERMACROS_H_ */
