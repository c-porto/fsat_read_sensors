#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mutex>
#include <cstdint>

#include "sensor.hpp"
#include "db.hpp"

#define TO_STRINGZ(x) #x

static inline const char* meas_type_to_str(eMeasureType type) {
    switch (type) {
        case TEMP: return TO_STRINGZ(TEMP); break;
        case SHUNT_VOLT: return TO_STRINGZ(SHUNT_VOLT); break;
        case CURRENT: return TO_STRINGZ(CURRENT); break;
        case BUS_VOLT: return TO_STRINGZ(BUS_VOLT); break;
        case POWER: return TO_STRINGZ(POWER); break;
        default: break;
    }
    return "";
};

namespace fs = std::filesystem;

class CSensorManager {
    using fileIter   = std::filesystem::directory_entry;
    using sensorPtr  = std::shared_ptr<CSensor>;
    using sensorPair = std::pair<std::string, sensorPtr>;

  public:
    CSensorManager(std::string hwmonPath, std::string dbPath) : m_szBaseHwmonPath{hwmonPath}, m_DB{dbPath} {}
    void    runManager(void);
    void    registerSensors(std::vector<std::string>&& searchList);
    void    registerSingleSensor(const std::string& sensorName);
    void    unregisterSingleSensor(const std::string& sensorName);
    void    trackRegisteredDevices(void);
    void    setMeasurementPeriod(const uint64_t time_ms);
    int32_t startTracking(std::string& sensorName);
    int32_t stopTracking(std::string& sensorName);

  private:
    void                                       matchForDeviceNames(std::vector<std::string>& devs, std::string name, fs::path it);
    void                                       readTrackedSensors(void);
    void                                       readAllInaTypes(const std::pair<std::string, std::shared_ptr<CSensor> >& ina);
    std::unordered_map<std::string, sensorPtr> m_mSensorMap;
    std::vector<sensorPair>                    m_vTrackingSensors;
    std::string                                m_szBaseHwmonPath;
    std::mutex                                 m_lock;
    uint64_t                                   m_MeasurementPeriodMS{1000ULL};
    CSqliteDb                                  m_DB;
};

#endif
