#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <deque>
#include <memory>
#include <unordered_map>
#include <string>

#include "sensor.hpp"

int salve();

class SensorManager {
    using sensor_ptr = std::shared_ptr<Sensor>;

  public:
    SensorManager(std::string hwmonPath) : m_szBaseHwmonPath{hwmonPath} {}
    void    runManager();
    void    registerSensors(char** sensorNames);
    int32_t startTracking(std::string& sensorName);

  private:
    std::unordered_map<std::string, sensor_ptr> m_mSensorMap;
    std::deque<sensor_ptr>                      m_dTrackingSensors;
    std::string                                 m_szBaseHwmonPath;
};

#endif
