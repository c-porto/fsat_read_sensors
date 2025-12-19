#include <iio.h>

#include <memory>
#include <optional>
#include <ranges>
#include <read-sensors/sensor.hpp>
#include <read-sensors/xadc.hpp>
#include <utils/log/log.hpp>

namespace sensor {

xadc::xadc(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name) {
  supported_types_ = {"voltage"};
  name_ = name;
  channel_based_ = true;

  dev_ = std::make_unique<fsatutils::iio::Device>(ctx, name);
}

std::optional<std::vector<SensorDataEntry>> xadc::read() {
  std::vector<SensorDataEntry> readings;

  for (auto& channel : channels_) {
    long long raw = 0;
    long long offset = 0;
    double scale = 0.0f;

    try {
      raw = channel->read_attr<long long>("raw");
      scale = channel->read_attr<double>("scale");
    } catch (std::runtime_error const& e) {
      logs::log(ERR, "Failed to read channel [%s]!\n", channel->name().c_str());
      continue;
    }

    double read = static_cast<double>(static_cast<int16_t>(raw) *
                                      static_cast<double>(scale)) +
                  static_cast<double>(offset);

    if (channel->name() == "temp0") {
      try {
        offset = channel->read_attr<long long>("offset");
      } catch (std::runtime_error const& e) {
        logs::log(ERR, "Failed to read channel [%s]!\n",
                  channel->name().c_str());
        continue;
      }

      read += static_cast<double>(offset);

      read /= 1000.0;

      sensor::SensorDataEntry entry{
          .sensorName = name_,
          .sensorType = channel->name(),
          .measurementType = "temperature",
          .value = read,
      };

      readings.push_back(entry);

    } else {
      sensor::SensorDataEntry entry{
          .sensorName = name_,
          .sensorType = channel->name(),
          .measurementType = "voltage",
          .value = read,
      };

      readings.push_back(entry);
    }
  }

  if (readings.empty()) return std::nullopt;

  return readings;
}

int xadc::addChannel(std::string const& channel) {
  auto result =
      channels_ |
      std::views::filter(
          [&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
            return channel == ch->name();
          });

  if (!result.empty()) {
    logs::log(WARN, "Channel [%s] was already registered!\n", channel.c_str());
    return 0;
  }

  try {
    channels_.push_back(
        std::make_unique<fsatutils::iio::Channel>(channel, *dev_, false));
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to create channel [%s]!\n", channel.c_str());
    return -1;
  }

  logs::log(INFO, "Channel [%s] has been added to device [%s]!\n",
            channel.c_str(), name_.c_str());

  return 0;
}

int xadc::removeChannel(std::string const& channel) {
  auto initial_size = channels_.size();

  std::erase_if(channels_,
                [&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
                  return ch->name() == channel;
                });

  return (channels_.size() < initial_size) ? 0 : -1;
}

bool xadc::hasChannel(std::string const& channel) {
  auto result =
      channels_ |
      std::views::filter(
          [&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
            return channel == ch->name();
          });

  return !result.empty();
}

}  // namespace sensor
