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

	// ? is a variable that will be bound later with a `sqlite3_bind_XXXX` function
	// see https://www2.sqlite.org/draft/c3ref/bind_blob.html for details
	String insertC = "INSERT INTO " + tableName + " VALUES(";
	for (int i = 0; i < values.size(); i++) {
		insertC += "?";
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

/** Helper function that takes in a start and end row and only inserts that given range. 
	Assumes the range is valid and below the SQLITE_LIMIT_VARIABLE_NUMBER */
bool groupInsertRows(sqlite3* db, const String tableName, const Array<Array<String>>& value_vector, const int start_row, const int end_row) {
	
	// ? is a variable that will be bound later with a `sqlite3_bind_XXXX` function
	// see https://www2.sqlite.org/draft/c3ref/bind_blob.html for details
	String insertC = "INSERT INTO " + tableName + " VALUES";
	for (int i = start_row; i < end_row; ++i) {
		insertC += "(";
		for (int j = 0; j < value_vector[i].size(); j++) {
			insertC += "?";
			if (j < value_vector[i].size() - 1) insertC += ",";
		}
		insertC += ")";
		if (i < end_row - 1) {
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
	for (int i = start_row; i < end_row; ++i) {
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

bool insertRowsIntoDB(sqlite3* db, const String tableName, const Array<Array<String>>& value_vector) {
	if (value_vector.length() == 0) {
		logPrintf("Warning insert rows with empty row value array ignored!\n");
		return false;
	}

	// Figure out how many insert groups we need
	// SQLITE_LIMIT_VARIABLE_NUMBER indicates the max number of variables in the compiled sqlite version
	int max_variables = sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
	int num_rows_per_group = max_variables / value_vector[0].size();
	//logPrintf("insertRows %d total in %d rows with %d groups of %d per INSERT\n", 
	//			value_vector.size() * value_vector[0].size(), 
	//			value_vector.size(), 
	//			num_rows_per_group, 
	//			value_vector[0].size()
	//		);

	int ret = 0; // sqlite return value
	int start_row = 0;
	bool ret_val = true;
	while (start_row < value_vector.size()) {
		int end_row = min(start_row + num_rows_per_group, value_vector.size());
		//logPrintf("inserting rows %d to %d\n", start_row, end_row);

		bool success = groupInsertRows(db, tableName, value_vector, start_row, end_row);

		if (!success) {
			logPrintf("Failed group insert!\n");
			ret_val = false;
		}

		start_row += num_rows_per_group;
	}

	return ret_val;
}

