#ifndef LOGGERMACROS_H_
#define LOGGERMACROS_H_

/*
================================================================================================

    Macros for PantomirLogger

================================================================================================
*/

#include "Logger.h"

namespace LogCategory {
	inline constexpr std::string_view Engine = "Engine";
	inline constexpr std::string_view Engine_Platform = "Engine::Platform";
	inline constexpr std::string_view Engine_Renderer = "Engine::Renderer";
	inline constexpr std::string_view Engine_Utils = "Engine::Utils";
	inline constexpr std::string_view Editor = "Editor";
	inline constexpr std::string_view Tools = "Tools";
	inline constexpr std::string_view Temp = "Temp";
} // namespace LogCategory

// Primary macro – fully type-checked with std::format
#define LOG(category, level, fmt, ...)      \
	Pantomir::Logger::GetInstance().Log(    \
	    std::string(LogCategory::category), \
	    Pantomir::LogLevel::level,          \
	    fmt,                                \
	    ##__VA_ARGS__)

// Custom category macro
#define LOG_CUSTOM(categories, level, fmt, ...) \
	Pantomir::Logger::GetInstance().Log(        \
	    Pantomir::MakeLogCategory(categories),  \
	    Pantomir::LogLevel::level,              \
	    fmt,                                    \
	    ##__VA_ARGS__)

#endif /* LOGGERMACROS_H_ */
