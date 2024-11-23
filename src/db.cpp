#include <sqlite3.h>
#include <string>

#include "db.hpp"
#include "log.hpp"

std::int8_t CSqliteDb::createSqliteDB(void) {
    int32_t rq = sqlite3_open(dbPath_.c_str(), &dbHandle_);

    if (rq) {
        logs::log(ERR, "Failed to create/open database!! Error: " + std::string{sqlite3_errmsg(dbHandle_)});
        sqlite3_close(dbHandle_);
        return -1;
    }

    dbOpen_ = true;

    return 0;
}

std::int8_t CSqliteDb::createMeasurementsTable(void) {
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS sensor_measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensorName TEXT NOT NULL,
            sensorType TEXT NOT NULL,
            measurementType TEXT NOT NULL,
            value REAL NOT NULL,
            timestamp TEXT DEFAULT CURRENT_TIMESTAMP
        );
    )";

    char*       errMsg = nullptr;

    int32_t     result = sqlite3_exec(dbHandle_, createTableSQL, nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        logs::log(ERR, "Failed to create Sensors table in DB!!");
        sqlite3_free(errMsg);
        return -1;
    }

    logs::log(INFO, "Sensor Table was created (if it didn't exist already)!");

    return 0;
}
