#include <cstddef>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <systemd/sd-daemon.h>

#include "parser.hpp"
#include "daemon.hpp"
#include "log.hpp"

Daemon::Daemon(std::shared_ptr<SensorManager> man)
	: m_pManager{ man }
{
	std::ofstream ofs;
	pid_t pid = getpid();

	this->setupSocket();

	ofs.open("/run/read-sensors/read-sensors.pid", std::ios::out | std::ios::trunc);
	ofs << pid;
	ofs.close();

	m_tId = std::thread{ &Daemon::run, this };

	m_tId.detach();
}

void Daemon::run(void)
{
	constexpr size_t bufSize = 1024U;
	char buffer[bufSize];
	size_t readSize;

	logs::log(INFO, "Starting to listen to socket...\n");

	while (true) {
		readSize = this->rxCommand(buffer, bufSize);

		logs::log(INFO, "Received a command!\n");

		if (readSize > 0) {
			std::string res = this->parseCommand(buffer, readSize);

			this->sendResponse(res.c_str(), res.size() + 1);
		}
	}

	close(m_iSockFd);
}

void Daemon::setupSocket(void)
{
	int num_fds = sd_listen_fds(0);

	if (num_fds < 1) {
		logs::log(ERR, "No socket file descriptors received.\n");
		logs::log(ERR, "Terminating...\n");
		exit(1);
	}

	m_iSockFd = SD_LISTEN_FDS_START;

	// Check if the received FD is a valid listening socket
	if (!sd_is_socket(m_iSockFd, AF_UNIX, SOCK_STREAM, -1)) {
		logs::log(ERR, "No socket file descriptors received.\n");
		logs::log(ERR, "Terminating...\n");
		exit(1);
	}
}

std::string Daemon::parseCommand(void *packet, size_t size)
{
	SParsedArgs pkt = CParse::parsePacket(packet, size);
	eFlatSatCommands cmd;
	std::string response;
	int32_t err = 0;

	if (pkt.error) {
		logs::log(ERR, "Parsing request failed!\n");
		return *pkt.error;
	}

	cmd = *pkt.cmd;

	switch (cmd) {
	case CMD_TRACK_SENSOR: {
		logs::log(INFO, "Track sensor command received.\n");

		err = m_pManager->startTracking(*pkt.sensor);
		break;
	}
	case CMD_UNTRACK_SENSOR: {
		logs::log(INFO, "Untrack sensor command received.\n");

		err = m_pManager->stopTracking(*pkt.sensor);
		break;
	}
	case CMD_SET_MEASUREMENT_PERIOD: {
		logs::log(INFO, "Set measurement period command received.\n");

		logs::log(INFO, "New period: %lu ms.\n", *pkt.measPeriod);

		err = m_pManager->setMeasurementPeriod(*pkt.measPeriod);
		break;
	}
	default:
		logs::log(ERR, "Invalid command received!\n");
		break;
	}

	if (err == 0)
		response = "Command `" + std::string{ cmdToString(cmd) } +
			   "` was executed sucessfully!";
	else
		response =
			"Command `" + std::string{ cmdToString(cmd) } +
			"` failed in execution! Check the service journal entries to obtain more information.";

	return response;
}

ssize_t Daemon::rxCommand(void *buff, size_t maxSize)
{
	int clientFd = accept(m_iSockFd, NULL, NULL);

	if (clientFd < 0) {
		logs::log(ERR, "Error while accepting the connection!\n");
		return -1;
	}

	m_iClientFd = clientFd;

	ssize_t rBytes = read(clientFd, buff, maxSize - 1);

	if (rBytes < 0) {
		logs::log(ERR, "Error while reading data from the client!\n");
		return -1;
	}

	char *rq = static_cast<char *>(buff);

	rq[rBytes] = '\0';

	return rBytes;
}

ssize_t Daemon::sendResponse(const char *res, size_t size)
{
	ssize_t sBytes;

	sBytes = send(m_iClientFd, res, size, 0U);

	if (sBytes < 0)
		logs::log(ERR, "Error while sending response to client!\n");

	close(m_iClientFd);
	return sBytes;
}
