#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "sensor-manager.hpp"
#include "log.hpp"
#include "sensor.hpp"

namespace fs = std::filesystem;

static void readAllInaTypes(const std::pair<std::string, std::shared_ptr<Sensor> >& ina) {
    const std::array<eMeasureType, 4> types = {BUS_VOLT, SHUNT_VOLT, CURRENT, POWER};

    for (const auto& type : types) {
        std::optional<double> value = ina.second->read(type);

        if (value)
            logs::logSensorData(ina.first, type, *value);
    }
}

void SensorManager::matchForDeviceNames(std::vector<std::string>& searchList, std::string name, fileIter it) {
    for (auto dev = searchList.begin(); dev != searchList.end(); ++dev) {
        if (*dev == name) {
            std::string hwmonPath = it.path().parent_path();

            auto        sensor = std::make_shared<Sensor>(hwmonPath, (name == std::string("tmp102")) ? TMP112 : INA219);
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

void SensorManager::registerSensors(std::vector<std::string>&& searchList) {
    const std::string targetName = "uevent";

    if (!fs::exists(m_szBaseHwmonPath) || !fs::is_directory(m_szBaseHwmonPath)) {
        logs::log(ERR, "Base path provided is not valid!");
        logs::log(ERR, "Terminating...");

        exit(1);
    }

    try {
        for (const auto& file : fs::recursive_directory_iterator(m_szBaseHwmonPath)) {
            if (file.is_regular_file() && file.path().filename() == targetName) {
                std::ifstream ifs;
                std::string   deviceName;
                std::string   input;

                ifs.open(file.path(), std::ios::in);
                std::getline(ifs, input);

                if (input.rfind("OF_NAME=", 0) == 0)
                    deviceName = input.substr(strlen("OF_NAME="));
                else
                    continue;

                this->matchForDeviceNames(searchList, deviceName.c_str(), file);
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

void SensorManager::readTrackedSensors(void) {
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

void SensorManager::runManager(void) {
    while (true) {
        if (!m_dTrackingSensors.empty()) {
            this->readTrackedSensors();
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void SensorManager::trackRegisteredDevices(void) {
    logs::log(INFO, "Start tracking register devices...");

    for (const auto& pair : m_mSensorMap)
        m_dTrackingSensors.push_back(pair);
}

int32_t startTracking(std::string_view sensorName) {
    return -1;
}
