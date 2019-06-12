#include "Logger.h"

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
String Logger::genUniqueTimestamp() {
	_SYSTEMTIME t;
	GetLocalTime(&t);
	char tmCharArray[30] = { 0 };
	sprintf(tmCharArray, "%04d-%02d-%02d %02d:%02d:%02d.%06d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds*1000);
	std::string timeStr(tmCharArray);
	return String(timeStr);
}

String Logger::genFileTimestamp() {
	_SYSTEMTIME t;
	GetLocalTime(&t);
	char tmCharArray[30] = { 0 };
	sprintf(tmCharArray, "%04d_%02d_%02d-%02d_%02d_%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	std::string timeStr(tmCharArray);
	return String(timeStr);
}

void Logger::createResultsFile(String filename, String subjectID)
{
	// generate folder result_data if it does not exist.
	if (!FileSystem::isDirectory(String("../results"))) {
		FileSystem::createDirectory(String("../results"));
	}

	// create a unique file name
	String timeStr = genUniqueTimestamp();

	// create the file
	if (sqlite3_open(filename.c_str(), &m_db)) {
		// TODO: report error if failed.
	}

	// create tables inside the db file.
	// 1. Experiment description (time and subject ID)
	// create sqlite table
	Array<Array<String>> expColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
	};
	createTableInDB(m_db, "Experiments", expColumns); // no need of Primary Key for this table.

	// populate table
	Array<String> expValues = {
		"'"+timeStr+"'",
		"'"+subjectID+"'"
	};
	insertRowIntoDB(m_db, "Experiments", expValues);

	// 2. Conditions
	// create sqlite table
	Array<Array<String>> conditionColumns = {
			{ "id", "integer", "PRIMARY KEY"}, // this makes id a key value, requiring it to be unique for each row.
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "intensity", "real" },
	};
	createTableInDB(m_db, "Conditions", conditionColumns); // Primary Key needed for this table.

	// 3. Trials, only need to create the table.
	Array<Array<String>> trialColumns = {
			{ "condition_ID", "integer" },
			{ "session_ID", "text"},
			{ "session_mode", "text"},
			{ "start_time", "text" },
			{ "end_time", "text" },
			{ "task_execution_time", "real" },
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// 4. Frame_Info, create the table
	Array<Array<String>> frameInfoColumns = {
		{"time", "text"},
		{"idt", "real"},
		{"sdt", "real"},
	};
	createTableInDB(m_db, "Frame_Info", frameInfoColumns);
}

void Logger::addConditions(Array<SingleThresholdMeasurement> measurements){
	for (int i = 0; i < measurements.size(); ++i) {
		Param meas = measurements[i].getParam();
		Array<String> conditionValues = {
			String(i), // this index is uniquely and statically assigned to each SingleThresholdMeasurement.
			String(std::to_string(meas.val["targetFrameRate"])),
			String(std::to_string(meas.val["targetFrameLag"])),
			String(std::to_string(meas.val["intensity"])),
		};
		insertRowIntoDB(m_db, "Conditions", conditionValues);
	}
}

void Logger::recordTrialResponse(Array<String> values)
{
	insertRowIntoDB(m_db, "Trials", values);
}

void Logger::closeResultsFile() {
	sqlite3_close(m_db);
}
