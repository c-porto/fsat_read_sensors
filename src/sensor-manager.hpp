#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sensor.hpp"

namespace fs = std::filesystem;

class CSensorManager {
    using fileIter    = std::filesystem::directory_entry;
    using sensorPtr  = std::shared_ptr<CSensor>;
    using sensorPair = std::pair<std::string, sensorPtr>;

  public:
    CSensorManager(std::string hwmonPath) : m_szBaseHwmonPath{hwmonPath} {}
    void    runManager(void);
    void    registerSensors(std::vector<std::string>&& searchList);
    void    trackRegisteredDevices(void);
    int32_t startTracking(std::string& sensorName);

  private:
    void                                        matchForDeviceNames(std::vector<std::string>& devs, std::string name, fs::path it);
    void                                        readTrackedSensors(void);
    std::unordered_map<std::string, sensorPtr> m_mSensorMap;
    std::deque<sensorPair>                     m_dTrackingSensors;
    std::string                                 m_szBaseHwmonPath;
};

#endif
