/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef TMP112_HPP_
#define TMP112_HPP_

#include <memory.h>

#include <read-sensors/sensor.hpp>
#include <string>
#include <fsatutils/iio/channel.hpp>
#include <fsatutils/iio/context.hpp>
#include <fsatutils/iio/device.hpp>

namespace sensor {

class Tmp112 : public Sensor {
 public:
  Tmp112(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name);
  std::optional<std::vector<SensorDataEntry>> read() override;

 private:
  std::unique_ptr<fsatutils::iio::Device> dev_;
  std::unique_ptr<fsatutils::iio::Channel> temp_;
};

}  // namespace sensor

#endif
