/* SPDX-License-Identifier: GPL-2.0-only */

#include <iio.h>

#include <memory>
#include <read-sensors/sensor.hpp>
#include <read-sensors/tmp112.hpp>
#include <stdexcept>
#include <string>
#include <fsatutils/log/log.hpp>

namespace sensor {

Tmp112::Tmp112(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name) {
  supported_types_ = {"temperature"};
  name_ = name;
  channel_based_ = false;

  dev_ = std::make_unique<fsatutils::iio::Device>(ctx, name);
  temp_ = std::make_unique<fsatutils::iio::Channel>("temp1", *dev_, false);
}

std::optional<std::vector<SensorDataEntry>> Tmp112::read() {
  std::vector<SensorDataEntry> readings;
  long long temp;

  try {
    temp = temp_->read_attr<long long>("input");
  } catch (std::runtime_error const& e) {
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

  readings.push_back(entry);

  return readings;
}

}  // namespace sensor
