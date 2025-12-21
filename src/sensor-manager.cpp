#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <read-sensors/ltc2983.hpp>
#include <read-sensors/sensor-manager.hpp>
#include <read-sensors/sensor.hpp>
#include <thread>
#include <utils/log/log.hpp>
#include <utils/zmq/zprotocol.hpp>
#include <vector>

namespace {
constexpr int ARGP_KEY_IIO_TYPE = 0xFF;
constexpr int ARGP_KEY_IIO_URI = 0xFE;
constexpr int ARGP_KEY_DB_PATH = 0xFD;

constexpr struct argp_option options[] = {
    {.name = "ctx-type",
     .key = ARGP_KEY_IIO_TYPE,
     .arg = "type",
     .flags = 0,
     .doc = "IIO Context available types: local, network",
     .group = 0},
    {.name = "ctx-host",
     .key = ARGP_KEY_IIO_URI,
     .arg = "host",
     .flags = 0,
     .doc = "Host used for network context: 192.168.99.1, flatsat2.local",
     .group = 0},
    {.name = "sqlite-path",
     .key = ARGP_KEY_DB_PATH,
     .arg = "path",
     .flags = 0,
     .doc = "Database path: /var/local/read-sensors.sqlite3",
     .group = 0},
    {.name = nullptr,
     .key = 0,
     .arg = nullptr,
     .flags = 0,
     .doc = nullptr,
     .group = 0},
};

constexpr argp man_argp = {
    .options = options,
    .parser = [](int key, char* arg, argp_state* state) -> int {
      (void)state;
      switch (key) {
        case ARGP_KEY_DB_PATH:
          if (arg) SensorManager::cli_config.dbPath.assign(arg);
          break;
        case ARGP_KEY_IIO_TYPE:
          if (arg) {
            if (strcasecmp("local", arg) == 0) {
              SensorManager::cli_config.iioType =
                  fsatutils::iio::ContextType::LOCAL;
            } else if (strcasecmp("network", arg) == 0) {
              SensorManager::cli_config.iioType =
                  fsatutils::iio::ContextType::NETWORK;
            } else {
              SensorManager::cli_config.iioType =
                  fsatutils::iio::ContextType::DEFAULT;
              logs::log(WARN, "Invalid context type! Using DEFAULT...\n");
            }
          }
          break;
        case ARGP_KEY_IIO_URI:
          if (arg) SensorManager::cli_config.iioUri = std::string{arg};
          break;
        default:
          return ARGP_ERR_UNKNOWN;
      }

      return 0;
    },
    .args_doc = nullptr,
    .doc = "Sensor Manager CLI options",
    .children = nullptr,
    .help_filter = nullptr,
    .argp_domain = nullptr,
};
}  // namespace

std::vector<argp_child> SensorManager::get_argp_children() {
  return std::vector<argp_child>{
      {.argp = &man_argp, .flags = 0, .header = "Sensor Manager", .group = 0}};
}

std::vector<fsatutils::zmq::Command> SensorManager::getCommandDescription() {
  fsatutils::zmq::Command set_period;
  set_period.cmd = "set_measurement_period";
  set_period.args = {{
      .name = "period (ms)",
      .value = "",
      .type = fsatutils::zmq::ArgType::UINT64,
      .optional = false,
  }};

  fsatutils::zmq::Command reg;
  reg.cmd = "register";
  reg.args = {{
                  .name = "sensor name",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              },
              {
                  .name = "sensor type",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              }};

  fsatutils::zmq::Command unreg;
  unreg.cmd = "unregister";
  unreg.args = {
      {
          .name = "sensor name",
          .value = "",
          .type = fsatutils::zmq::ArgType::STRING,
          .optional = false,
      },
  };

  fsatutils::zmq::Command track;
  reg.cmd = "track";
  reg.args = {{
                  .name = "sensor name",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              },
              {
                  .name = "channel",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              }};

  fsatutils::zmq::Command untrack;
  reg.cmd = "untrack";
  reg.args = {{
                  .name = "sensor name",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              },
              {
                  .name = "channel",
                  .value = "",
                  .type = fsatutils::zmq::ArgType::STRING,
                  .optional = false,
              }};

  return {set_period, reg, unreg, track, untrack};
}

