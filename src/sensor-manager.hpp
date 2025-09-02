#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <iio.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

#include "sensor.hpp"
#include "db.hpp"
#include "ina219.hpp"
#include "tmp112.hpp"
#include "log.hpp"

class SensorManager {
    public:
	SensorManager(std::string dbPath)
		: db_{ dbPath }
	{
		ctx_ = iio_create_default_context();

		if (ctx_) {
			logs::log(ERR, "Failed to create default IIO context!!\n");
			throw std::runtime_error("iio ctx");
		}

		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h1-45_46"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h1-49_50"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h1-47_48"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h1-51_52"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h2-35_36"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h2-25_26"));
		registered_sensors_.push_back(std::make_shared<Ina219>(ctx_, "h2-27_28"));
		registered_sensors_.push_back(std::make_shared<Tmp112>(ctx_, "ambient-temp"));
	}

	~SensorManager()
	{
		if (ctx_) {
			iio_context_destroy(ctx_);
		}
	}

	void runManager(void);
	int32_t trackRegisteredDevices(void);
	int32_t setMeasurementPeriod(const uint64_t time_ms);
	int32_t startTracking(std::string &sensorName);
	int32_t stopTracking(std::string &sensorName);

    private:
	void logMeasurement(sensor::SensorDataEntry const &meas);
	void readTrackedSensors(void);
	std::vector<std::shared_ptr<sensor::Sensor> > tracking_sensors_{};
	std::vector<std::shared_ptr<sensor::Sensor> > registered_sensors_{};
	std::mutex lock_;
	uint64_t meas_period_ms_{ 1000ULL };
	struct iio_context *ctx_;
	SqliteDb db_;
};

#endif
