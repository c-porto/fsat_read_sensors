#include "sensor-manager.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "log.hpp"
#include "sensor.hpp"

namespace fs = std::filesystem;

static void readAllInaTypes(const std::pair<std::string, std::shared_ptr<CSensor> >& ina) {
    const std::array<eMeasureType, 4> types = {BUS_VOLT, SHUNT_VOLT, CURRENT, POWER};

    for (const auto& type : types) {
        std::optional<double> value = ina.second->read(type);

        if (value)
            logs::logSensorData(ina.first, type, *value);
    }
}

void CSensorManager::matchForDeviceNames(std::vector<std::string>& searchList, std::string name, fs::path it) {
    for (auto dev = searchList.begin(); dev != searchList.end(); ++dev) {
        if (*dev == name) {
            std::string hwmonPath = it.parent_path();

            auto        sensor = std::make_shared<CSensor>(hwmonPath, (name == std::string("tmp102")) ? TMP112 : INA219);
            auto        pair   = std::make_pair(name, sensor);

            /* Save path+dev pair on the internal map */
            m_mSensorMap.insert(pair);

            logs::log(INFO, "Found device |" + *dev + "| in path: " + hwmonPath);

            /* Remove device of the search list */
            searchList.erase(dev);

            return;
        }
    }
}

void CSensorManager::registerSensors(std::vector<std::string>&& searchList) {
    const std::string        targetName = "uevent";
    std::vector<std::string> list       = std::move(searchList);

    if (!fs::exists(m_szBaseHwmonPath) || !fs::is_directory(m_szBaseHwmonPath)) {
        logs::log(ERR, "Base path provided is not valid!");
        logs::log(ERR, "Terminating...");

        exit(1);
    }

    try {
        for (const auto& file : fs::directory_iterator(m_szBaseHwmonPath, fs::directory_options::follow_directory_symlink)) {
            fs::path ufile = file.path() / targetName;
            if (fs::exists(ufile)) {
                std::ifstream ifs;
                std::string   deviceName;
                std::string   input;

                ifs.open(ufile, std::ios::in);
                std::getline(ifs, input);

                if (input.rfind("OF_NAME=", 0) == 0)
                    deviceName = input.substr(strlen("OF_NAME="));
                else
                    continue;

                this->matchForDeviceNames(list, deviceName, ufile);
            }
        }
    } catch (std::exception& e) {
        std::stringstream ss;
        ss << "Exception occurred: " << e.what() << "\n";
        logs::log(ERR, ss.str());

        return;
    }

    for (const auto& unreg : searchList)
        logs::log(WARN, "Could not register device: " + std::string(unreg));
}

void CSensorManager::readTrackedSensors(void) {
    for (const auto& sensorPair : m_dTrackingSensors) {
        if (sensorPair.second->m_eIC == TMP112) {
            std::optional<double> read = sensorPair.second->read(TEMP);

            if (read)
                logs::logSensorData(sensorPair.first, TEMP, *read);

        } else {
            readAllInaTypes(sensorPair);
        }
    }
}

void CSensorManager::runManager(void) {
    while (true) {
        if (!m_dTrackingSensors.empty()) {
            this->readTrackedSensors();
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void CSensorManager::trackRegisteredDevices(void) {
    logs::log(INFO, "Start tracking register devices...");

    for (const auto& pair : m_mSensorMap)
        m_dTrackingSensors.push_back(pair);
}

int32_t CSensorManager::startTracking(std::string& sensorName) {
    int32_t err   = -1;
    auto    found = m_mSensorMap.find(sensorName);

    if (found != m_mSensorMap.end()) {
        m_mSensorMap.insert(*found);
        err = 0;
    }

    return err;
}
