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
	using fileIter = std::filesystem::directory_entry;
	using sensor_ptr = std::shared_ptr<Sensor>;
	using sensor_pair = std::pair<std::string, sensor_ptr>;

    public:
	SensorManager(std::string hwmonPath)
		: m_szBaseHwmonPath{ hwmonPath }
	{
	}
	void runManager(void);
	void registerSensors(std::vector<std::string>&& searchList);
	void trackRegisteredDevices(void);
	int32_t startTracking(std::string_view sensorName);

    private:
	void matchForDeviceNames(std::vector<std::string> &devs,
				 std::string name, fileIter it);
	void readTrackedSensors(void);
	std::unordered_map<std::string, sensor_ptr> m_mSensorMap;
	std::deque<sensor_pair> m_dTrackingSensors;
	std::string m_szBaseHwmonPath;
};

#endif
