#include <memory>

#include "log.hpp"
#include "sensor-manager.hpp"
#include <vector>

#define BASE_PATH "test/sys/class/hwmon/"
#define LOG_DIR   "test/etc/fsat/log/"

std::vector<std::string> devs = {"main-radio-power", "tmp102", "obdh-power", "beacon-power", "edc-power", "beacon2-power", "antenna-power", "payload-power"};

int main(int argc, char** argv) {
    logs::init(LOG_DIR);

    std::shared_ptr<CSensorManager> man = std::make_shared<CSensorManager>(BASE_PATH);

    man->registerSensors(std::move(devs));
    man->trackRegisteredDevices();
    man->runManager();
}
