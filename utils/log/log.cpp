#include <filesystem>
#include <fstream>
#include <iostream>
#include <utils/log/log.hpp>

void logs::init(std::string log_dir) {
  logFile = log_dir + "/fsat-sens.log";

  if (!std::filesystem::exists(log_dir))
    std::filesystem::create_directories(log_dir);
}

void logs::log(const LogLevel level, std::string str) {
  std::lock_guard<std::recursive_mutex> guard{logMutex};
  std::string coloredStr = str;
  std::string out = str;

  switch (level) {
    case LOG:
      out = "[LOG] " + str;
      coloredStr = str;
      break;
    case WARN:
      out = "[WARN] " + str;
      coloredStr = "\033[1;33m" + str + "\033[0m";  // yellow
      break;
    case ERR:
      out = "[ERR] " + str;
      coloredStr = "\033[1;31m" + str + "\033[0m";  // red
      break;
    case INFO:
      out = "[INFO] " + str;
      coloredStr = "\033[1;32m" + str + "\033[0m";  // green
      break;
    case DEBUG:
      out = "[DEBUG] " + str;
      coloredStr = "\033[1;34m" + str + "\033[0m";  // blue
      break;
    default:
      break;
  }

  if (!disableFileLogs && !logFile.empty()) {
    std::ofstream ofs;
    ofs.open(logFile, std::ios::out | std::ios::app);
    ofs << out << "\n";
    ofs.close();
  }

  if (!disableJournal) std::cout << ((coloredLogs) ? coloredStr : str) << '\n';
}
