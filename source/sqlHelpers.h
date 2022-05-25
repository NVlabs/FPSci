#pragma once

#include "sqlite/sqlite3.h"
#include <G3D/G3D.h>


bool createTableInDB(sqlite3* db, const String tableName, const Array<Array<String>>& columns);
bool insertRowIntoDB(sqlite3* db, const String tableName, const Array<String>& values, const String colNames = "");
bool insertRowsIntoDB(sqlite3* db, const String tableName, const Array<Array<String>>& valueVector, const String colNames = "");
