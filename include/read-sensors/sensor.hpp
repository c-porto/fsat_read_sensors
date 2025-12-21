#ifndef SENSOR_HPP_
#define SENSOR_HPP_

#include <optional>
#include <string>
#include <vector>

namespace sensor {
struct SensorDataEntry {
  std::string sensorName;
  std::string sensorType;
  std::string measurementType;
  double value;
};

class Sensor {
 public:
  virtual ~Sensor() = default;
  virtual std::optional<std::vector<SensorDataEntry>> read() = 0;
  std::vector<std::string> supported_types_;
  std::string getName() const { return name_; }

  /* Channel based sensors */
  [[nodiscard]] bool isChannelBased() const { return channel_based_; }
  virtual int addChannel(std::string const& channel) {
    (void)channel;
    return -1;
  }
  virtual int removeChannel(std::string const& channel) {
    (void)channel;
    return -1;
  }
  virtual bool hasChannel(std::string const& channel) {
    (void)channel;
    return -1;
  }
  virtual int activeChannels() { return -1; }

 protected:
  std::string name_;
  bool channel_based_;
};

}  // namespace sensor

#endif
