#include "sqlHelpers.h"

#include <sstream>
#include <string>
#include <vector>

/////// SQLite Back-End Helpers ///////
// Do not mess with these functions //

int callback(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
	std::vector<std::vector<std::string>>* records = static_cast<std::vector<std::vector<std::string>>*>(p_data);
	try {
		records->emplace_back(p_fields, p_fields + num_fields);
	}
	catch (...) {
		// abort select on failure, don't let exception propogate thru sqlite3 call-stack
		return 1;
	}
	return 0;
}

std::vector<std::vector<std::string>> select_stmt(sqlite3* db, const char* stmt)
{
	std::vector<std::vector<std::string>> records;
	char *errmsg;
	int ret = sqlite3_exec(db, stmt, callback, &records, &errmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Error in select statement: %s\n", errmsg);
	}
	return records;
}

void sql_stmt(sqlite3* db, const char* stmt)
{
	char *errmsg;
	int ret = sqlite3_exec(db, stmt, 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Error in select statement: %s\n", errmsg);
	}
}

/////// SQLite Front-End Helpers ///////

void createTableInDB(sqlite3* db, std::string tableName, std::vector<std::vector<std::string>> columns)
{
	// Do not use this command for the trialData table, that one is special!

	std::stringstream createTableC;
	createTableC << "CREATE TABLE IF NOT EXISTS " << tableName << " ( id integer PRIMARY KEY AUTOINCREMENT, ";
	for (int i = 0; i < columns.size(); i++)
	{
		createTableC << vec2str(columns[i], " ");
		if (i < columns.size() - 1)
		{
			createTableC << ", ";
		}
		else
		{
			createTableC << ");";
		}
	}
	sql_stmt(db, createTableC.str().c_str());
}

int insertIntoDB(sqlite3* db, std::string tableName, std::vector<std::string> values, std::string colNames) {
	// Quotes must be added around text-type values (eg. "addQuotes(expVersion)")

	std::vector<std::vector<std::string>> records;

	std::stringstream insertC;

	if (colNames.size() > 1)
	{
		insertC << "INSERT INTO " << tableName << colNames << " VALUES(" << vec2str(values, ",") << ");";
	}
	else
	{
		insertC << "INSERT INTO " << tableName << " VALUES(null, " << vec2str(values, ",") << ");";
	}
	sql_stmt(db, insertC.str().c_str());

	if (tableName != "trialData")
	{
		// get assigned id number
		std::stringstream queryC;
		queryC << "SELECT * FROM " << tableName << " WHERE id = (SELECT MAX(id) FROM " << tableName << ");";
		records = select_stmt(db, queryC.str().c_str());
		return std::stoi(records[0][0]);
	}
	else
	{
		return 0;
	}

}

std::string queryStimDB(sqlite3* db, int stimID, std::string columnName)
{
	std::stringstream queryC;
	queryC << "SELECT " << columnName << " FROM stimParams WHERE id =" << std::to_string(stimID) << ";";
	std::vector<std::vector<std::string>> records = select_stmt(db, queryC.str().c_str());
	return records[0][0];
}


void createTrialDataTable(sqlite3* db)
{
	// These are the params being recorded per trial
	// NOTE: the combination of expID and stimID should uniquely identify all other parameters
	// This table is only for recording dynamic variables (eg trialNum, response, stimVal)
	// You shouldn't need to edit this function unless you are also editing the StimManager class

	std::string trialDataCreateC =
		"CREATE TABLE IF NOT EXISTS trialData"
		"( expID integer NOT NULL, "
		"stimID integer NOT NULL, "
		"trialID integer NOT NULL, "
		"dateTime text NOT NULL, "
		"stimVal real NOT NULL, "
		"response integer NOT NULL, "
		"reactionTime integer, " // optional for timed fsm
		"textureIx integer, " // optional for randomized per trial textures
		"reversalCount integer, " // optional for staircase
		"responseUpCount integer, " // optional for staircase
		"responseDownCount integer, " // optional for staircase
		"stepSize real, " // optional for staircase
		"PRIMARY KEY(expID, stimID, trialID));";
	sql_stmt(db, trialDataCreateC.c_str());
}

