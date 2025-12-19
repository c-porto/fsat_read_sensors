#include <iio.h>

#include <memory>
#include <string>
#include <utils/errors.hpp>
#include <utils/iio/device.hpp>

namespace fsatutils {

namespace iio {

Device::Device(std::shared_ptr<Context> ctx, std::string name)
    : ctx_{ctx}, name_{std::move(name)} {
  raw_ = iio_context_find_device(*ctx_, name_.c_str());

  if (raw_ == nullptr) {
    throw_runtime_error("Failed to create " + name_ + " IIO Device!");
  }
}

Channel Device::find_device_channel(std::string const& channel_name,
                                    bool output) {
  auto ch = iio_device_find_channel(raw_, channel_name.c_str(), output);

  if (ch == nullptr) {
    throw_runtime_error("Failed to find " + channel_name + " IIO Channel!");
  }

  return {channel_name, raw_, output};
}

}  // namespace iio

}  // namespace fsatutils
