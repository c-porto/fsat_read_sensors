#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <memory>
#include <optional>
#include <vector>

namespace fsatutils {

namespace zmq {

class Client {
 public:
  struct CommandArg {
    std::string name;
    std::string value;
  };

  struct CommandRequest {
    std::string name;
    std::vector<CommandArg> args;
  };

  Client(std::string host);
  ~Client();

  bool sendCommand(std::string_view service, Client::CommandRequest &req);
  bool sendDiscover();
  bool recvAndLogResponses();

 private:
  class impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace zmq

}  // namespace fsatutils

#endif
