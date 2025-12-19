#ifndef DEVICE_HPP_
#define DEVICE_HPP_

#include <iio.h>

#include <memory>
#include <string>
#include <utils/iio/channel.hpp>
#include <utils/iio/context.hpp>

namespace fsatutils {

namespace iio {

class Device {
 public:
  Device(std::shared_ptr<Context> ctx, std::string name);

  Channel find_device_channel(std::string const& channel_name, bool output);

  std::string name() const noexcept { return name_; }

  operator struct iio_device*() { return raw_; };

 private:
  std::shared_ptr<Context> ctx_;
  struct iio_device* raw_;
  std::string name_;
};

}  // namespace iio

}  // namespace fsatutils

#endif
