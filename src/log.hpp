#ifndef LOG_HPP_
#define LOG_HPP_

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

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

    inline void                 log(const LogLevel level, const char* fmt, ...) {
        std::lock_guard<std::recursive_mutex> guard{logMutex};
        std::string                           logMsg = "";
        char*                                 allocFmt;
        va_list                               args;

        va_start(args, fmt);

        if (vasprintf(&allocFmt, fmt, args) < 0) {
            log(ERR, "Failed to allocated log string!!");
            va_end(args);
            return;
        }

        va_end(args);

        if (!disableTime) {
            auto    now        = std::chrono::system_clock::now();
            auto    time_t_now = std::chrono::system_clock::to_time_t(now);
            std::tm utc_tm;
            gmtime_r(&time_t_now, &utc_tm);
            std::ostringstream oss;
            oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");

            logMsg += "[" + oss.str() + "]";
        }

        logMsg += allocFmt;

        free(allocFmt);

        log(level, logMsg);
    }

} // namespace logs

#endif
