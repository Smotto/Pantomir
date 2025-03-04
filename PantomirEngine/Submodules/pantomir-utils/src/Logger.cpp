#include "Logger.h"

#include <iostream>
#include <ctime>

namespace Pantomir {

Logger& Logger::GetInstance() {
	static Logger instance;
	return instance;
}

Logger::Logger() {
	m_logFile.open("pantomir_log.txt", std::ios::out | std::ios::app);
}

Logger::~Logger() {
	if (m_logFile.is_open()) m_logFile.close();
}

void Logger::SetMinLogLevel(LogLevel level) {
	m_minLevel = level;
}

void Logger::WriteLog(LogLevel level, const std::string& category, const std::string& message) {
	static const char* levelNames[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

	std::lock_guard<std::mutex> lock(m_logMutex);

	time_t now = time(nullptr);
	std::string timestamp = ctime(&now);
	timestamp.pop_back(); // Remove newline

	std::string logMessage = "[" + timestamp + "] [" + levelNames[static_cast<int>(level)] + "] [" + category + "] " + message;

	std::cout << logMessage << "\n";
	if (m_logFile.is_open()) {
		m_logFile << logMessage << "\n";
	}
	m_logFile.flush();
	std::cout.flush();
}

} // namespace Pantomir
