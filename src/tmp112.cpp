/* SPDX-License-Identifier: GPL-2.0-only */

#include <iio.h>
#include <stdexcept>
#include <string>

#include "sensor.hpp"
#include "tmp112.hpp"
#include "log.hpp"

Tmp112::Tmp112(struct iio_context *ctx, std::string name)
	: ctx_{ ctx }
{
	name_ = std::move(name);
	supported_types_ = { "temperature" };

	if (ctx_ == nullptr)
		throw std::runtime_error("context is null!");

	dev_ = iio_context_find_device(ctx_, name.c_str());

	if (dev_ == nullptr) {
		logs::log(ERR, "Failed to find device in context!\n");
		throw std::runtime_error("find iio device!");
	}

	ch_temp_ = iio_device_find_channel(dev_, "temp1", false);

	if (ch_temp_ == nullptr) {
		logs::log(ERR, "Failed to find temp1 channel!\n");
		throw std::runtime_error("find temp1");
	}
}

std::optional<sensor::SensorDataEntry> Tmp112::read(std::string const &sensorType)
{
	if (!ch_temp_)
		return std::nullopt;

	long long temp = 0;

	auto res = iio_channel_attr_read_longlong(ch_temp_, "input", &temp);

	if (res < 0) {
		logs::log(ERR, "Failed to read temp1 input attr!\n");
		return std::nullopt;
	}

	double read = static_cast<double>(static_cast<double>(temp) / 1000.0);

	sensor::SensorDataEntry entry{
		.sensorName = name_,
		.sensorType = "tmp112",
		.measurementType = "temp",
		.value = read,
	};

	return entry;
}
