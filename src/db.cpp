#include <sqlite3.h>

#include <fsatutils/log/log.hpp>
#include <read-sensors/db.hpp>
#include <string>

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
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensorName TEXT NOT NULL,
            sensorType TEXT NOT NULL,
            measurementType TEXT NOT NULL,
            value REAL NOT NULL,
            timestamp TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE INDEX IF NOT EXISTS idx_meas_sensor_time
            ON measurements(sensorName, timestamp);
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

void SqliteDb::applyPragmas(void) {
  if (!dbOpen_) return;

  /* WAL improves write throughput and lets readers (e.g. Grafana) run during
   * writes. synchronous is left at FULL to guarantee durability across
   * crashes/power loss. */
  const char* pragmas =
      "PRAGMA journal_mode = WAL;"
      "PRAGMA temp_store = MEMORY;";

  char* errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, pragmas, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(WARN, "Failed to apply DB pragmas: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
  }
}

int SqliteDb::prepareInsertMeasurementStmt(void) {
  if (!dbOpen_) return -1;

  const char* insertStmt =
      "INSERT INTO measurements (sensorName, sensorType, "
      "measurementType, value) VALUES (?, ?, ?, ?);";

  if (sqlite3_prepare_v2(dbHandle_, insertStmt, -1, &insertMeasurementStmt_,
                         nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare insert measurement stmt: %s\n",
              sqlite3_errmsg(dbHandle_));
    insertMeasurementStmt_ = nullptr;
    return -1;
  }
  return 0;
}

int SqliteDb::begin(void) {
  if (!dbOpen_) return -1;
  char* errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, "BEGIN;", nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(ERR, "BEGIN failed: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }
  return 0;
}

int SqliteDb::commit(void) {
  if (!dbOpen_) return -1;
  char* errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, "COMMIT;", nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(ERR, "COMMIT failed: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }
  return 0;
}

int SqliteDb::addMeasurementToDB(sensor::SensorDataEntry const& sensor) {
  if (!dbOpen_ || !insertMeasurementStmt_) return -1;

  sqlite3_stmt* stmt = insertMeasurementStmt_;
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);

  sqlite3_bind_text(stmt, 1, sensor.sensorName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, sensor.sensorType.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, sensor.measurementType.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 4, sensor.value);

  int32_t result = sqlite3_step(stmt);

  if (result != SQLITE_DONE) {
    logs::log(ERR, "Failed to execute DB statament!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  return 0;
}

int SqliteDb::createRegisteredSensorsTable(void) {
  const char* sql = R"(
        CREATE TABLE IF NOT EXISTS registered (
            sensorName TEXT PRIMARY KEY,
            sensorType TEXT NOT NULL,
            createdAt TEXT DEFAULT CURRENT_TIMESTAMP
        );
    )";

  char* errMsg = nullptr;
  int32_t result = sqlite3_exec(dbHandle_, sql, nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    logs::log(ERR, "Failed to create registered table!! Error: %s\n",
              errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }

  logs::log(INFO,
            "registered table was created (if it didn't exist)!\n");
  return 0;
}

int SqliteDb::createTrackedSensorsTable(void) {
  const char* sql = R"(
        CREATE TABLE IF NOT EXISTS tracked (
            sensorName TEXT NOT NULL,
            channel TEXT NOT NULL,
            createdAt TEXT DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (sensorName, channel)
        );
    )";

  char* errMsg = nullptr;
  int32_t result = sqlite3_exec(dbHandle_, sql, nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    logs::log(ERR, "Failed to create tracked table!! Error: %s\n",
              errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }

  logs::log(INFO, "tracked table was created (if it didn't exist)!\n");
  return 0;
}

int SqliteDb::addRegisteredSensor(std::string const& name,
                                  std::string const& type) {
  if (!dbOpen_) return -1;

  const char* stmtStr =
      "INSERT OR REPLACE INTO registered (sensorName, sensorType) "
      "VALUES (?, ?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare addRegisteredSensor: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    logs::log(ERR, "Failed to insert registered sensor: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  return 0;
}

int SqliteDb::removeRegisteredSensor(std::string const& name) {
  if (!dbOpen_) return -1;

  const char* stmtStr = "DELETE FROM registered WHERE sensorName = ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare removeRegisteredSensor: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE ? 0 : -1;
}

std::vector<std::pair<std::string, std::string>>
SqliteDb::getRegisteredSensors() {
  std::vector<std::pair<std::string, std::string>> out;
  if (!dbOpen_) return out;

  const char* stmtStr =
      "SELECT sensorName, sensorType FROM registered;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare getRegisteredSensors: %s\n",
              sqlite3_errmsg(dbHandle_));
    return out;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    out.emplace_back(n ? n : "", t ? t : "");
  }
  sqlite3_finalize(stmt);
  return out;
}

int SqliteDb::addTrackedSensor(std::string const& name,
                               std::string const& channel) {
  if (!dbOpen_) return -1;

  const char* stmtStr =
      "INSERT OR IGNORE INTO tracked (sensorName, channel) "
      "VALUES (?, ?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare addTrackedSensor: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, channel.c_str(), -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE ? 0 : -1;
}

int SqliteDb::removeTrackedSensor(std::string const& name,
                                  std::string const& channel) {
  if (!dbOpen_) return -1;

  const char* stmtStr =
      "DELETE FROM tracked WHERE sensorName = ? AND channel = ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare removeTrackedSensor: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, channel.c_str(), -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE ? 0 : -1;
}

int SqliteDb::removeTrackedSensorAll(std::string const& name) {
  if (!dbOpen_) return -1;

  const char* stmtStr = "DELETE FROM tracked WHERE sensorName = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare removeTrackedSensorAll: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE ? 0 : -1;
}

std::vector<std::pair<std::string, std::string>> SqliteDb::getTrackedSensors() {
  std::vector<std::pair<std::string, std::string>> out;
  if (!dbOpen_) return out;

  const char* stmtStr = "SELECT sensorName, channel FROM tracked;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(dbHandle_, stmtStr, -1, &stmt, nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare getTrackedSensors: %s\n",
              sqlite3_errmsg(dbHandle_));
    return out;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* c = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    out.emplace_back(n ? n : "", c ? c : "");
  }
  sqlite3_finalize(stmt);
  return out;
}
