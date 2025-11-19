#include "Logger.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace Pantomir {

	Logger& Logger::GetInstance() {
		static Logger instance;
		return instance;
	}

	Logger::Logger() {
		m_logFile.open("pantomir_log.txt", std::ios::out | std::ios::app);
	}

	Logger::~Logger() {
		if (m_logFile.is_open())
			m_logFile.close();
	}

	void Logger::WriteLog(LogLevel           level,
	                      const std::string& category,
	                      const std::string& message) {
		constexpr const char* levelNames[] = { "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };

		auto                  now = std::chrono::system_clock::now();
		auto                  time_t = std::chrono::system_clock::to_time_t(now);
		auto                  ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
		          1000;

		std::ostringstream ts;
		ts << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
		   << '.' << std::setfill('0') << std::setw(3) << ms.count();

		std::string                 logLine = std::format("[{}] [{}] [{}] {}",
                                          ts.str(),
                                          levelNames[static_cast<int>(level)],
                                          category,
                                          message);

		std::lock_guard<std::mutex> lock(m_logMutex);
		std::cout << logLine << std::endl;
		if (m_logFile.is_open()) {
			m_logFile << logLine << std::endl;
			m_logFile.flush();
		}
	}

} // namespace Pantomir