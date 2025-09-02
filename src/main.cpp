#include <memory>

#include "log.hpp"
#include "sensor-manager.hpp"
#include "daemon.hpp"

#define LOG_DIR "/var/log/fsat/"
#define DB_PATH "/var/local/read-sensors.sqlite3"

int main(int argc, char **argv)
{
	logs::init(LOG_DIR);

	std::shared_ptr<SensorManager> man = std::make_shared<SensorManager>(DB_PATH);

	Daemon daemon{ man };

	man->runManager();
}
