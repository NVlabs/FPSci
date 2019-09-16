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

void Logger::createResultsFile(String filename, String subjectID, String description)
{
	// generate folder result_data if it does not exist.
	if (!FileSystem::isDirectory(String("../results"))) {
		FileSystem::createDirectory(String("../results"));
	}

	// create a unique file name (can bring this back if desired)
	String timeStr = genUniqueTimestamp();

	// create the file
	if (sqlite3_open(filename.c_str(), &m_db)) {
		// Write an error to the log
		logPrintf(("Error creating log file: " + filename).c_str());
	}

	// create tables inside the db file.
	// 1. Experiment description (time and subject ID)
	// create sqlite table
	Array<Array<String>> expColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
			{ "appendingDescription", "text"}
	};
	createTableInDB(m_db, "Experiments", expColumns); // no need of Primary Key for this table.

	// populate table
	Array<String> expValues = {
		"'"+timeStr+"'",
		"'"+subjectID+"'",
		"'"+description+"'"
	};
	insertRowIntoDB(m_db, "Experiments", expValues);

	// 2. Targets
	// create sqlite table
	Array<Array<String>> targetColumns = {
			{ "trial_id", "integer"}, // Trial ID refers to the trial which this target is affiliated with
			{ "target_id", "text" },
			{ "type", "text"},
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "min_ecc_h", "real" },
			{ "min_ecc_V", "real" },
			{ "max_ecc_h", "real" },
			{ "max_ecc_V", "real" },
			{ "min_speed", "real" },
			{ "max_speed", "real" },
			{ "min_motion_change_period", "real" },
			{ "max_motion_change_period", "real" },
			{ "jump_enabled", "text" },
			{ "model", "text" }
	};
	createTableInDB(m_db, "Targets", targetColumns); // Primary Key needed for this table.

	// 3. Trials, only need to create the table.
	Array<Array<String>> trialColumns = {
			{ "trial_id", "integer" },
			{ "session_id", "text" },
			{ "session_mode", "text" },
			{ "start_time", "text" },
			{ "end_time", "text" },
			{ "task_execution_time", "real" },
			{ "remaining_targets", "real" },
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// 4. Target_Trajectory, only need to create the table.
	Array<Array<String>> targetTrajectoryColumns = {
			{ "time", "text" },
			{ "target_id", "text"},
			{ "position_x", "real" },
			{ "position_y", "real" },
			{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);

	// 5. Player_Action, only need to create the table.
	Array<Array<String>> viewTrajectoryColumns = {
			{ "time", "text" },
			{ "position_az", "real" },
			{ "position_el", "real" },
			{ "position_x", "real"},
			{ "position_y", "real"},
			{ "position_z", "real"},
			{ "event", "text" },
			{ "target_id", "text" },
	};
	createTableInDB(m_db, "Player_Action", viewTrajectoryColumns);

	// 6. Frame_Info, create the table
	Array<Array<String>> frameInfoColumns = {
			{"time", "text"},
			{"idt", "real"},
			{"sdt", "real"},
	};
	createTableInDB(m_db, "Frame_Info", frameInfoColumns);
}

void Logger::recordTargetTrajectory(Array<Array<String>> trajectory) {
	insertRowsIntoDB(m_db, "Target_Trajectory", trajectory);
}

void Logger::recordPlayerActions(Array<Array<String>> actions) {
	insertRowsIntoDB(m_db, "Player_Action", actions);
}

void Logger::recordFrameInfo(Array<Array<String>> info) {
	insertRowsIntoDB(m_db, "Frame_Info", info);
}

void Logger::addTargets(Array<SingleThresholdMeasurement> measurements) {
	for (int i = 0; i < measurements.size(); i++) {
		for (Param tparam : measurements[i].TargetParameters) {
			const String type = (tparam.val["destCount"] > 0) ? "waypoint" : "parametrized";
			Array<String> targetValues = {
				String(std::to_string(i)),										// This is the trial ID
				"'" + String(tparam.str["name"]) +"'",							// This is the target name
				"'" + type + "'",
				String(std::to_string(tparam.val["targetFrameRate"])),			
				String(std::to_string(tparam.val["targetFrameLag"])),
				String(std::to_string(tparam.val["minEccH"])),
				String(std::to_string(tparam.val["minEccV"])),
				String(std::to_string(tparam.val["maxEccH"])),
				String(std::to_string(tparam.val["maxEccV"])),
				String(std::to_string(tparam.val["minSpeed"])),
				String(std::to_string(tparam.val["maxSpeed"])),
				String(std::to_string(tparam.val["minMotionChangePeriod"])),
				String(std::to_string(tparam.val["maxMotionChangePeriod"])),
				"'" + String(tparam.str["jumpEnabled"]) + "'",
				"'" + String(tparam.str["model"] + "'")
			};
			insertRowIntoDB(m_db, "Targets", targetValues);
		}
	}
}

void Logger::recordTrialResponse(Array<String> values) {
	insertRowIntoDB(m_db, "Trials", values);
}

void Logger::closeResultsFile() {
	sqlite3_close(m_db);
}
