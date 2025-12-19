#ifndef READ_SENSORS_HPP_
#define READ_SENSORS_HPP_

#include <iio.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <read-sensors/db.hpp>
#include <read-sensors/ina219.hpp>
#include <read-sensors/ltc2983.hpp>
#include <read-sensors/sensor.hpp>
#include <read-sensors/tmp112.hpp>
#include <read-sensors/xadc.hpp>
#include <string>
#include <utils/cli/arg_handler.hpp>
#include <utils/iio/context.hpp>
#include <utils/log/log.hpp>
#include <utils/zmq/zprotocol.hpp>
#include <vector>

inline std::uint64_t strToU64(const char* str) {
  try {
    return static_cast<std::uint64_t>(std::stoull(str, nullptr, 10));
  } catch (const std::invalid_argument& e) {
    logs::log(ERR, "Invalid string to convert: %s\n", e.what());
    return 0;
  } catch (const std::out_of_range& e) {
    logs::log(ERR, "Out of Range to convert: %s\n", e.what());
    return 0;
  }
}

class SensorManager : fsatutils::ArgpModule<SensorManager> {
  struct ManagerCLI {
    std::string dbPath;
    fsatutils::iio::ContextType iioType;
    std::optional<std::string> iioUri;
  };

  class SensorFactory {
    using CreateFunction = std::function<std::shared_ptr<sensor::Sensor>(
        std::string const& sensorName, std::string const& sensorType)>;

   public:
    SensorFactory(std::shared_ptr<fsatutils::iio::Context> ctx) : ctx_{ctx} {
      registerCreator(
          "ina219",
          [this](std::string const& sensorName, std::string const& sensorType)
              -> std::shared_ptr<sensor::Sensor> {
            try {
              return std::make_shared<sensor::Ina219>(ctx_, sensorName);
            } catch (std::runtime_error const& e) {
              logs::log(ERR, "Failed to create sensor [%s] of type [%s]\n",
                        sensorName.c_str(), sensorType.c_str());
              return nullptr;
            }
          });

      registerCreator(
          "tmp112",
          [this](std::string const& sensorName, std::string const& sensorType)
              -> std::shared_ptr<sensor::Sensor> {
            try {
              return std::make_shared<sensor::Tmp112>(ctx_, sensorName);
            } catch (std::runtime_error const& e) {
              logs::log(ERR, "Failed to create sensor [%s] of type [%s]\n",
                        sensorName.c_str(), sensorType.c_str());
              return nullptr;
            }
          });

      registerCreator(
          "ltc2983",
          [this](std::string const& sensorName, std::string const& sensorType)
              -> std::shared_ptr<sensor::Sensor> {
            try {
              return std::make_shared<sensor::Ltc2983>(ctx_, sensorName);
            } catch (std::runtime_error const& e) {
              logs::log(ERR, "Failed to create sensor [%s] of type [%s]\n",
                        sensorName.c_str(), sensorType.c_str());
              return nullptr;
            }
          });

      registerCreator(
          "xadc",
          [this](std::string const& sensorName, std::string const& sensorType)
              -> std::shared_ptr<sensor::Sensor> {
            try {
              return std::make_shared<sensor::xadc>(ctx_, sensorName);
            } catch (std::runtime_error const& e) {
              logs::log(ERR, "Failed to create sensor [%s] of type [%s]\n",
                        sensorName.c_str(), sensorType.c_str());
              return nullptr;
            }
          });
    }
    void registerCreator(const std::string& sensorType,
                         CreateFunction creator) {
      registry_[sensorType] = creator;
    }

    std::shared_ptr<sensor::Sensor> create(
        std::string const& sensorName, std::string const& sensorType) const {
      if (auto it = registry_.find(sensorType); it != registry_.end()) {
        return it->second(sensorName, sensorType);
      }
      return nullptr;
    }

    bool canCreate(const std::string& type) const {
      return registry_.contains(type);
    }

   private:
    std::unordered_map<std::string, CreateFunction> registry_;
    std::shared_ptr<fsatutils::iio::Context> ctx_;
  };

 public:
  SensorManager(ManagerCLI& cli);
  void runManager(void);
  void stopManager(void);
  int trackRegisteredDevices(void);
  int setMeasurementPeriod(const uint64_t time_ms);
  int startTracking(std::string const& sensorName, std::string const& channel);
  int stopTracking(std::string const& sensorName, std::string const& channel);
  int registerDevice(std::string const& sensorName,
                     std::string const& sensorType);
  int unregisterDevice(std::string& sensorName);

  static void commandHandler(void* manager, fsatutils::zmq::Command cmd);
  static std::vector<fsatutils::zmq::Command> getCommandDescription();

  static std::vector<argp_child> get_argp_children();
  inline static ManagerCLI cli_config;

 private:
  void logMeasurement(sensor::SensorDataEntry const& meas);
  void readTrackedSensors(void);

  std::vector<std::shared_ptr<sensor::Sensor> > tracking_sensors_{};
  std::vector<std::shared_ptr<sensor::Sensor> > registered_sensors_{};
  std::mutex lock_;
  std::shared_ptr<fsatutils::iio::Context> ctx_;
  SqliteDb db_;
  std::unique_ptr<SensorFactory> factory_;
  uint64_t meas_period_ms_{60000ULL};
  bool running_{true};
};

#endif
