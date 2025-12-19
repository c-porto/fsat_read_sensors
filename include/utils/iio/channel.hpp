#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include <iio.h>

#include <string>

namespace fsatutils {

namespace iio {

class Channel {
 public:
  Channel(std::string name, struct iio_device* device, bool output_channel);

  template <typename AttrType>
  void write_attr(std::string const& attr, AttrType const& value);
  template <typename AttrType>
  AttrType read_attr(std::string const& attr) const;

  std::string name() const noexcept { return name_; }

  operator struct iio_channel*() { return raw_; };

 private:
  struct iio_channel* raw_;
  struct iio_device* dev_;
  std::string name_;
  bool output_;
};

}  // namespace iio

}  // namespace fsatutils

#endif
