#ifndef LOGGER_H_
#define LOGGER_H_

#include <chrono> // for proper timestamp
#include <format>
#include <fstream>
#include <mutex>
#include <source_location> // for file/line info (optional later)
#include <string>

namespace Pantomir {

	enum class LogLevel { Debug,
		                  Info,
		                  Warning,
		                  Error,
		                  Fatal };

	class Logger {
	public:
		static Logger& GetInstance();

		void           SetMinLogLevel(LogLevel level) noexcept {
            m_minLevel = level;
		}

		// Core logging function – now takes std::format_string
		template <typename... Args>
		void Log(const std::string&          category,
		         LogLevel                    level,
		         std::format_string<Args...> fmt,
		         Args&&... args) {
			if (level < m_minLevel)
				return;

			std::string message = std::format(fmt, std::forward<Args>(args)...);
			WriteLog(level, category, message);
		}

	private:
		Logger();
		~Logger();

		void          WriteLog(LogLevel           level,
		                       const std::string& category,
		                       const std::string& message);

		LogLevel      m_minLevel = LogLevel::Debug;
		std::mutex    m_logMutex;
		std::ofstream m_logFile;
	};

} // namespace Pantomir

#endif /* LOGGER_H_ */
