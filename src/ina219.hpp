/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef INA219_HPP_
#define INA219_HPP_

#include <iio.h>
#include <string>

#include "sensor.hpp"

class Ina219 : public sensor::Sensor {
    public:
	Ina219(struct iio_context *ctx, std::string name);
	std::optional<sensor::SensorDataEntry> read(std::string const &sensorType) override;

    private:
	std::optional<sensor::SensorDataEntry> get_power() const;
	std::optional<sensor::SensorDataEntry> get_current() const;
	std::optional<sensor::SensorDataEntry> get_voltage() const;
	struct iio_context *ctx_;
	struct iio_device *dev_;
	struct iio_channel *ch_volt_;
	struct iio_channel *ch_curr_;
	struct iio_channel *ch_pwr_;
};

#endif
