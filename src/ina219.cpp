#include <functional>
#include <iio.h>
#include <optional>
#include <stdexcept>
#include <unordered_map>

#include "sensor.hpp"
#include "log.hpp"
#include "ina219.hpp"

Ina219::Ina219(struct iio_context *ctx, std::string name)
	: ctx_{ ctx }
{
	name_ = std::move(name);
	supported_types_ = { "voltage", "current", "power" };

	if (ctx_ == nullptr)
		throw std::runtime_error("context is null!");

	dev_ = iio_context_find_device(ctx_, name.c_str());

	if (dev_ == nullptr) {
		logs::log(ERR, "Failed to find device [%s] in context!\n", name.c_str());
		throw std::runtime_error("find iio device!");
	}

	ch_volt_ = iio_device_find_channel(dev_, "voltage1", false);

	if (ch_volt_ == nullptr) {
		logs::log(ERR, "Failed to find voltage1 channel!\n");
		throw std::runtime_error("find voltage1");
	}

	ch_curr_ = iio_device_find_channel(dev_, "current3", false);

	if (ch_curr_ == nullptr) {
		logs::log(ERR, "Failed to find current3 channel!\n");
		throw std::runtime_error("find current3");
	}

	ch_pwr_ = iio_device_find_channel(dev_, "power2", false);

	if (ch_pwr_ == nullptr) {
		logs::log(ERR, "Failed to find power2 channel!\n");
		throw std::runtime_error("find power2");
	}
}

std::optional<sensor::SensorDataEntry> Ina219::read(std::string const &sensorType)
{
	static const std::unordered_map<
		std::string, std::function<std::optional<sensor::SensorDataEntry>(const Ina219 &)> >
		type_map = {
			{ "voltage", &Ina219::get_voltage },
			{ "current", &Ina219::get_current },
			{ "power", &Ina219::get_power },
		};

	auto it = type_map.find(sensorType);

	if (it != type_map.end()) {
		return it->second(*this);
	}

	return std::nullopt;
}

std::optional<sensor::SensorDataEntry> Ina219::get_power() const
{
	if (!ch_pwr_)
		return std::nullopt;

	long long raw = 0;
	double scale = 0.0f;

	auto res = iio_channel_attr_read_longlong(ch_pwr_, "raw", &raw);

	if (res < 0) {
		logs::log(ERR, "Failed to read power2 raw attr!\n");
		return std::nullopt;
	}

	res = iio_channel_attr_read_double(ch_pwr_, "scale", &scale);

	if (res < 0) {
		logs::log(ERR, "Failed to read power2 scale attr!\n");
		return std::nullopt;
	}

	double read = static_cast<double>(static_cast<int16_t>(raw) * static_cast<double>(scale));

	sensor::SensorDataEntry entry{
		.sensorName = name_,
		.sensorType = "ina219",
		.measurementType = "power",
		.value = read,
	};

	return entry;
}

std::optional<sensor::SensorDataEntry> Ina219::get_current() const
{
	if (!ch_curr_)
		return std::nullopt;

	long long raw = 0;
	double scale = 0.0f;

	auto res = iio_channel_attr_read_longlong(ch_curr_, "raw", &raw);

	if (res < 0) {
		logs::log(ERR, "Failed to read current3 raw attr!\n");
		return std::nullopt;
	}

	res = iio_channel_attr_read_double(ch_curr_, "scale", &scale);

	if (res < 0) {
		logs::log(ERR, "Failed to read current3 scale attr!\n");
		return std::nullopt;
	}

	double read = static_cast<double>(static_cast<int16_t>(raw) * static_cast<double>(scale));

	sensor::SensorDataEntry entry{
		.sensorName = name_,
		.sensorType = "ina219",
		.measurementType = "current",
		.value = read,
	};

	return entry;
}

std::optional<sensor::SensorDataEntry> Ina219::get_voltage() const
{
	if (!ch_volt_)
		return std::nullopt;

	long long raw = 0;
	double scale = 0.0f;

	auto res = iio_channel_attr_read_longlong(ch_volt_, "raw", &raw);

	if (res < 0) {
		logs::log(ERR, "Failed to read voltage1 raw attr!\n");
		return std::nullopt;
	}

	res = iio_channel_attr_read_double(ch_volt_, "scale", &scale);

	if (res < 0) {
		logs::log(ERR, "Failed to read voltage1 scale attr!\n");
		return std::nullopt;
	}

	double read = static_cast<double>(static_cast<int16_t>(raw) * static_cast<double>(scale));

	sensor::SensorDataEntry entry{
		.sensorName = name_,
		.sensorType = "ina219",
		.measurementType = "voltage",
		.value = read,
	};

	return entry;
}
