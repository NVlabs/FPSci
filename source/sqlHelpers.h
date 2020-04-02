#pragma once

#include "sqlite/sqlite3.h"
#include <G3D/G3D.h>
#include <string>
#include <vector>
#include <sstream>


void createTableInDB(sqlite3* db, String tableName, Array<Array<String>> columns);
void insertRowIntoDB(sqlite3* db, String tableName, Array<String> values, String colNames = "");
void insertRowsIntoDB(sqlite3* db, String tableName, Array<Array<String>> valueVector, String colNames = "");
