#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "daemon.hpp"
#include "src/log.hpp"

struct SParse {
    uint8_t                      flags;
    uint8_t                      len;
    char*                        payload;

    static std::optional<SParse> parsePacket(void* packet, size_t size) {
        if (size < 3U) {
            logs::log(ERR, "Packet is not formatted correctly");
            return {};
        }

        SParse pkt;

        pkt.flags   = (*reinterpret_cast<uint8_t*>(packet));
        pkt.len     = (*(reinterpret_cast<uint8_t*>(packet) + 1UL));
        pkt.payload = reinterpret_cast<char*>(packet) + 2UL;

        if ((pkt.len + 2U) != size) {
            logs::log(ERR, "Packet lenght does not match payload lenght");
            return {};
        }

        return std::optional<SParse>(pkt);
    }
};

CDaemon::CDaemon(std::shared_ptr<CSensorManager> man) : m_pManager{man} {
    this->setupSocket();

    m_tId = std::thread{&CDaemon::run, this};

    m_tId.detach();
}

void CDaemon::run(void) {
    constexpr size_t bufSize = 256U;
    char buffer[bufSize];
    size_t readSize;

    logs::log(INFO, "Starting to listen to socket...");

    while (true) {
        this->rxCommand(buffer, &readSize, bufSize);

        logs::log(INFO, "Received a Command!!");

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
    std::optional<SParse> cmd = SParse::parsePacket(packet, size);

    if (!cmd)
        return;

    if (cmd->flags & 1U) {
        std::string sensor{cmd->payload, cmd->len};

        m_pManager->startTracking(sensor);
    }
}

ssize_t CDaemon::rxCommand(void* buff, size_t* readSize, size_t maxSize) {
    ssize_t rbytes = recvfrom(m_iSockFd, buff, maxSize, 0, NULL, NULL);

    if (rbytes < 0) {
        logs::log(ERR, "Error receiving message from socket");
        return -1;
    }

    *readSize = rbytes;

    return 0;
}
