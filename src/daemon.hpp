#ifndef DAEMON_HPP_
#define DAEMON_HPP_

#include <cstddef>
#include <memory>
#include <sys/types.h>
#include <thread>

#include "sensor-manager.hpp"

enum eFlatSatCommands {
	CMD_NONE,
	CMD_TRACK_SENSOR,
	CMD_UNTRACK_SENSOR,
	CMD_SET_MEASUREMENT_PERIOD,
};

inline std::uint64_t strToU64(const char *str)
{
	try {
		return static_cast<uint64_t>(std::stoull(str, nullptr, 10));
	} catch (const std::invalid_argument &e) {
		logs::log(ERR, "Invalid string to convert: %s\n", e.what());
		return 0;
	} catch (const std::out_of_range &e) {
		logs::log(ERR, "Out of Range to convert: %s\n", e.what());
		return 0;
	}
}

inline const char *cmdToString(eFlatSatCommands cmd)
{
	switch (cmd) {
	case CMD_TRACK_SENSOR:
		return "track sensor";
		break;
	case CMD_UNTRACK_SENSOR:
		return "untrack sensor";
		break;
	case CMD_SET_MEASUREMENT_PERIOD:
		return "set measurement period";
		break;
	default:
		break;
	}
	return "";
};

class Daemon {
    public:
	Daemon(std::shared_ptr<SensorManager> man);

    private:
	std::string parseCommand(void *packet, size_t size);
	void run(void);
	void setupSocket(void);
	ssize_t rxCommand(void *buff, size_t maxSize);
	ssize_t sendResponse(const char *res, size_t size);
	std::shared_ptr<SensorManager> m_pManager;
	std::thread m_tId;
	int m_iSockFd;
	int m_iClientFd;
};

#endif
