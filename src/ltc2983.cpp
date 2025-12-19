#include <iio.h>

#include <memory>
#include <optional>
#include <ranges>
#include <read-sensors/ltc2983.hpp>
#include <read-sensors/sensor.hpp>
#include <utils/log/log.hpp>

namespace sensor {

Ltc2983::Ltc2983(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name) {
  supported_types_ = {"temperature"};
  name_ = name;
  channel_based_ = true;

  dev_ = std::make_unique<fsatutils::iio::Device>(ctx, name);
}

std::optional<std::vector<SensorDataEntry>> Ltc2983::read() {
  std::vector<SensorDataEntry> readings;

  for (auto& channel : channels_) {
    long long raw = 0;
    double scale = 0.0f;

    try {
      raw = channel->read_attr<long long>("raw");
      scale = channel->read_attr<double>("scale");
    } catch (std::runtime_error const& e) {
      logs::log(ERR, "Failed to read channel [%s]!\n", channel->name().c_str());
      continue;
    }

    double read = static_cast<double>(static_cast<int16_t>(raw) *
                                      static_cast<double>(scale));
    /* Convert to Celsius */
    read /= 1000.0;

    sensor::SensorDataEntry entry{
        .sensorName = name_,
        .sensorType = channel->name(),
        .measurementType = "temperature",
        .value = read,
    };

    readings.push_back(entry);
  }

  if (readings.empty()) return std::nullopt;

  return readings;
}

int Ltc2983::addChannel(std::string const& channel) {
  auto result =
      channels_ |
      std::views::filter([&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
        return channel == ch->name();
      });

  if (!result.empty()) {
    logs::log(WARN, "Channel [%s] was already registered!\n", channel.c_str());
    return 0;
  }

  try {
    channels_.push_back(std::make_unique<fsatutils::iio::Channel>(channel, *dev_, false));
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to create channel [%s]!\n", channel.c_str());
    return -1;
  }

  return 0;
}

int Ltc2983::removeChannel(std::string const& channel) {
  auto initial_size = channels_.size();

  std::erase_if(channels_, [&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
    return ch->name() == channel;
  });

  return (channels_.size() < initial_size) ? 0 : -1;
}

bool Ltc2983::hasChannel(std::string const& channel) {
  auto result =
      channels_ |
      std::views::filter([&channel](std::unique_ptr<fsatutils::iio::Channel> const& ch) {
        return channel == ch->name();
      });

  return !result.empty();
}

}  // namespace sensor
