#pragma once

#include "sqlite/sqlite3.h"
#include <G3D/G3D.h>
#include <string>
#include <vector>
#include <sstream>


void createTableInDB(sqlite3* db, String tableName, Array<Array<String>> columns);
void insertRowIntoDB(sqlite3* db, String tableName, Array<String> values, String colNames = "");
void insertRowsIntoDB(sqlite3* db, String tableName, Array<Array<String>> valueVector, String colNames = "");
int getMaxID(sqlite3* db, String tableName);
String addQuotes(String s);

/**
	To operate generically on datatypes in code that includes this
	library, these functions need to be recompiled with the code that
	includes this library so the compiler can deduce which types need
	function definitions. If the implementations were in the cpp file,
	type deduction would happen at compile time of *this* library instead.
*/
template <class T>
String vec2str(Array<T> v, String delimiter = ",")
{
	String ss;
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i != 0)
		{
			ss += delimiter;
		}
		ss += v[i];
	}
	return ss;
}