void SensorManager::commandHandler(void* manager, fsatutils::zmq::Command cmd) {
  SensorManager* man = static_cast<SensorManager*>(manager);

  if (cmd.cmd == "set_measurement_period") {
    uint64_t new_period = strToU64(cmd.args[0].value.c_str());
    man->setMeasurementPeriod(new_period);
  } else if (cmd.cmd == "register") {
    std::string sensorName;
    std::string sensorType;
    for (const auto& arg : cmd.args) {
      if (arg.name == "sensor name") {
        sensorName = arg.value;
      } else if (arg.name == "sensor type") {
        sensorType = arg.value;
      }
    }
    man->registerDevice(sensorName, sensorType);
  } else if (cmd.cmd == "unregister") {
    std::string sensorName;
    for (const auto& arg : cmd.args) {
      if (arg.name == "sensor name") {
        sensorName = arg.value;
      }
    }
    man->unregisterDevice(sensorName);
  } else if (cmd.cmd == "track") {
    std::string sensorName;
    std::string channel;
    for (const auto& arg : cmd.args) {
      if (arg.name == "sensor name") {
        sensorName = arg.value;
      } else if (arg.name == "channel") {
        channel = arg.value;
      }
    }
    man->startTracking(sensorName, channel);
  } else if (cmd.cmd == "untrack") {
    std::string sensorName;
    std::string channel;
    for (const auto& arg : cmd.args) {
      if (arg.name == "sensor name") {
        sensorName = arg.value;
      } else if (arg.name == "channel") {
        channel = arg.value;
      }
    }
    man->stopTracking(sensorName, channel);
  }
}

void SensorManager::readTrackedSensors(void) {
  std::lock_guard<std::mutex> lock{this->lock_};
  for (const auto& sensor : tracking_sensors_) {
    std::optional<std::vector<sensor::SensorDataEntry>> readings =
        sensor->read();

    if (readings) {
      for (const auto& read : *readings) {
        db_.addMeasurementToDB(read);
        this->logMeasurement(read);
      }
    }
  }
}

SensorManager::SensorManager(ManagerCLI& cli) : db_{cli.dbPath} {
  if (cli.iioUri) {
    ctx_ = std::make_shared<fsatutils::iio::Context>(cli.iioType, *cli.iioUri);
  } else {
    ctx_ = std::make_shared<fsatutils::iio::Context>(cli.iioType);
  }

  factory_ = std::make_unique<SensorFactory>(ctx_);

  /* Register FlatSat2 Devices */
  registerDevice("xadc", "xadc");
  registerDevice("h1-45_46", "ina219");
  registerDevice("h1-49_50", "ina219");
  registerDevice("h1-47_48", "ina219");
  registerDevice("h1-51_52", "ina219");
  registerDevice("h2-35_36", "ina219");
  registerDevice("h2-25_26", "ina219");
  registerDevice("h2-27_28", "ina219");
  registerDevice("ambient-temp", "tmp112");

  /* Track XADC Channels */
  startTracking("xadc", "temp0");
  startTracking("xadc", "voltage0");
  startTracking("xadc", "voltage1");
  startTracking("xadc", "voltage2");
  startTracking("xadc", "voltage3");
  startTracking("xadc", "voltage4");
  startTracking("xadc", "voltage5");
  startTracking("xadc", "voltage6");
  startTracking("xadc", "voltage7");
}

void SensorManager::runManager(void) {
  while (true) {
    auto next = std::chrono::steady_clock::now();

    if (!tracking_sensors_.empty()) {
      this->readTrackedSensors();
    }

    std::this_thread::sleep_until(next +
                                  std::chrono::milliseconds(meas_period_ms_));
  }
}

int SensorManager::trackRegisteredDevices(void) {
  logs::log(INFO, "Start tracking register devices...\n");

  for (const auto& sensor : registered_sensors_)
    tracking_sensors_.push_back(sensor);

  return 0;
}

