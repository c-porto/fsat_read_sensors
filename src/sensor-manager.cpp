#include "sensor-manager.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "log.hpp"
#include "sensor.hpp"

void SensorManager::readTrackedSensors(void)
{
	for (const auto &sensor : tracking_sensors_) {
		for (const auto &meas_type : sensor->supported_types_) {
			std::optional<sensor::SensorDataEntry> read = sensor->read(meas_type);

			if (read) {
				db_.addMeasurementToDB(*read);
				this->logMeasurement(*read);
			}
		}
	}
}

void SensorManager::runManager(void)
{
	while (true) {
		if (!tracking_sensors_.empty()) {
			this->readTrackedSensors();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(meas_period_ms_));
	}
}

int32_t SensorManager::trackRegisteredDevices(void)
{
	logs::log(INFO, "Start tracking register devices...\n");

	for (const auto &sensor : registered_sensors_)
		tracking_sensors_.push_back(sensor);

	return 0;
}

int32_t SensorManager::startTracking(std::string &sensorName)
{
	bool is_registered = false;
	std::shared_ptr<sensor::Sensor> found = nullptr;

	for (const auto &sensor : registered_sensors_) {
		if (sensor->getName() == sensorName) {
			is_registered = true;
			found = sensor;
			break;
		}
	}

	if (!is_registered) {
		logs::log(WARN, "Device was not properly registered! Can't track it!\n");
		return -1;
	}

	for (const auto &sensor : tracking_sensors_) {
		if (sensor->getName() == sensorName) {
			logs::log(WARN, "Device is already being tracked!!!\n");
			return -1;
		}
	}

	logs::log(INFO, "Now tracking |%s| sensor\n", sensorName.c_str());

	std::lock_guard<std::mutex> lock{ this->lock_ };

	tracking_sensors_.push_back(found);

	return 0;
}

int32_t SensorManager::stopTracking(std::string &sensorName)
{
	std::lock_guard<std::mutex> lock{ this->lock_ };

	auto deleted = std::remove_if(tracking_sensors_.begin(), tracking_sensors_.end(),
				      [sensorName](std::shared_ptr<sensor::Sensor> p) {
					      return p->getName() == sensorName;
				      });

	if (deleted == tracking_sensors_.end()) {
		logs::log(ERR, "Sensor requested was not being tracked!\n");
		return -1;
	}

	tracking_sensors_.erase(deleted, tracking_sensors_.end());

	return 0;
}

void logMeasurement(sensor::SensorDataEntry const &meas)
{
	logs::log(DEBUG, "Measurement of type [%s] from %s [%s] was %f\n",
		  meas.measurementType.c_str(), meas.sensorName.c_str(), meas.sensorType.c_str(),
		  meas.value);
}

int32_t SensorManager::setMeasurementPeriod(uint64_t period_ms)
{
	std::lock_guard<std::mutex> lock{ this->lock_ };

	meas_period_ms_ = period_ms;

	return 0;
}
