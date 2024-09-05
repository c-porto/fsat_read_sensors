#include <cstddef>
#include <optional>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "parser.hpp"
#include "daemon.hpp"
#include "log.hpp"

CDaemon::CDaemon(std::shared_ptr<CSensorManager> man) : m_pManager{man} {
    this->setupSocket();

    m_tId = std::thread{&CDaemon::run, this};

    m_tId.detach();
}

void CDaemon::run(void) {
    constexpr size_t bufSize = 256U;
    char             buffer[bufSize];
    size_t           readSize;

    logs::log(INFO, "Starting to listen to socket...");

    while (true) {
        readSize = this->rxCommand(buffer, bufSize);

        this->parseCommand(buffer, readSize);
    }

    close(m_iSockFd);
}

void CDaemon::setupSocket(void) {
    m_iSockFd = dup(0);

    if (m_iSockFd < 0) {
        logs::log(ERR, "Could not get unix socket id");
        exit(1);
    }
}

void CDaemon::parseCommand(void* packet, size_t size) {
    std::optional<SParsedArgs> pkt = CParse::parsePacket(packet, size);

    if (!pkt)
        return;

    switch (pkt->cmd) {
        case CMD_TRACK_SENSOR: {
            logs::log(INFO, "Track sensor command received");

            std::string sensor{pkt->payload, pkt->len};
            m_pManager->startTracking(sensor);
            break;
        }
        case CMD_REGISTER_SENSOR: {
            logs::log(INFO, "Register sensor command received");

            std::string sensor{pkt->payload, pkt->len};
            m_pManager->registerSingleSensor(sensor);
            break;
        }
        case CMD_UNTRACK_SENSOR: {
            logs::log(INFO, "Untrack sensor command received");

            std::string sensor{pkt->payload, pkt->len};
            m_pManager->stopTracking(sensor);
            break;
        }
        case CMD_UNREGISTER_SENSOR: {
            logs::log(INFO, "Unregister sensor command received");

            std::string sensor{pkt->payload, pkt->len};
            m_pManager->unregisterSingleSensor(sensor);
            break;
        }
        default: logs::log(ERR, "Invalid command received!"); break;
    }
}

ssize_t CDaemon::rxCommand(void* buff, size_t maxSize) {
    ssize_t rbytes = recvfrom(m_iSockFd, buff, maxSize, 0, NULL, NULL);

    if (rbytes < 0) {
        logs::log(ERR, "Error receiving message from socket");
        return -1;
    }

    return rbytes;
}
