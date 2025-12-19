#include <zmq.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <utils/errors.hpp>
#include <utils/log/log.hpp>
#include <utils/zmq/service.hpp>
#include <utils/zmq/zprotocol.hpp>
#include <variant>

namespace fsatutils {

namespace zmq {

struct ZMQEngine {
  void* ctx;
  void* sub;
  void* pub;
};

class Service::impl {
  struct RegistryData {
    std::vector<CommandArg> args;
    std::vector<std::pair<CommandHandlerFn, void*>> handlers;
  };

 public:
  impl(ServiceDescription desc);

  void runService();
  void stopService();

  void cleanResources();

  bool registerCommand(CommandType command, std::vector<CommandArg> args,
                       std::optional<CommandHandlerFn> handler,
                       std::optional<void*> handlerData);

  bool registerHandler(CommandType& command, Service::CommandHandlerFn handler,
                       void* handlerData);

  void workTask(std::stop_token token);

 private:
  std::variant<std::monostate, Command, DiscoverMsgHeader> parseMessage(
      std::span<const uint8_t> msg);

  bool runCommandHandler(Command cmd);

  std::vector<char> serializeServiceDescription();

  bool connectToEngineProxy();

  ServiceDescription desc_;
  ZMQEngine engine_;
  std::unordered_map<CommandType, RegistryData> command_registry_;
  std::jthread work_thread_;
};

Service::Service(ServiceDescription desc)
    : impl_{std::make_unique<Service::impl>(desc)} {}

Service::~Service() { impl_->cleanResources(); }

void Service::runService() { impl_->runService(); }

void Service::stopService() { impl_->stopService(); }

Service& Service::registerCommand(CommandType command,
                                  std::vector<CommandArg> args,
                                  std::optional<CommandHandlerFn> handler,
                                  std::optional<void*> handlerData) {
  if (!impl_->registerCommand(command, args, handler, handlerData)) {
    logs::log(ERR, "Failed to register command [%s]\n", command.c_str());
  }

  return *this;
}

bool Service::registerHandler(CommandType& command,
                              Service::CommandHandlerFn handler,
                              void* handlerData) {
  return impl_->registerHandler(command, handler, handlerData);
}

Service::impl::impl(ServiceDescription desc) : desc_{std::move(desc)} {
  if (!connectToEngineProxy()) {
    throw_runtime_error("Failed to connect to FlatSat2 ZMQ Engine!");
  }
}

void Service::impl::runService() {
  std::ofstream ofs;
  pid_t pid = getpid();

  ofs.open("/run/read-sensors/read-sensors.pid",
           std::ios::out | std::ios::trunc);
  ofs << pid;
  ofs.close();

  work_thread_ =
      std::jthread{[this](std::stop_token stoken) { this->workTask(stoken); }};
}

void Service::impl::stopService() {
  work_thread_.request_stop();

  if (work_thread_.joinable()) {
    work_thread_.join();
  }
}

void Service::impl::cleanResources() {
  if (engine_.pub != nullptr) {
    zmq_close(engine_.pub);
  }

  if (engine_.sub != nullptr) {
    zmq_close(engine_.pub);
  }

  if (engine_.ctx != nullptr) {
    zmq_ctx_destroy(engine_.ctx);
  }

  stopService();
}

void Service::impl::workTask(std::stop_token stoken) {
  while (!stoken.stop_requested()) {
    zmq_msg_t msg;

    if (zmq_msg_init_size(&msg, ZMQ_FLATSAT_ENGINE_MTU) < 0) {
      logs::log(ERR, "Error message buffer [%s]\n", zmq_strerror(zmq_errno()));
      continue;
    }

    if (zmq_msg_recv(&msg, engine_.sub, 0) < 0) {
      logs::log(ERR, "Error recv data [%s]\n", zmq_strerror(zmq_errno()));
      continue;
    }

    std::span<uint8_t> m{static_cast<uint8_t*>(zmq_msg_data(&msg)),
                         zmq_msg_size(&msg)};

    std::variant<std::monostate, Command, DiscoverMsgHeader> request =
        parseMessage(m);

    if (std::holds_alternative<std::monostate>(request)) {
      logs::log(ERR, "Failed to parse message!");
      zmq_msg_close(&msg);
      continue;
    }

    if (std::holds_alternative<DiscoverMsgHeader>(request)) {
      logs::log(INFO, "Discover request received! Sending service details...");

      auto res = serializeServiceDescription();

      if (zmq_send(engine_.pub, res.data(), res.size(), 0U) < 0) {
        logs::log(
            ERR,
            "Failed to send service data as response to discover request!");
      }
    }

    if (std::holds_alternative<Command>(request)) {
      auto command = std::get<Command>(request);

      if (!runCommandHandler(command)) {
        logs::log(ERR, "Failed to run command handler!");
      }
    }
    zmq_msg_close(&msg);
  }
}

std::variant<std::monostate, Command, DiscoverMsgHeader>
Service::impl::parseMessage(std::span<const uint8_t> msg) {
  if (msg.size() < 4U) {
    logs::log(ERR, "Message was too short to be parsed!");
    return std::monostate{};
  }

  /* Check the subscribed topic of the message */
  if (memcmp(msg.data(), g_discoverTopic.data(), 4U) == 0) {
    auto header = msg.subspan(4U, msg.size() - 4U);
    return DiscoverMsgHeader{.version = header[0]};
  }

  if (memcmp(msg.data(), desc_.name.c_str(), desc_.name.size()) == 0) {
    auto raw_header = msg.subspan(desc_.name.size(), sizeof(CommandMsgHeader));
    auto payload =
        msg.last(msg.size() - desc_.name.size() - sizeof(CommandMsgHeader));

    CommandMsgHeader header = {
        .version = raw_header[0],
        .proto = static_cast<MessageProtocol>(raw_header[1]),
    };

    switch (header.proto) {
      case MessageProtocol::BINARY: {
        return std::monostate{};
      }
      case MessageProtocol::JSON: {
        auto parsed_cmd = parseJSON(payload);
        if (parsed_cmd.has_value()) {
          return parsed_cmd.value();
        } else {
          return std::monostate{};
        }
      }
      case MessageProtocol::PROTOBUF: {
        return std::monostate{};
      }
      default:
        throw_runtime_error("Unknown message protocol!");
    }
  }

  throw_runtime_error("Message with an unsubscribed topic was received!");
}

bool Service::impl::runCommandHandler(Command cmd) {
  if (!command_registry_.contains(cmd.cmd)) {
    logs::log(ERR, "Service does not suppport command [%s]!\n",
              cmd.cmd.c_str());
    return false;
  }

  auto& cmdData = command_registry_[cmd.cmd];

  for (auto& handler : cmdData.handlers) {
    if (handler.first != nullptr) {
      handler.first(handler.second, cmd);
    }
  }

  return true;
}

std::vector<char> Service::impl::serializeServiceDescription() {
  using json = nlohmann::json;

  json j;

  j["name"] = desc_.name;
  j["version"] = desc_.version;
  j["compatible_protocols"] =
      protoToString(static_cast<MessageProtocol>(desc_.compatibleProtocols));

  json cmd_array = json::array();

  for (const auto& cmd : command_registry_) {
    json c;
    json arg_array = json::array();

    c["name"] = cmd.first;

    for (const auto& arg : cmd.second.args) {
      json a;

      a["name"] = arg.name;
      a["type"] = typeToString(arg.type);
      a["optional"] = arg.optional;

      arg_array.push_back(a);
    }

    c["args"] = arg_array;
  }

  j["commands"] = cmd_array;

  std::string json_str = j.dump();

  return {json_str.begin(), json_str.end()};
}

bool Service::impl::connectToEngineProxy() {
  engine_.ctx = zmq_ctx_new();

  if (engine_.ctx == nullptr) {
    logs::log(ERR, "Failed to create zmq context!\n");
    return false;
  }

  engine_.pub = zmq_socket(engine_.ctx, ZMQ_PUB);

  if (engine_.pub == nullptr) {
    logs::log(ERR, "Failed to create zmq publisher!\n");
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  engine_.sub = zmq_socket(engine_.ctx, ZMQ_SUB);

  if (engine_.sub == nullptr) {
    logs::log(ERR, "Failed to create zmq subscribe!\n");
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  std::string xsub = "tcp://*:" + std::to_string(ZMQ_FLATSAT_ENGINE_XSUB_PORT);

  if (zmq_connect(engine_.pub, xsub.c_str()) != 0) {
    logs::log(ERR, "Failed to connect to engine xsub!\n");
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  std::string xpub = "tcp://*:" + std::to_string(ZMQ_FLATSAT_ENGINE_XPUB_PORT);

  if (zmq_connect(engine_.sub, xpub.c_str()) != 0) {
    logs::log(ERR, "Failed to connect to engine xpub!\n");
    zmq_close(engine_.pub);
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  if (zmq_setsockopt(engine_.sub, ZMQ_SUBSCRIBE, desc_.name.c_str(),
                     desc_.name.size()) != 0) {
    logs::log(ERR, "Failed to subscribe to service name!\n");
    zmq_close(engine_.sub);
    zmq_close(engine_.pub);
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  if (zmq_setsockopt(engine_.sub, ZMQ_SUBSCRIBE, g_discoverTopic.data(), 4U) !=
      0) {
    logs::log(ERR, "Failed to subscribe to discover command!\n");
    zmq_close(engine_.sub);
    zmq_close(engine_.pub);
    zmq_ctx_term(engine_.ctx);
    return false;
  }

  logs::log(INFO,
            "Connected to ZMQ Engine: pub(tx): [%u], sub(rx): [%u], rx "
            "filters: %s; %s\n",
            ZMQ_FLATSAT_ENGINE_XSUB_PORT, ZMQ_FLATSAT_ENGINE_XPUB_PORT,
            desc_.name.c_str(), g_discoverTopic.data());

  return true;
}

bool Service::impl::registerCommand(CommandType command,
                                    std::vector<CommandArg> args,
                                    std::optional<CommandHandlerFn> handler,
                                    std::optional<void*> handlerData) {
  void* data = handlerData.value_or(nullptr);
  auto fn = handler.value_or(nullptr);

  if (command_registry_.contains(command)) {
    if (fn != nullptr) {
      auto& r = command_registry_[command];
      r.handlers.push_back({fn, data});
    }

    return true;
  }

  RegistryData first_reg = {
      .args = args,
      .handlers = {{fn, data}},
  };

  auto res = command_registry_.emplace(command, first_reg);

  return res.second;
}

bool Service::impl::registerHandler(CommandType& command,
                                    Service::CommandHandlerFn handler,
                                    void* handlerData) {
  if (!command_registry_.contains(command)) return false;

  auto& reg = command_registry_[command];

  reg.handlers.push_back({handler, handlerData});

  return true;
}

}  // namespace zmq

}  // namespace fsatutils
