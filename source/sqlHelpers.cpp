#include "sqlHelpers.h"


bool createTableInDB(sqlite3* db, const String tableName, const Array<Array<String>>& columns) {
	// This method builds up a query with the format "CREATE TABLE IF NOT EXISTS {tableName} ({column name} {column type} {column modifiers}, ...);
	// Columns is passed in using the order {column name, column type, column modifiers} for each entry in the Array

	String createTableC = "CREATE TABLE IF NOT EXISTS " + tableName + " ( ";
	for (int i = 0; i < columns.size(); i++) {
		for (int j = 0; j < columns[i].length(); j++) {
			String val = columns[i][j];
			createTableC += (j == 0 ? val : toUpper(val)) + " ";
		}
		if (i < columns.size() - 1) {
			createTableC +=  ",";			// Add comma to finish column
		}
		else {
			createTableC += ");";			// Add close parens to finish query
		}
	}
	logPrintf("Creating table %s w/ SQL query:%s\n\n", tableName.c_str(), createTableC.c_str());
	char* errmsg;
	int ret = sqlite3_exec(db, createTableC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		logPrintf("Error in CREATE TABLE statement (%s): %s\n", createTableC, errmsg);
	}
	return ret == SQLITE_OK;
}

bool insertRowIntoDB(sqlite3* db, const String tableName, const Array<String>& values) {
	if (values.length() == 0) {
		logPrintf("Warning insert row with empty values ignored!\n");
		return false;
	}

	String insertC = "INSERT INTO " + tableName + " VALUES(";
	for (int i = 0; i < values.size(); i++) {
		insertC += "?";
		//insertC += values[i];
		if(i < values.size() - 1) insertC += ",";
	}
	insertC += ");";

	// prepare
	sqlite3_stmt* res;
	int ret = sqlite3_prepare_v2(db, insertC.c_str(), -1, &res, 0);
	if (ret != SQLITE_OK)
	{
		logPrintf("Error preparing INSERT INTO statement (%s): %s\n", insertC, sqlite3_errmsg(db));
		return ret == SQLITE_OK;
	}
	// bind values
	for (int i = 0; i < values.size(); i++) {
		// All values sent to this function are explicitly Strings and get stored as text in the database
		sqlite3_bind_text(res, i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);
	}
	ret = sqlite3_step(res);
	if (ret != SQLITE_DONE)
	{
		logPrintf("Error in INSERT (%s) with VALUE including %s!\n", insertC, values[0]);
	}
	// clean up the sqlite3_stmt
	ret = sqlite3_finalize(res);
	//logPrintf("Inserting row into %s table w/ SQL query:%s\n\n", tableName.c_str(), insertC.c_str());
	if (ret != SQLITE_OK) {
		logPrintf("Error in INSERT INTO statement (%s): %s\n", insertC, sqlite3_errmsg(db));
	}
	return ret == SQLITE_OK;
}

bool insertRowsIntoDB(sqlite3* db, const String tableName, const Array<Array<String>>& value_vector) {
	if (value_vector.length() == 0) {
		logPrintf("Warning insert rows with empty row value array ignored!\n");
		return false;
	}

	//// TODO: need to not try to submit more than the max number of variables per statement
	//int max_variables = sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
	//logPrintf("Max sqlite variables %d", max_variables);
	//// If the size is too large, try again with half the size and return the combined result
	//if (value_vector.size() * value_vector[0].size() > max_variables) {
	//	// recurse on both halves
	//	int middle = value_vector.middleIndex();
	//	bool first = insertRowsIntoDB(db, tableName, value_vector[0:middle]);
	//	bool second = insertRowsIntoDB(db, tableName, value_vector[middle:-1]);
	//	return first && second;
	//}

	String insertC = "INSERT INTO " + tableName + " VALUES";
	for (int i = 0; i < value_vector.size(); ++i) {
	//int len = int(min(value_vector.size(), 5));
	//for (int i = 0; i < len; ++i) {
		insertC += "(";
		for (int j = 0; j < value_vector[i].size(); j++) {
			insertC += "?";
			if (j < value_vector[i].size() - 1) insertC += ",";
		}
		insertC += ")";
		if (i < value_vector.size() - 1) {
		//if (i < len - 1) {
			// We have more rows coming after this row.
			insertC += ",";
		}
		else {
			// The last row of this insert operation. Terminate it with a semi-colon (which is optional).
			insertC += ";";
		}
	}
	logPrintf("insertRowsIntoDB: %s\n\n", insertC);


	// prepare
	sqlite3_stmt* res;
	int ret = sqlite3_prepare_v2(db, insertC.c_str(), -1, &res, 0);
	if (ret != SQLITE_OK)
	{
		logPrintf("Error preparing INSERT INTO statement (%s): %s\n", insertC, sqlite3_errmsg(db));
		return ret == SQLITE_OK;
	}
	// bind values
	for (int i = 0; i < value_vector.size(); ++i) {
	//for (int i = 0; i < len; ++i) {
		for (int j = 0; j < value_vector[i].size(); j++) {
			sqlite3_bind_text(res, i * value_vector[i].size() + j + 1, value_vector[i][j].c_str(), -1, SQLITE_TRANSIENT);
		}
	}
	ret = sqlite3_step(res);
	if (ret != SQLITE_DONE)
	{
		logPrintf("Error in INSERT (%s) with VALUE including %s!\n", insertC, value_vector[0][0]);
	}
	// clean up the sqlite3_stmt
	ret = sqlite3_finalize(res);
	//logPrintf("Inserting row into %s table w/ SQL query:%s\n\n", tableName.c_str(), insertC.c_str());
	if (ret != SQLITE_OK) {
		logPrintf("Error in INSERT INTO statement (%s): %s\n", insertC, sqlite3_errmsg(db));
	}
	return ret == SQLITE_OK;
}

