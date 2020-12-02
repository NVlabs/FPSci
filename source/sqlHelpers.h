#pragma once

#include "sqlite/sqlite3.h"
#include <G3D/G3D.h>


bool createTableInDB(sqlite3* db, String tableName, Array<Array<String>> columns);
bool insertRowIntoDB(sqlite3* db, String tableName, Array<String> values, String colNames = "");
bool insertRowsIntoDB(sqlite3* db, String tableName, Array<Array<String>> valueVector, String colNames = "");
