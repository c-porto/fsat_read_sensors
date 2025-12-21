#ifndef ZPROTOCOL_HPP_
#define ZPROTOCOL_HPP_

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utils/log/log.hpp>
#include <vector>

#define ZMQ_FLATSAT_ENGINE_MTU 8192U

#define ZMQ_FLATSAT_ENGINE_XPUB_PORT 2809
#define ZMQ_FLATSAT_ENGINE_XSUB_PORT 2808

namespace fsatutils {

namespace zmq {

enum class ArgType : uint8_t {
  INT8,
  UINT8,
  INT16,
  UINT16,
  INT32,
  UINT32,
  INT64,
  UINT64,
  STRING,
  BLOB,
};

enum class MessageProtocol : uint8_t {
  BINARY = 0x01,
  JSON = 0x02,
  PROTOBUF = 0x04,
};

struct DiscoverMsgHeader {
  uint8_t version;
};

struct CommandMsgHeader {
  uint8_t version;
  MessageProtocol proto;
};

using CommandType = std::string;

struct CommandArg {
  std::string name;
  std::string value;
  ArgType type;
  bool optional;
};

struct Command {
  std::string cmd;
  std::vector<CommandArg> args;
};

inline constexpr std::string_view protoToString(MessageProtocol proto) {
  switch (proto) {
    case MessageProtocol::BINARY:
      return "binary";
    case MessageProtocol::JSON:
      return "JSON";
    case MessageProtocol::PROTOBUF:
      return "protobuf";
  }
  return "Unknown";
}

inline constexpr std::string_view typeToString(ArgType t) {
  switch (t) {
    case ArgType::INT8:
      return "i8";
    case ArgType::UINT8:
      return "u8";
    case ArgType::INT16:
      return "i16";
    case ArgType::UINT16:
      return "u16";
    case ArgType::INT32:
      return "i32";
    case ArgType::UINT32:
      return "u32";
    case ArgType::INT64:
      return "i64";
    case ArgType::UINT64:
      return "u64";
    case ArgType::STRING:
      return "string";
    case ArgType::BLOB:
      return "blob";
  }
  return "Invalid";
}

inline constexpr std::optional<ArgType> stringToType(
    std::string_view str) noexcept {
  if (str == "i8") return ArgType::INT8;
  if (str == "u8") return ArgType::UINT8;
  if (str == "i16") return ArgType::INT16;
  if (str == "u16") return ArgType::UINT16;
  if (str == "i32") return ArgType::INT32;
  if (str == "u32") return ArgType::UINT32;
  if (str == "i64") return ArgType::INT64;
  if (str == "u64") return ArgType::UINT64;
  if (str == "blob") return ArgType::BLOB;
  if (str == "string") return ArgType::STRING;
  return std::nullopt;
}

using json = nlohmann::json;

inline std::optional<Command> parseJSON(std::span<const uint8_t> command) {
  Command cmd;

  try {
    json j = json::parse(command);

    cmd.cmd = j["command"];

    for (auto const& arg : j["args"]) {
      CommandArg a{};

      a.name = arg["name"];
      a.value = arg["value"];

      cmd.args.push_back(a);
    }

  } catch (const json::parse_error& e) {
    logs::log(ERR, "Failed to parse JSON message: %s\n", e.what());
    return std::nullopt;
  } catch (const std::exception& e) {
    logs::log(ERR, "Exception raised in parsing JSON message: %s\n", e.what());
    return std::nullopt;
  }

  return cmd;
};

inline std::string_view g_discoverTopic = "disc";

}  // namespace zmq

}  // namespace fsatutils

#endif
