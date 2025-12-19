/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef INA219_HPP_
#define INA219_HPP_

#include <memory>
#include <read-sensors/sensor.hpp>
#include <string>
#include <utils/iio/context.hpp>
#include <utils/iio/device.hpp>
#include <utils/iio/channel.hpp>

namespace sensor {

class Ina219 : public Sensor {
 public:
  Ina219(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name);
  std::optional<std::vector<SensorDataEntry>> read() override;

 private:
  std::optional<sensor::SensorDataEntry> get_power() const;
  std::optional<sensor::SensorDataEntry> get_current() const;
  std::optional<sensor::SensorDataEntry> get_voltage() const;
  std::unique_ptr<fsatutils::iio::Device> dev_;
  std::unique_ptr<fsatutils::iio::Channel> volt_;
  std::unique_ptr<fsatutils::iio::Channel> curr_;
  std::unique_ptr<fsatutils::iio::Channel> pwr_;
};

}  // namespace sensor

#endif
