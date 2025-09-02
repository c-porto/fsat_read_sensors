/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef TMP112_HPP_
#define TMP112_HPP_

#include <iio.h>
#include <string>

#include "sensor.hpp"

class Tmp112 : public sensor::Sensor {
    public:
	Tmp112(struct iio_context *ctx, std::string name);
	std::optional<sensor::SensorDataEntry> read(std::string const &sensorType) override;

    private:
	struct iio_context *ctx_;
	struct iio_device *dev_;
	struct iio_channel *ch_temp_;
};

#endif
