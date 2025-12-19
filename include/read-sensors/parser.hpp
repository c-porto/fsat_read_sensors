#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <stdint.h>

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utils/log/log.hpp>

#include "daemon.hpp"

namespace {

struct commandJson {
  std::string service;
  std::string command;
  std::string args;
};

}  // namespace

struct SParsedArgs {
  std::optional<eFlatSatCommands> cmd;
  std::optional<std::string> sensor;
  std::optional<std::uint64_t> measPeriod;
  std::optional<std::string> error;
};

class CParse {
 public:
  static SParsedArgs parsePacket(void* packet, size_t size) {
    (void)size;
    (void)packet;
    return {};
  }
};

#endif
