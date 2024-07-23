#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sensor.hpp"

class SensorManager {
    using fileIter   = std::filesystem::directory_entry;
    using sensorPtr  = std::shared_ptr<Sensor>;
    using sensorPair = std::pair<std::string, sensorPtr>;

  public:
    SensorManager(std::string hwmonPath) : m_szBaseHwmonPath{hwmonPath} {}
    void    runManager(void);
    void    registerSensors(std::vector<std::string>&& searchList);
    void    trackRegisteredDevices(void);
    int32_t startTracking(std::string_view sensorName);

  private:
    void                                       matchForDeviceNames(std::vector<std::string>& devs, std::string name, fileIter it);
    void                                       readTrackedSensors(void);
    std::unordered_map<std::string, sensorPtr> m_mSensorMap;
    std::deque<sensorPair>                     m_dTrackingSensors;
    std::string                                m_szBaseHwmonPath;
};

#endif
