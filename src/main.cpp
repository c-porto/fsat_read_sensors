#include <memory>

#include "log.hpp"
#include "sensor-manager.hpp"
#include "daemon.hpp"
#include <vector>

#define BASE_PATH "/sys/class/hwmon/"
#define LOG_DIR   "/var/local/fsat/log/"

std::vector<std::string> devs = {"main-radio-power", "tmp102", "obdh-power", "beacon-power", "edc-power", "beacon2-power", "antenna-power", "payload-power"};

int main(int argc, char** argv) {
    logs::init(LOG_DIR);
    logs::disableSensorStdOut = true;
    logs::disableTime = true;

    std::shared_ptr<CSensorManager> man = std::make_shared<CSensorManager>(BASE_PATH);

    CDaemon daemon{man};

    man->registerSensors(std::move(devs));
    man->runManager();
}