int SensorManager::startTracking(std::string const& sensorName,
                                 std::string const& channel) {
  int err = 0;

  auto it = std::ranges::find_if(
      registered_sensors_, [sensorName](std::shared_ptr<sensor::Sensor> p) {
        return p->getName() == sensorName;
      });

  if (it == registered_sensors_.end()) {
    logs::log(WARN, "Device was not properly registered! Can't track it!\n");
    return -1;
  }

  auto tracked = std::ranges::find_if(
      tracking_sensors_, [sensorName](std::shared_ptr<sensor::Sensor> p) {
        return p->getName() == sensorName;
      });

  if (tracked != tracking_sensors_.end()) {
    logs::log(WARN, "Device is already being tracked!\n");

    err = -1;

    if ((*tracked)->isChannelBased()) {
      std::lock_guard<std::mutex> lock{this->lock_};
      err = (*tracked)->addChannel(channel);
    }
  } else {
    logs::log(INFO, "Now tracking [%s] sensor\n", sensorName.c_str());

    std::lock_guard<std::mutex> lock{this->lock_};

    tracking_sensors_.push_back(*it);

    if ((*it)->isChannelBased()) {
      err = (*it)->addChannel(channel);
    }
  }

  return err;
}

int SensorManager::stopTracking(std::string const& sensorName,
                                std::string const& channel) {
  std::lock_guard<std::mutex> lock{this->lock_};

  auto it = std::ranges::find_if(
      tracking_sensors_, [sensorName](std::shared_ptr<sensor::Sensor> p) {
        return p->getName() == sensorName;
      });

  if (it == tracking_sensors_.end()) {
    logs::log(ERR, "Sensor requested was not being tracked!\n");
    return -1;
  }

  auto sensor = *it;

  if (sensor->isChannelBased()) {
    if (!sensor->hasChannel(channel)) {
      logs::log(ERR, "Sensor does not have channel [%s] registered!\n",
                channel.c_str());
      return -1;
    }

    sensor->removeChannel(channel);

    logs::log(INFO, "Sensor [%s] now have [%d] channels registered!\n",
              sensor->getName().c_str(), sensor->activeChannels());
  } else {
    tracking_sensors_.erase(it);
  }

  return 0;
}

void SensorManager::logMeasurement(sensor::SensorDataEntry const& meas) {
  logs::log(DEBUG, "Measurement of type [%s] from %s [%s] was %f\n",
            meas.measurementType.c_str(), meas.sensorName.c_str(),
            meas.sensorType.c_str(), meas.value);
}

int SensorManager::setMeasurementPeriod(uint64_t period_ms) {
  std::lock_guard<std::mutex> lock{this->lock_};

  meas_period_ms_ = period_ms;

  return 0;
}

int SensorManager::registerDevice(std::string const& sensorName,
                                  std::string const& sensorType) {
  if (!factory_->canCreate(sensorType)) {
    logs::log(ERR, "Device is not supported!\n");
    return -1;
  }

  auto it = std::ranges::find_if(
      registered_sensors_, [sensorName](std::shared_ptr<sensor::Sensor> p) {
        return p->getName() == sensorName;
      });

  if (it != registered_sensors_.end()) {
    logs::log(WARN, "Device was already registered!\n");
    return -1;
  }

  auto sensor = factory_->create(sensorName, sensorType);

  if (sensor == nullptr) {
    logs::log(ERR, "Failed to create device!\n");
    return -1;
  }

  std::lock_guard<std::mutex> lock{this->lock_};

  registered_sensors_.push_back(sensor);

  logs::log(INFO, "Sensor [%s] was added to Registered list!\n",
            sensorName.c_str());

  return 0;
}

int SensorManager::unregisterDevice(std::string& sensorName) {
  std::lock_guard<std::mutex> lock{this->lock_};

  auto tracked = tracking_sensors_.size();
  auto registered = registered_sensors_.size();

  std::erase_if(registered_sensors_,
                [sensorName](std::shared_ptr<sensor::Sensor> p) {
                  return p->getName() == sensorName;
                });

  std::erase_if(tracking_sensors_,
                [sensorName](std::shared_ptr<sensor::Sensor> p) {
                  return p->getName() == sensorName;
                });

  if (tracked < tracking_sensors_.size()) {
    logs::log(WARN, "Sensor [%s] was removed from Tracking list!\n",
              sensorName.c_str());
  }

  if (registered < registered_sensors_.size()) {
    logs::log(INFO, "Sensor [%s] was removed from Registered list!\n",
              sensorName.c_str());
  }

  return 0;
}
