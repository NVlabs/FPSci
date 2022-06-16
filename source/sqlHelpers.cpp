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

bool insertRowIntoDB(sqlite3* db, const String tableName, const Array<String>& values, const String colNames) {
	if (values.length() == 0) {
		logPrintf("Warning insert row with empty values ignored!\n");
		return false;	// Don't attempt to insert for empty values
	}
	// Quotes must be added around text-type values (eg. "addQuotes(expVersion)")
	// Note that ID does not need to be provided unless PRIMARY KEY is set.
	String insertC = "INSERT INTO " + tableName + colNames + " VALUES(";
	for (int i = 0; i < values.size(); i++) {
		String insertVal = values[i];
		if (beginsWith(values[i], "'")) {	// Check if we are handling a single-quote delimited string
			insertVal = values[i].substr(1, values[i].length() - 2);
			replaceSingleQuote(insertVal);
			insertVal = "'" + insertVal + "'";
		}
		insertC += insertVal;
		if(i < values.size() - 1) insertC += ",";
	}
	insertC += ");";
	
	char* errmsg;
	int ret = sqlite3_exec(db, insertC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		logPrintf("Error in INSERT INTO statement (%s): %s\n", insertC, errmsg);
	}
	return ret == SQLITE_OK;
}

bool insertRowsIntoDB(sqlite3* db, const String tableName, const Array<Array<String>>& value_vector, const String colNames) {
	if (value_vector.length() == 0) {
		logPrintf("Warning insert rows with empty row value array ignored!\n");
		return false;		// Don't insert for empty value vector (creates an error)
	}
	// Quotes must be added around text-type values
	// Note that ID does not need to be provided unless PRIMARY KEY is set
	String insertC = "INSERT INTO " + tableName + colNames + " VALUES";
	for (int i = 0; i < value_vector.size(); ++i) {
		insertC += "(";
		for (int j = 0; j < value_vector[i].size(); j++) {
			String insertVal = value_vector[i][j];
			if (beginsWith(value_vector[i][j], "'")) {	// Check if we are handling a single-quote delimited string
				insertVal = value_vector[i][j].substr(1, value_vector[i][j].length() - 2);
				replaceSingleQuote(insertVal);
				insertVal = "'" + insertVal + "'";
			}
			insertC += insertVal;
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
		logPrintf("Error in INSERT INTO statement (%s): %s\n", insertC, errmsg);
	}
	return ret == SQLITE_OK;
}

bool replaceSingleQuote(String& str) {
	size_t pos;
	size_t lastPos = 0;
	bool found = false;
	while ((pos = str.find('\'', lastPos)) != String::npos) {
		found = true;
		str = str.substr(0, pos) + "'" + str.substr(pos);
		lastPos = pos + 2;
	}
	return found;
}
