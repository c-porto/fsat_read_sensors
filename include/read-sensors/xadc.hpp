#ifndef XADC_HPP_
#define XADC_HPP_

#include <memory>
#include <read-sensors/sensor.hpp>
#include <string>
#include <utils/iio/channel.hpp>
#include <utils/iio/context.hpp>
#include <utils/iio/device.hpp>

namespace sensor {

class xadc : public Sensor {
 public:
  xadc(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name);
  std::optional<std::vector<SensorDataEntry>> read() override;
  int addChannel(std::string const& channel) override;
  int removeChannel(std::string const& channel) override;
  bool hasChannel(std::string const& channel) override;
  int activeChannels() override { return channels_.size(); }

 private:
  std::unique_ptr<fsatutils::iio::Device> dev_;
  std::vector<std::unique_ptr<fsatutils::iio::Channel>> channels_;
};

}  // namespace sensor

#endif
