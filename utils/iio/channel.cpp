#include <iio.h>

#include <array>
#include <string>
#include <utils/errors.hpp>
#include <utils/iio/channel.hpp>
#include <utils/log/log.hpp>

namespace fsatutils {

namespace iio {

Channel::Channel(std::string name, struct iio_device* device,
                 bool output_channel)
    : dev_{device}, name_{std::move(name)}, output_{output_channel} {
  raw_ = iio_device_find_channel(dev_, name_.c_str(), output_);

  if (raw_ == nullptr) {
    throw_runtime_error("Failed to find " + name_ + " IIO Channel!");
  }
}

template <>
void Channel::write_attr(std::string const& attr, long long const& value) {
  int res = iio_channel_attr_write_longlong(raw_, attr.c_str(), value);
  if (res < 0) {
    throw_runtime_error("Failed to write " + attr + " Channel attribute!");
  }
}

template <>
void Channel::write_attr(std::string const& attr, bool const& value) {
  int res = iio_channel_attr_write_bool(raw_, attr.c_str(), value);
  if (res < 0) {
    throw_runtime_error("Failed to write " + attr + " Channel attribute!");
  }
}

template <>
void Channel::write_attr(std::string const& attr, std::string const& value) {
  int res = iio_channel_attr_write(raw_, attr.c_str(), value.c_str());

  if ((res < 0) || (res != static_cast<int>(value.length() + 1))) {
    throw_runtime_error("Failed to write " + attr + " Channel attribute!");
  }
}

template <>
long long Channel::read_attr(std::string const& attr) const {
  long long val = 0;

  int res = iio_channel_attr_read_longlong(raw_, attr.c_str(), &val);

  if (res < 0) {
    throw_runtime_error("Failed to read " + attr + " Channel attribute!");
  }

  return val;
}

template <>
std::string Channel::read_attr(std::string const& attr) const {
  std::array<char, 1024U> buf;

  int res = iio_channel_attr_read(raw_, attr.c_str(), buf.data(), buf.size());

  if (res < 0) {
    throw_runtime_error("Failed to read " + attr + " Channel attribute!");
  }

  return {buf.data()};
}

template <>
bool Channel::read_attr(std::string const& attr) const {
  bool val;

  int res = iio_channel_attr_read_bool(raw_, attr.c_str(), &val);

  if (res < 0) {
    throw_runtime_error("Failed to read " + attr + " Channel attribute!");
  }

  return val;
}

template <>
double Channel::read_attr(std::string const& attr) const {
  double val;

  int res = iio_channel_attr_read_double(raw_, attr.c_str(), &val);

  if (res < 0) {
    throw_runtime_error("Failed to read " + attr + " Channel attribute!");
  }

  return val;
}

}  // namespace iio

}  // namespace fsatutils
