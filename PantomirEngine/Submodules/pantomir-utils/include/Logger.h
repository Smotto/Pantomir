#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>
#include <mutex>
#include <string>
#include <format>

namespace Pantomir {
enum class LogLevel { Debug, Info, Warning, Error, Fatal };

class Logger {
public:
	static Logger& GetInstance();

	void SetMinLogLevel(LogLevel level);

	template<typename... Args> // variadic template
	void Log(const std::string& category, LogLevel level, const char* format, Args&&... args) {
		if (level < m_minLevel) return;

		std::string message = std::vformat(format, std::make_format_args(args...));
		WriteLog(level, category, message);
	}

private:
	Logger();
	~Logger();

	void WriteLog(LogLevel level, const std::string& category, const std::string& message);

	LogLevel m_minLevel = LogLevel::Debug;
	std::mutex m_logMutex;
	std::ofstream m_logFile;
};

} // namespace Pantomir

#endif /* LOGGER_H_ */
