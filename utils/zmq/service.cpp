#include <zmq.h>

#include <array>
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
      std::span<std::uint8_t, ZMQ_FLATSAT_ENGINE_MTU> buf,
      std::span<const uint8_t> topic, int more, std::size_t more_size);

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
    std::array<std::uint8_t, ZMQ_FLATSAT_ENGINE_MTU> buf;
    int more = 0;
    std::size_t more_size = sizeof(more);

    int res = zmq_recv(engine_.sub, buf.data(), buf.size(), 0);

    if (res < 0) {
      logs::log(ERR, "Error recv data [%s]\n", zmq_strerror(zmq_errno()));
      continue;
    }

    zmq_getsockopt(engine_.sub, ZMQ_RCVMORE, &more, &more_size);

    if (!more) {
      logs::log(ERR, "Message is not multipart!\n");
      continue;
    }

    std::span<uint8_t> m{buf.data(), static_cast<std::size_t>(res)};

    std::variant<std::monostate, Command, DiscoverMsgHeader> request =
        parseMessage(buf, m, more, more_size);

    if (std::holds_alternative<std::monostate>(request)) {
      logs::log(ERR, "Failed to parse message!");
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
  }
}

std::variant<std::monostate, Command, DiscoverMsgHeader>
Service::impl::parseMessage(std::span<std::uint8_t, ZMQ_FLATSAT_ENGINE_MTU> buf,
                            std::span<const uint8_t> topic, int more,
                            std::size_t more_size) {
  auto min_size = std::min(g_discoverTopic.size(), desc_.name.size());

  if (topic.size() < min_size) {
    logs::log(ERR, "Message was too short to be parsed!");
    return std::monostate{};
  }

  if (topic.size() == g_discoverTopic.size()) {
    /* Check the subscribed topic of the message */

    if (std::equal(topic.begin(), topic.end(), g_discoverTopic.begin())) {
      int res = zmq_recv(engine_.sub, buf.data(), buf.size(), 0);

      if (res < 0) {
        logs::log(ERR, "Error recv discover header [%s]\n",
                  zmq_strerror(zmq_errno()));
        return std::monostate{};
      }

      return DiscoverMsgHeader{.version = buf[0]};
    }
  }

  /* If Topic isn't "disc" it must be the service's name, courtesy of ZMQ
   * filters */

  int res = zmq_recv(engine_.sub, buf.data(), buf.size(), 0);

  if (res < 0) {
    logs::log(ERR, "Error recv command header [%s]\n",
              zmq_strerror(zmq_errno()));
    return std::monostate{};
  } else if (res != 2) {
    logs::log(ERR, "Command header must be 2 bytes\n");
    return std::monostate{};
  }

  std::span<uint8_t> raw_header{buf.data(), 2U};

  CommandMsgHeader header = {
      .version = raw_header[0],
      .proto = static_cast<MessageProtocol>(raw_header[1]),
  };

  zmq_getsockopt(engine_.sub, ZMQ_RCVMORE, &more, &more_size);

  if (!more) {
    logs::log(ERR, "Payload is missing on multipart message!\n");
    return std::monostate{};
  }

  res = zmq_recv(engine_.sub, buf.data(), buf.size(), 0);

  if (res < 0) {
    logs::log(ERR, "Error recv command payload [%s]\n",
              zmq_strerror(zmq_errno()));
    return std::monostate{};
  }

  std::span<const uint8_t> payload{buf.data(), static_cast<std::size_t>(res)};

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
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  engine_.sub = zmq_socket(engine_.ctx, ZMQ_SUB);

  if (engine_.sub == nullptr) {
    logs::log(ERR, "Failed to create zmq subscribe!\n");
    zmq_close(engine_.pub);
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  const char* xsub = "tcp://0.0.0.0:2808";

  if (zmq_connect(engine_.pub, xsub) != 0) {
    logs::log(ERR, "Failed to connect to engine xsub!\n");
    zmq_close(engine_.pub);
    zmq_close(engine_.sub);
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  const char* xpub = "tcp://0.0.0.0:2809";

  if (zmq_connect(engine_.sub, xpub) != 0) {
    logs::log(ERR, "Failed to connect to engine xpub!\n");
    zmq_close(engine_.pub);
    zmq_close(engine_.sub);
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  if (zmq_setsockopt(engine_.sub, ZMQ_SUBSCRIBE, desc_.name.c_str(),
                     desc_.name.size()) != 0) {
    logs::log(ERR, "Failed to subscribe to service name!\n");
    zmq_close(engine_.sub);
    zmq_close(engine_.pub);
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  if (zmq_setsockopt(engine_.sub, ZMQ_SUBSCRIBE, g_discoverTopic.data(), 4U) !=
      0) {
    logs::log(ERR, "Failed to subscribe to discover command!\n");
    zmq_close(engine_.sub);
    zmq_close(engine_.pub);
    zmq_ctx_destroy(engine_.ctx);
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
