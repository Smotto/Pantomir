#ifndef LOGGERMACROS_H_
#define LOGGERMACROS_H_

#include "Logger.h"

#define LOG_DEBUG(category, format, ...)  Pantomir::Logger::GetInstance().Log(category, Pantomir::LogLevel::Debug, format, ##__VA_ARGS__)
#define LOG_INFO(category, format, ...)   Pantomir::Logger::GetInstance().Log(category, Pantomir::LogLevel::Info, format, ##__VA_ARGS__)
#define LOG_WARN(category, format, ...)   Pantomir::Logger::GetInstance().Log(category, Pantomir::LogLevel::Warning, format, ##__VA_ARGS__)
#define LOG_ERROR(category, format, ...)  Pantomir::Logger::GetInstance().Log(category, Pantomir::LogLevel::Error, format, ##__VA_ARGS__)
#define LOG_FATAL(category, format, ...)  Pantomir::Logger::GetInstance().Log(category, Pantomir::LogLevel::Fatal, format, ##__VA_ARGS__)

#endif /* LOGGERMACROS_H_ */
