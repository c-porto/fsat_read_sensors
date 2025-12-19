#ifndef SERVICE_HPP_
#define SERVICE_HPP_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "zprotocol.hpp"

namespace fsatutils {

namespace zmq {

class Service {
 public:
  using CommandHandlerFn = std::function<void(void*, Command)>;

  struct ServiceDescription {
    std::string name;
    std::string version;
    uint8_t compatibleProtocols;
    uint8_t preferedProtocol;
  };

  Service(ServiceDescription desc);
  ~Service();

  void runService();
  void stopService();

  Service& registerCommand(
      CommandType command, std::vector<CommandArg> args,
      std::optional<CommandHandlerFn> handler = std::nullopt,
      std::optional<void*> handlerData = std::nullopt);

  bool registerHandler(CommandType& command, CommandHandlerFn handler,
                       void* handlerData);

 private:
  class impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace zmq

}  // namespace fsatutils

#endif
