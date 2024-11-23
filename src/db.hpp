#ifndef DB_HPP_
#define DB_HPP_

#include <cstdint>
#include <sqlite3.h>
#include <concepts>
#include <string>

#include "log.hpp"

template <typename T>
concept SensorDataEntry = requires(T t) {
    { t.sensorName } -> std::convertible_to<std::string>;
    { t.sensorType } -> std::convertible_to<std::string>;
    { t.measurementType } -> std::convertible_to<std::string>;
    { t.value } -> std::convertible_to<double>;
};

class CSqliteDb {
  public:
    CSqliteDb(std::string dbPath) : dbPath_{dbPath} {
        createSqliteDB();
        createMeasurementsTable();
    }
    ~CSqliteDb() {
        sqlite3_close(dbHandle_);
    }

    operator bool() const {
        return dbOpen_;
    };

    std::int8_t createSqliteDB(void);
    std::int8_t createMeasurementsTable(void);

    template <SensorDataEntry Sensor>
    std::int8_t addMeasurementToDB(Sensor& sensor);

  private:
    std::string dbPath_;
    sqlite3*    dbHandle_;
    bool        dbOpen_{false};
};

template <SensorDataEntry Sensor>
std::int8_t CSqliteDb::addMeasurementToDB(Sensor& sensor) {
    if (!dbOpen_)
        return -1;

    const char*   insertStmt = "INSERT INTO sensor_measurements (sensorName, sensorType, measurementType, value) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    int32_t       result = sqlite3_prepare_v2(dbHandle_, insertStmt, -1, &stmt, nullptr);

    if (result) {
        logs::log(ERR, "Failed to prepare DB statament!! Error: " + std::string{sqlite3_errmsg(dbHandle_)});
        return -1;
    }

    sqlite3_bind_text(stmt, 1, sensor.sensorName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, sensor.sensorType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, sensor.measurementType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, sensor.value);

    result = sqlite3_step(stmt);

    if (result != SQLITE_DONE) {
        logs::log(ERR, "Failed to execute DB statament!! Error: " + std::string{sqlite3_errmsg(dbHandle_)});
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

#endif
