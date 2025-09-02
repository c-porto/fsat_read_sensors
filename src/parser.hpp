#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <cstddef>
#include <stdint.h>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "log.hpp"
#include "daemon.hpp"

using json = nlohmann::json;

namespace
{
const std::unordered_map<std::string, eFlatSatCommands> cmdMap = {
	{ "track", CMD_TRACK_SENSOR },
	{ "untrack", CMD_UNTRACK_SENSOR },
	{ "set_measurement_period", CMD_SET_MEASUREMENT_PERIOD },
};

struct commandJson {
	std::string service;
	std::string command;
	std::string args;
};

/* Defining JSON Serialization */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(commandJson, service, command, args)

inline std::vector<std::string> splitString(const std::string &input, char delimiter)
{
	std::vector<std::string> result;

	for (auto part : input | std::views::split(delimiter)) {
		result.emplace_back(part.begin(), part.end());
	}

	return result;
}
}

struct SParsedArgs {
	std::optional<eFlatSatCommands> cmd;
	std::optional<std::string> sensor;
	std::optional<std::uint64_t> measPeriod;
	std::optional<std::string> error;
};

class CParse {
    public:
	static SParsedArgs parsePacket(void *packet, size_t size)
	{
		SParsedArgs pkt;
		json jsonCmd;
		const char *rawJson = static_cast<const char *>(packet);
		std::uint64_t meas;

		try {
			jsonCmd = json::parse(rawJson);
		} catch (json::parse_error &ex) {
			logs::log(ERR, "Failed to parse input!! Exception: %s\n", ex.what());
			SParsedArgs err = {
				.error = "Fail to parse JSON",
			};
			return err;
		}

		commandJson rq = jsonCmd.template get<commandJson>();

		if (rq.service != "read-sensors") {
			logs::log(ERR, "Request has invalid service field!\n");
			SParsedArgs err = {
				.error = "Invalid service",
			};
			return err;
		}

		auto cmd = cmdMap.find(rq.command);

		if (cmd == cmdMap.end()) {
			logs::log(ERR, "Request has invalid command field!\n");
			SParsedArgs err = {
				.error = "Invalid command",
			};
			return err;
		}

		std::vector<std::string> args = splitString(rq.args, ':');

		if (args.size() == 0U) {
			logs::log(ERR, "Request did not provide arguments!\n");
			SParsedArgs err = {
				.error = "Request did not provide arguments!",
			};
			return err;
		}

		pkt.cmd = cmd->second;

		switch (cmd->second) {
		case CMD_TRACK_SENSOR:
			pkt.sensor = args[0];
			break;
		case CMD_UNTRACK_SENSOR:
			pkt.sensor = args[0];
			break;
		case CMD_SET_MEASUREMENT_PERIOD:
			meas = strToU64(args[0].c_str());
			if (meas == 0U) {
				logs::log(ERR,
					  "Request argument was not valid! Provided Argument: %s\n",
					  args[0].c_str());
				SParsedArgs err = {
					.error =
						"Argument was not valid!! It should be a unsigned integer. Provided: " +
						args[0],
				};
				return err;
			}
			pkt.measPeriod = meas;
			break;
		default:
			logs::log(ERR, "Should be unreachable!!\n");
			SParsedArgs err = {
				.error = "Unreachable!?",
			};
			return err;
		}

		return pkt;
	}
};

#endif
