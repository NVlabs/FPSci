#include "sqlHelpers.h"


void createTableInDB(sqlite3* db, String tableName, Array<Array<String>> columns) {
	// Do not use this command for the trialData table, that one is special!
	String createTableC = "CREATE TABLE IF NOT EXISTS " + tableName + " ( ";

	for (int i = 0; i < columns.size(); i++)
	{
		for (String val : columns[i]) {
			createTableC += val + " ";
		}
		if (i < columns.size() - 1) {
			createTableC +=  ",";
		}
		else {
			createTableC += ");";
		}
	}
	logPrintf("Creating table %s w/ SQL query:%s\n\n", tableName.c_str(), createTableC.c_str());
	char* errmsg;
	int ret = sqlite3_exec(db, createTableC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Error in CREATE TABLE statement: %s\n", errmsg);
	}
}

void insertRowIntoDB(sqlite3* db, String tableName, Array<String> values, String colNames) {
	// Quotes must be added around text-type values (eg. "addQuotes(expVersion)")
	// Note that ID does not need to be provided unless PRIMARY KEY is set.
	String insertC = "INSERT INTO " + tableName + colNames + " VALUES(";
	for (int i = 0; i < values.size(); i++) {
		insertC += values[i];
		if(i < values.size() - 1) insertC += ",";
	}
	insertC += ");";
	//logPrintf("Inserting row into %s table w/ SQL query:%s\n\n", tableName.c_str(), insertC.c_str());
	char* errmsg;
	int ret = sqlite3_exec(db, insertC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Error in INSERT INTO statement: %s\n", errmsg);
	}
}

void insertRowsIntoDB(sqlite3* db, String tableName, Array<Array<String>> value_vector, String colNames) {
	// Quotes must be added around text-type values
	// Note that ID does not need to be provided unless PRIMARY KEY is set
	String insertC = "INSERT INTO " + tableName + colNames + " VALUES";
	for (int i = 0; i < value_vector.size(); ++i) {
		insertC += "(";
		for (int j = 0; j < value_vector[i].size(); j++) {
			insertC += value_vector[i][j];
			if (j < value_vector[i].size() - 1) insertC += ",";
		}
		insertC += ")";
		if (i < value_vector.size() - 1) { // We have more rows coming after this row.
			insertC += ","; 
		}
		else { // The last row of this insert operation. Terminate it with a semi-colon.
			insertC += ";";
		}
	}
	//logPrintf("Inserting rows into %s table with SQL query:%s\n\n", tableName.c_str(), insertC.c_str());
	char* errmsg;
	int ret = sqlite3_exec(db, insertC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Error in INSERT INTO statement: %s\n", errmsg);
	}
}

