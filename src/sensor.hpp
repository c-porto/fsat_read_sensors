#ifndef SENSOR_HPP_
#define SENSOR_HPP_

#include <optional>
#include <string>

enum eMeasureType {
    TEMP,
    CURRENT,
    SHUNT_VOLT,
    BUS_VOLT,
    POWER,
};

enum eSensorType : int32_t {
    INA219 = 0,
    TMP112,
};

class Sensor {
  public:
    Sensor(std::string path, eSensorType sensor_type) : m_szDriverFile{path}, m_eIC{sensor_type} {}
    std::optional<double> read(const eMeasureType type) const;

  private:
    const std::string m_szDriverFile;
    const eSensorType m_eIC;
};

#endif
