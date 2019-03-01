#pragma once

#include "sqlite/sqlite3.h"
#include <string>
#include <vector>
#include <sstream>


void createTableInDB(sqlite3* db, std::string tableName, std::vector<std::vector<std::string>> columns);
int insertIntoDB(sqlite3* db, std::string tableName, std::vector<std::string> values, std::string colNames = "");
std::string queryStimDB(sqlite3* db, int stimID, std::string columnName);
void createTrialDataTable(sqlite3* db);

/**
	To operate generically on datatypes in code that includes this
	library, these functions need to be recompiled with the code that
	includes this library so the compiler can deduce which types need
	function definitions. If the implementations were in the cpp file,
	type deduction would happen at compile time of *this* library instead.
*/
template <class T>
std::string vec2str(std::vector<T> v, std::string delimiter = ",")
{
	std::stringstream ss;
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i != 0)
		{
			ss << delimiter;
		}
		ss << v[i];
	}
	return ss.str();
}
