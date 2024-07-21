#include <array>
#include <filesystem>
#include <memory>

#include "log.hpp"
#include "sensor.hpp"

#define DEFAULT_LOG_DIR       "./log"
#define DEFAULT_SYS_CLASS_DIR "./test/sys/class/hwmon/hwmon0/"

int salve() {
    if (!std::filesystem::exists(DEFAULT_LOG_DIR))
        std::filesystem::create_directories(DEFAULT_LOG_DIR);

    logs::init(DEFAULT_LOG_DIR);

    logs::coloredLogs = true;

    std::unique_ptr<const Sensor>        test = std::make_unique<const Sensor>(DEFAULT_SYS_CLASS_DIR, TMP112);

    const std::array<eMeasureType, 5ULL> arr = {SHUNT_VOLT, CURRENT, BUS_VOLT, POWER, TEMP};

    for (const auto& ms : arr) {
        auto read = test->read(ms);

        if (!read)
            logs::log(WARN, "FUNCIONA O OPTIONAL");

        std::string readTest = "Read value: " + std::to_string(*read);

        logs::log(INFO, readTest);
    }

    return 0;
}
