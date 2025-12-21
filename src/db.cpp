#include <sqlite3.h>

#include <read-sensors/db.hpp>
#include <string>
#include <fsatutils/log/log.hpp>

int SqliteDb::createSqliteDB(void) {
  int32_t rq = sqlite3_open(dbPath_.c_str(), &dbHandle_);

  if (rq) {
    logs::log(ERR, "Failed to create/open database!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    sqlite3_close(dbHandle_);
    return -1;
  }

  dbOpen_ = true;

  return 0;
}

int SqliteDb::createMeasurementsTable(void) {
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

  char* errMsg = nullptr;

  int32_t result =
      sqlite3_exec(dbHandle_, createTableSQL, nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    logs::log(ERR, "Failed to create Sensors table in DB!!\n");
    sqlite3_free(errMsg);
    return -1;
  }

  logs::log(INFO, "Sensor Table was created (if it didn't exist already)!\n");

  return 0;
}

int SqliteDb::addMeasurementToDB(sensor::SensorDataEntry const& sensor) {
  if (!dbOpen_) return -1;

  const char* insertStmt =
      "INSERT INTO sensor_measurements (sensorName, sensorType, "
      "measurementType, value) VALUES (?, ?, ?, ?);";

  sqlite3_stmt* stmt = nullptr;

  int32_t result =
      sqlite3_prepare_v2(dbHandle_, insertStmt, -1, &stmt, nullptr);

  if (result) {
    logs::log(ERR, "Failed to prepare DB statament!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, sensor.sensorName.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, sensor.sensorType.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, sensor.measurementType.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_double(stmt, 4, sensor.value);

  result = sqlite3_step(stmt);

  if (result != SQLITE_DONE) {
    logs::log(ERR, "Failed to execute DB statament!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}
