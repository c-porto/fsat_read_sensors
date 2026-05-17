#ifndef DB_HPP_
#define DB_HPP_

#include <sqlite3.h>

#include <string>
#include <utility>
#include <vector>

#include "sensor.hpp"

class SqliteDb {
 public:
  SqliteDb(std::string dbPath) : dbPath_{dbPath} {
    createSqliteDB();
    applyPragmas();
    createMeasurementsTable();
    createRegisteredSensorsTable();
    createTrackedSensorsTable();
    prepareInsertMeasurementStmt();
  }
  ~SqliteDb() {
    if (insertMeasurementStmt_) sqlite3_finalize(insertMeasurementStmt_);
    sqlite3_close(dbHandle_);
  }

  operator bool() const { return dbOpen_; };

  int createSqliteDB(void);
  int createMeasurementsTable(void);
  int createRegisteredSensorsTable(void);
  int createTrackedSensorsTable(void);

  int addMeasurementToDB(sensor::SensorDataEntry const& sensor);

  int begin(void);
  int commit(void);

  int addRegisteredSensor(std::string const& name, std::string const& type);
  int removeRegisteredSensor(std::string const& name);
  std::vector<std::pair<std::string, std::string>> getRegisteredSensors();

  int addTrackedSensor(std::string const& name, std::string const& channel);
  int removeTrackedSensor(std::string const& name, std::string const& channel);
  int removeTrackedSensorAll(std::string const& name);
  std::vector<std::pair<std::string, std::string>> getTrackedSensors();

 private:
  std::string dbPath_;
  sqlite3* dbHandle_;
  bool dbOpen_{false};
  sqlite3_stmt* insertMeasurementStmt_{nullptr};

  int prepareInsertMeasurementStmt(void);
  void applyPragmas(void);
};

#endif
