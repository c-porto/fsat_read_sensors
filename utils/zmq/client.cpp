#include <zmq.h>

#include <array>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <utils/errors.hpp>
#include <utils/log/log.hpp>
#include <utils/zmq/client.hpp>
#include <utils/zmq/zprotocol.hpp>

using json = nlohmann::json;

namespace fsatutils {

namespace zmq {

struct ZMQEngine {
  void* ctx;
  void* sub;
  void* pub;
};

class Client::impl {
 public:
  impl(std::string host);

  void cleanResources();

  bool sendCommand(std::string_view service, Client::CommandRequest& req);

  bool sendDiscover();

  bool recvAndLogResponses();

 private:
  bool connectToEngineProxy();
  ZMQEngine engine_;
  std::string host_;
};

Client::Client(std::string host) : impl_{std::make_unique<impl>(host)} {}

Client::~Client() { impl_->cleanResources(); }

bool Client::sendCommand(std::string_view service,
                         Client::CommandRequest& req) {
  return impl_->sendCommand(service, req);
}

bool Client::sendDiscover() { return impl_->sendDiscover(); }

bool Client::recvAndLogResponses() { return impl_->recvAndLogResponses(); }

Client::impl::impl(std::string host) : host_{std::move(host)} {
  if (!connectToEngineProxy()) {
    throw_runtime_error("Failed to connect to FlatSat2 ZMQ Engine!");
  }
}

void Client::impl::cleanResources() {
  if (engine_.pub != nullptr) {
    zmq_close(engine_.pub);
  }

  if (engine_.sub != nullptr) {
    zmq_close(engine_.pub);
  }

  if (engine_.ctx != nullptr) {
    zmq_ctx_destroy(engine_.ctx);
  }
}

bool Client::impl::sendCommand(std::string_view service,
                               Client::CommandRequest& req) {
  json command;

  command["command"] = req.name;

  json args = json::array();

  for (auto const& arg : req.args) {
    json a;

    a["name"] = arg.name;
    a["value"] = arg.value;

    args.push_back(a);
  }

  command["args"] = args;

  std::string payload = command.dump();

  CommandMsgHeader header = {.version = 1, .proto = MessageProtocol::JSON};

  if (zmq_send(engine_.pub, service.data(), service.size(), ZMQ_SNDMORE) < 0) {
    logs::log(ERR, "Failed to send service name! ZMQ error [%s]\n",
              zmq_strerror(errno));
    return false;
  }

  std::array<const std::uint8_t, 2> buf = {
      header.version, static_cast<std::uint8_t>(header.proto)};

  if (zmq_send(engine_.pub, buf.data(), buf.size(), ZMQ_SNDMORE) < 0) {
    logs::log(ERR, "Failed to send command header! ZMQ error [%s]\n",
              zmq_strerror(errno));
    return false;
  }

  if (zmq_send(engine_.pub, payload.data(), payload.size(), 0) < 0) {
    logs::log(ERR, "Failed to send JSON payload! ZMQ error [%s]\n",
              zmq_strerror(errno));
    return false;
  }

  return true;
}

bool Client::impl::sendDiscover() {
  DiscoverMsgHeader header = {.version = 1};

  if (zmq_send(engine_.pub, g_discoverTopic.data(), g_discoverTopic.size(),
               ZMQ_SNDMORE) < 0) {
    logs::log(ERR, "Failed to send discover topic! ZMQ error [%s]\n",
              zmq_strerror(errno));
    return false;
  }

  std::array<const std::uint8_t, 1> buf = {header.version};

  if (zmq_send(engine_.pub, buf.data(), buf.size(), 0) < 0) {
    logs::log(ERR, "Failed to send discover header! ZMQ error [%s]\n",
              zmq_strerror(errno));
    return false;
  }

  return true;
}

bool Client::impl::recvAndLogResponses() {
  using clock = std::chrono::steady_clock;
  using namespace std::chrono_literals;
  constexpr std::chrono::milliseconds window{800ms};

  std::array<char, ZMQ_FLATSAT_ENGINE_MTU> buf;

  bool received_any = false;

  const auto deadline = clock::now() + window;

  zmq_pollitem_t item = {
      .socket = engine_.sub, .fd = 0, .events = ZMQ_POLLIN, .revents = 0};

  while (1) {
    auto now = clock::now();

    if (now >= deadline) return received_any;

    auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);

    int rc = zmq_poll(&item, 1, remaining.count());

    if (rc == -1) {
      logs::log(ERR, "ZMQ Poll failed [%s]", zmq_strerror(errno));
      return false;
    }

    if (rc == 0) {
      return received_any;
    }

    if (item.revents & ZMQ_POLLIN) {
      while (1) {
        int events = 0;
        size_t size = sizeof(events);

        zmq_getsockopt(engine_.sub, ZMQ_EVENTS, &events, &size);

        if (!(events & ZMQ_POLLIN)) break;

        int res = zmq_recv(engine_.sub, buf.data(), buf.size(), 0);

        if (res < 0) {
          logs::log(ERR, "Failed to receve message! ZMQ error [%s]",
                    zmq_strerror(errno));
          return false;
        }

        received_any = true;

        std::string_view payload{buf.data(), static_cast<std::size_t>(res)};

        std::cout << "Discovered: " << payload << std::endl;
      }
    }
  }
}

bool Client::impl::connectToEngineProxy() {
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

  /* Subscribe to everything */
  if (zmq_setsockopt(engine_.sub, ZMQ_SUBSCRIBE, "", 0U) != 0) {
    logs::log(ERR, "Failed to subscribe to service name!\n");
    zmq_close(engine_.sub);
    zmq_close(engine_.pub);
    zmq_ctx_destroy(engine_.ctx);
    return false;
  }

  logs::log(DEBUG,
            "Connected to ZMQ Engine: pub(tx): [%u], sub(rx): [%u], rx "
            "filters: (none)\n",
            ZMQ_FLATSAT_ENGINE_XSUB_PORT, ZMQ_FLATSAT_ENGINE_XPUB_PORT);

  return true;
}

}  // namespace zmq

}  // namespace fsatutils
