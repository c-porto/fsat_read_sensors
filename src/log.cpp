#include "log.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/syslog.h>
#include <systemd/sd-journal.h>

void logs::init(std::string log_dir) {
    logFile  = log_dir + "/fsat-sens.log";
    dataFile = log_dir + "/read-sensors.log";

    if (!std::filesystem::exists(log_dir))
        std::filesystem::create_directories(log_dir);
}

void logs::log(const LogLevel level, std::string str) {
    std::lock_guard<std::recursive_mutex> guard{logMutex};
    std::string                           coloredStr = str;
    std::string                           out        = str;
    int                                   sysLog;

    switch (level) {
        case LOG:
            out        = "[LOG] " + str;
            coloredStr = str;
            sysLog     = LOG_NOTICE;
            break;
        case WARN:
            out        = "[WARN] " + str;
            coloredStr = "\033[1;33m" + str + "\033[0m"; // yellow
            sysLog     = LOG_WARNING;
            break;
        case ERR:
            out        = "[ERR] " + str;
            coloredStr = "\033[1;31m" + str + "\033[0m"; // red
            sysLog     = LOG_ERR;
            break;
        case INFO:
            out        = "[INFO] " + str;
            coloredStr = "\033[1;32m" + str + "\033[0m"; // green
            sysLog     = LOG_INFO;
            break;
        default: break;
    }

    if (!disableStdOut) {
        std::ofstream ofs;
        ofs.open(logFile, std::ios::out | std::ios::app);
        ofs << out << "\n";
        ofs.close();
    }

    if (!disableStdOut)
        std::cout << ((!coloredLogs) ? out : coloredStr) << std::endl;

    if (!disableJournal)
        sd_journal_print(sysLog, "%s", str.c_str());
}
