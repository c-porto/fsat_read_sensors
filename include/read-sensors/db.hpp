#ifndef DB_HPP_
#define DB_HPP_

#include <sqlite3.h>
#include <string>

#include "sensor.hpp"

class SqliteDb {
    public:
	SqliteDb(std::string dbPath)
		: dbPath_{ dbPath }
	{
		createSqliteDB();
		createMeasurementsTable();
	}
	~SqliteDb()
	{
		sqlite3_close(dbHandle_);
	}

	operator bool() const
	{
		return dbOpen_;
	};

	int createSqliteDB(void);
	int createMeasurementsTable(void);

	int addMeasurementToDB(sensor::SensorDataEntry const &sensor);

    private:
	std::string dbPath_;
	sqlite3 *dbHandle_;
	bool dbOpen_{ false };
};

#endif
