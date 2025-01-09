#ifndef LOG_HPP_
#define LOG_HPP_

#include <chrono>
#include <cmath>
#include <iomanip>
#include <mutex>
#include <string>
#include <format>

#include "sensor.hpp"

enum LogLevel {
    NONE = -1,
    LOG  = 0,
    INFO,
    WARN,
    ERR,
};

namespace logs {
    inline std::string          logFile;
    inline std::string          dataFile;
    inline bool                 disableJournal       = false;
    inline bool                 disableTime          = true;
    inline bool                 disableStdOut        = true;
    inline bool                 disableSensorStdOut  = true;
    inline bool                 disableSensorFileLog = true;
    inline bool                 coloredLogs          = true;
    inline std::recursive_mutex logMutex;

    void                        init(std::string logDir);
    void                        log(const LogLevel level, std::string str);

    void                        logSensorData(std::string sensorName, eMeasureType type, double data);

    template <typename... Args>
    void log(const LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        std::lock_guard<std::recursive_mutex> guard{logMutex};
        std::string                 logMsg = "";

        if (!disableTime) {
            auto        now       = std::chrono::system_clock::now();
            std::string timestamp = std::format("{:%FT%T}Z", now);
            logMsg += std::format("[{}]", timestamp);
        }

        logMsg += std::vformat(fmt.get(), std::make_format_args(args...));

        log(level, logMsg);
    }

} // namespace logs

#endif
