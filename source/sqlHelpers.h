#pragma once

#include "sqlite/sqlite3.h"
#include <G3D/G3D.h>


bool createTableInDB(sqlite3* db, const String tableName, const Array<Array<String>>& columns);
/**All values sent to this function are stored in the database as Strings.*/
bool insertRowIntoDB(sqlite3* db, const String tableName, const Array<String>& values);
bool insertRowsIntoDB(sqlite3* db, const String tableName, const Array<Array<String>>& valueVector);
