#ifndef SENSOR_HPP_
#define SENSOR_HPP_

#include <optional>
#include <string>
#include <vector>

namespace sensor
{
inline std::string measureTypes[] = { "temp", "current", "shunt_voltage", "voltage", "power" };

struct SensorDataEntry {
	std::string sensorName;
	std::string sensorType;
	std::string measurementType;
	double value;
};

class Sensor {
    public:
	virtual ~Sensor() = default;
	virtual std::optional<SensorDataEntry> read(std::string const &sensorType) = 0;
	std::vector<std::string> supported_types_;
	std::string getName() const
	{
		return name_;
	}

    protected:
	std::string name_;
};
}

#endif
