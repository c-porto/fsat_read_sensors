#ifndef LOG_HPP_
#define LOG_HPP_

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

enum LogLevel {
	NONE = -1,
	LOG = 0,
	INFO,
	WARN,
	ERR,
	DEBUG,
};

namespace logs
{
inline std::string logFile;
inline std::string dataFile;
inline bool disableJournal = false;
inline bool disableFileLogs = false;
inline bool coloredLogs = true;
inline std::recursive_mutex logMutex;

void init(std::string logDir);
void log(const LogLevel level, std::string str);

void logSensorData(std::string &sensorName, std::string &type, double data);

inline void log(const LogLevel level, const char *fmt, ...)
{
	std::lock_guard<std::recursive_mutex> guard{ logMutex };
	std::string logMsg = "";
	char *allocFmt;
	va_list args;

	va_start(args, fmt);

	if (vasprintf(&allocFmt, fmt, args) < 0) {
		log(ERR, "Failed to allocated log string!!");
		va_end(args);
		return;
	}

	va_end(args);

	logMsg += allocFmt;

	free(allocFmt);

	log(level, logMsg);
}

}

#endif
