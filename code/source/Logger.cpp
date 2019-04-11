#include "Logger.h"

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
String Logger::genUniqueTimestamp() {
	time_t t = std::time(nullptr);
	std::tm tmbuf;
	localtime_s(&tmbuf, &t);
	char tmCharArray[17];
	std::strftime(tmCharArray, sizeof(tmCharArray), "%Y%m%d_%H%M%S", &tmbuf);
	std::string timeStr(tmCharArray);
	return String(timeStr);
}

void TargetLogger::createResultsFile(String filename, String subjectID)
{
	// generate folder result_data if it does not exist.
	if (!FileSystem::isDirectory(String("../results"))) {
		FileSystem::createDirectory(String("../results"));
	}

	// create a unique file name (can bring this back if desired)
	String timeStr = genUniqueTimestamp();

	// create the file
	if (sqlite3_open(filename.c_str(), &m_db)) {
		// TODO: report error if failed.
	}

	// create tables inside the db file.
	// 1. Experiment description (time and subject ID)
	// create sqlite table
	std::vector<std::vector<std::string>> expColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
	};
	createTableInDB(m_db, "Experiments", expColumns); // no need of Primary Key for this table.

	// populate table
	std::vector<std::string> expValues = {
		addQuotes(timeStr.c_str()),
		addQuotes(subjectID.c_str())
	};
	insertRowIntoDB(m_db, "Experiments", expValues);

	// 2. Conditions
	// create sqlite table
	std::vector<std::vector<std::string>> conditionColumns = {
			{ "id", "integer", "PRIMARY KEY"}, // this makes id a key value, requiring it to be unique for each row.
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "min_ecc_h", "real" },
			{ "min_ecc_V", "real" },
			{ "max_ecc_h", "real" },
			{ "max_ecc_V", "real" },
			{ "min_speed", "real" },
			{ "max_speed", "real" },
			{ "motion_change_period", "real" },
	};
	createTableInDB(m_db, "Conditions", conditionColumns); // Primary Key needed for this table.

	// 3. Trials, only need to create the table.
	std::vector<std::vector<std::string>> trialColumns = {
			{ "condition_ID", "integer" },
			{ "session_ID", "text" },
			{ "session_mode", "text" },
			{ "start_time", "real" },
			{ "end_time", "real" },
			{ "task_execution_time", "real" },
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// 4. Target_Trajectory, only need to create the table.
	std::vector<std::vector<std::string>> targetTrajectoryColumns = {
			{ "time", "real" },
			{ "position_x", "real" },
			{ "position_y", "real" },
			{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);

	// 5. Player_Action, only need to create the table.
	std::vector<std::vector<std::string>> viewTrajectoryColumns = {
			{ "time", "real" },
			{ "event", "text" },
			{ "position_az", "real" },
			{ "position_el", "real" },
	};
	createTableInDB(m_db, "Player_Action", viewTrajectoryColumns);
}

void TargetLogger::recordTargetTrajectory(std::vector<std::vector<std::string>> trajectory) {
	insertRowsIntoDB(m_db, "Target_Trajectory", trajectory);
}

void TargetLogger::recordPlayerActions(std::vector<std::vector<std::string>> actions) {
	insertRowsIntoDB(m_db, "Player_Action", actions);
}

void TargetLogger::addConditions(std::vector<SingleThresholdMeasurement> measurements) {
	for (int i = 0; i < measurements.size(); ++i) {
		Param meas = measurements[i].getParam();
		std::vector<std::string> conditionValues = {
			std::to_string(i), // this index is uniquely and statically assigned to each SingleThresholdMeasurement.
			std::to_string(meas.val["targetFrameRate"]),
			std::to_string(meas.val["targetFrameLag"]),
			std::to_string(meas.val["minEccH"]),
			std::to_string(meas.val["minEccV"]),
			std::to_string(meas.val["maxEccH"]),
			std::to_string(meas.val["maxEccV"]),
			std::to_string(meas.val["minSpeed"]),
			std::to_string(meas.val["maxSpeed"]),
			std::to_string(meas.val["motionChangePeriod"]),
		};
		insertRowIntoDB(m_db, "Conditions", conditionValues);
	}
}

void ReactionLogger::createResultsFile(String filename, String subjectID)
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
	std::vector<std::vector<std::string>> expColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
	};
	createTableInDB(m_db, "Experiments", expColumns); // no need of Primary Key for this table.

	// populate table
	std::vector<std::string> expValues = {
		addQuotes(timeStr.c_str()),
		addQuotes(subjectID.c_str())
	};
	insertRowIntoDB(m_db, "Experiments", expValues);

	// 2. Conditions
	// create sqlite table
	std::vector<std::vector<std::string>> conditionColumns = {
			{ "id", "integer", "PRIMARY KEY"}, // this makes id a key value, requiring it to be unique for each row.
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "intensity", "real" },
	};
	createTableInDB(m_db, "Conditions", conditionColumns); // Primary Key needed for this table.

	// 3. Trials, only need to create the table.
	std::vector<std::vector<std::string>> trialColumns = {
			{ "condition_ID", "integer" },
			{ "session_ID", "text"},
			{ "session_mode", "text"},
			{ "start_time", "real" },
			{ "end_time", "real" },
			{ "task_execution_time", "real" },
	};
	createTableInDB(m_db, "Trials", trialColumns);
}

void ReactionLogger::addConditions(std::vector<SingleThresholdMeasurement> measurements){
	for (int i = 0; i < measurements.size(); ++i) {
		Param meas = measurements[i].getParam();
		std::vector<std::string> conditionValues = {
			std::to_string(i), // this index is uniquely and statically assigned to each SingleThresholdMeasurement.
			std::to_string(meas.val["targetFrameRate"]),
			std::to_string(meas.val["targetFrameLag"]),
			std::to_string(meas.val["intensity"]),
		};
		insertRowIntoDB(m_db, "Conditions", conditionValues);
	}
}

void Logger::recordTrialResponse(std::vector<std::string> values)
{
	insertRowIntoDB(m_db, "Trials", values);
}

void Logger::closeResultsFile() {
	sqlite3_close(m_db);
}
