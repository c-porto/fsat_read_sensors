#include <iio.h>

#include <memory>
#include <optional>
#include <read-sensors/ina219.hpp>
#include <read-sensors/sensor.hpp>
#include <stdexcept>
#include <fsatutils/log/log.hpp>
#include <vector>

namespace sensor {

Ina219::Ina219(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name) {
  supported_types_ = {"voltage", "current", "power"};
  name_ = name;
  channel_based_ = false;

  dev_ = std::make_unique<fsatutils::iio::Device>(ctx, name);

  volt_ = std::make_unique<fsatutils::iio::Channel>("voltage1", *dev_, false);
  curr_ = std::make_unique<fsatutils::iio::Channel>("current1", *dev_, false);
  pwr_ = std::make_unique<fsatutils::iio::Channel>("power2", *dev_, false);
}

std::optional<std::vector<SensorDataEntry>> Ina219::read() {
  std::vector<SensorDataEntry> readings;

  auto entry = this->get_current();

  if (entry) {
    readings.push_back(*entry);
  }

  entry = this->get_voltage();

  if (entry) {
    readings.push_back(*entry);
  }

  entry = this->get_power();

  if (entry) {
    readings.push_back(*entry);
  }

  if (readings.empty()) return std::nullopt;

  return readings;
}

std::optional<sensor::SensorDataEntry> Ina219::get_power() const {
  long long raw = 0;
  double scale = 0.0f;

  try {
    raw = pwr_->read_attr<long long>("raw");
    scale = pwr_->read_attr<double>("scale");
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to read power2 input attr!\n");
    return std::nullopt;
  }

  double read = static_cast<double>(static_cast<int16_t>(raw) *
                                    static_cast<double>(scale));

  sensor::SensorDataEntry entry{
      .sensorName = name_,
      .sensorType = "ina219",
      .measurementType = "power",
      .value = read,
  };

  return entry;
}

std::optional<sensor::SensorDataEntry> Ina219::get_current() const {
  long long raw = 0;
  double scale = 0.0f;

  try {
    raw = curr_->read_attr<long long>("raw");
    scale = curr_->read_attr<double>("scale");
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to read current1 input attr!\n");
    return std::nullopt;
  }

  double read = static_cast<double>(static_cast<int16_t>(raw) *
                                    static_cast<double>(scale));

  sensor::SensorDataEntry entry{
      .sensorName = name_,
      .sensorType = "ina219",
      .measurementType = "current",
      .value = read,
  };

  return entry;
}

std::optional<sensor::SensorDataEntry> Ina219::get_voltage() const {
  long long raw = 0;
  double scale = 0.0f;

  try {
    raw = volt_->read_attr<long long>("raw");
    scale = volt_->read_attr<double>("scale");
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to read voltage1 input attr!\n");
    return std::nullopt;
  }

  double read = static_cast<double>(static_cast<int16_t>(raw) *
                                    static_cast<double>(scale));

  sensor::SensorDataEntry entry{
      .sensorName = name_,
      .sensorType = "ina219",
      .measurementType = "voltage",
      .value = read,
  };

  return entry;
}
}  // namespace sensor
