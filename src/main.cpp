#include <fsatutils/cli/arg_handler.hpp>
#include <fsatutils/iio/context.hpp>
#include <fsatutils/log/log.hpp>
#include <fsatutils/zmq/service.hpp>
#include <memory>
#include <read-sensors/sensor-manager.hpp>

#define DB_PATH "/experiment/read-sensors.sqlite3"

#include "version.hpp"

using namespace fsatutils;

int main(int argc, char** argv) {
  /* Defaults */
  SensorManager::cli_config.iioType = iio::ContextType::DEFAULT;
  SensorManager::cli_config.dbPath = DB_PATH;

  ArgpHandler::Config config{};

  config.program_name = "read-sensors";
  config.doc = "Service to manage FlatSat2 sensors";
  config.flags = 0;
  config.parser = nullptr;
  config.program_version = PROJECT_VERSION;

  ArgpHandler handler(config);

  handler.add_child_structures(SensorManager::get_argp_children());

  int result = handler.parse(argc, argv);

  if (result != 0) {
    logs::log(ERR, "Failed to parse command line arguments! Terminating...");
    exit(1);
  }

  if (logs::logFile.empty()) {
    logs::init(logs::LOG_DIR);
  }

  std::shared_ptr<SensorManager> man =
      std::make_shared<SensorManager>(SensorManager::cli_config);

  auto manager_cmds = SensorManager::getCommandDescription();

  zmq::Service::ServiceDescription desc = {
      .name = "read-sensors",
      .version = PROJECT_VERSION,
      .compatibleProtocols =
          static_cast<std::uint8_t>(zmq::MessageProtocol::JSON),
      .preferedProtocol = static_cast<std::uint8_t>(zmq::MessageProtocol::JSON),
  };

  std::unique_ptr<zmq::Service> service;

  try {
    service = std::make_unique<zmq::Service>(desc);
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Failed to create ZMQ service: [%s]!\n", e.what());
  }

  if (service != nullptr) {
    for (auto const& cmd : manager_cmds) {
      service->registerCommand(cmd.cmd, cmd.args,
                               &SensorManager::commandHandler,
                               static_cast<void*>(man.get()));
    }

    service->runService();
  }

  man->runManager();

  return 0;
}
