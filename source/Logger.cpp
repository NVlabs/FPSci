#include "Logger.h"
#include "Session.h"

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
String Logger::genUniqueTimestamp() {
	return formatFileTime(getFileTime());
}

FILETIME Logger::getFileTime() {
	FILETIME ft;
	GetSystemTimePreciseAsFileTime(&ft);
	return ft;
}

String Logger::formatFileTime(FILETIME ft) {
	unsigned long long usecsinceepoch = (static_cast<unsigned long long>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;		// Get time since epoch in usec
	int usec = usecsinceepoch % 1000000;

	SYSTEMTIME datetime;
	FileTimeToSystemTime(&ft, &datetime);

	char tmCharArray[30] = { 0 };
	sprintf(tmCharArray, "%04d-%02d-%02d %02d:%02d:%02d.%06d", datetime.wYear, datetime.wMonth, datetime.wDay, datetime.wHour, datetime.wMinute, datetime.wSecond, usec);
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

void Logger::createResultsFile(String filename, String subjectID, String sessionID, String description)
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
	Columns sessColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "sessionID", "text", "NOT NULL"},
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
			{ "appendingDescription", "text"}
	};
	createTableInDB(m_db, "Sessions", sessColumns); // no need of Primary Key for this table.

	// populate table
	RowEntry sessValues = {
		"'" + sessionID + "'",
		"'" + timeStr + "'",
		"'" + subjectID + "'",
		"'" + description + "'"
	};
	insertRowIntoDB(m_db, "Sessions", sessValues);

	// 2. Targets
	// create sqlite table
	Columns targetColumns = {
			{ "name", "text"},
			{ "id", "text" },
			{ "type", "text"},
			{ "destSpace", "text"},
			{ "refresh_rate", "real"},
			{ "added_frame_lag", "real"},
			{ "min_size", "real"},
			{ "max_size", "real"},
			{ "min_ecc_h", "real" },
			{ "min_ecc_V", "real" },
			{ "max_ecc_h", "real" },
			{ "max_ecc_V", "real" },
			{ "min_speed", "real" },
			{ "max_speed", "real" },
			{ "min_motion_change_period", "real" },
			{ "max_motion_change_period", "real" },
			{ "jump_enabled", "text" },
			{ "model_file", "text" }
	};
	createTableInDB(m_db, "Targets", targetColumns); // Primary Key needed for this table.

	// 3. Trials, only need to create the table.
	Columns trialColumns = {
			{ "trial_id", "integer" },
			{ "session_id", "text" },
			{ "session_mode", "text" },
			{ "start_time", "text" },
			{ "end_time", "text" },
			{ "task_execution_time", "real" },
			{ "destroyed_targets", "real" },
			{ "total_targets", "real" }
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// 4. Target_Trajectory, only need to create the table.
	Columns targetTrajectoryColumns = {
			{ "time", "text" },
			{ "target_id", "text"},
			{ "position_x", "real" },
			{ "position_y", "real" },
			{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);

	// 5. Player_Action, only need to create the table.
	Columns viewTrajectoryColumns = {
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
	Columns frameInfoColumns = {
			{"time", "text"},
			//{"idt", "real"},
			{"sdt", "real"},
	};
	createTableInDB(m_db, "Frame_Info", frameInfoColumns);

	// 7. Question responses
	Columns questionColumns = {
		{"Session", "text"},
		{"Question", "text"},
		{"Response", "text"}
	};
	createTableInDB(m_db, "Questions", questionColumns);

	//8. User information
	Columns userColumns = {
		{"subjectID", "text"},
		{"session", "text"},
		{"time", "text"},
		{"cmp360", "real"},
		{"mouseDPI", "real"},
		{"reticleIndex", "int"},
		{"reticleScaleMin", "real"},
		{"reticleScaleMax", "real"},
		{"reticleColorMinScale", "text"},
		{"reticleColorMaxScale", "text"},
		{"turnScaleX", "real"},
		{"turnScaleY", "real"}
	};
	createTableInDB(m_db, "Users", userColumns);
}

void Logger::recordFrameInfo(const Array<FrameInfo>& frameInfo) {
	Array<RowEntry> rows;
	for (FrameInfo info : frameInfo) {
		Array<String> frameValues = {
			"'" + Logger::formatFileTime(info.time) + "'",
			//String(std::to_string(info.idt)),
			String(std::to_string(info.sdt))
		};
		rows.append(frameValues);
	}
	insertRowsIntoDB(m_db, "Frame_Info", rows);
}

void Logger::recordPlayerActions(const Array<PlayerAction>& actions) {
	Array<RowEntry> rows;
	for (PlayerAction action : actions) {
		String actionStr = "";
		switch (action.action) {
		case Invalid: actionStr = "invalid"; break;
		case Nontask: actionStr = "non-task"; break;
		case Aim: actionStr = "aim"; break;
		case Miss: actionStr = "miss"; break;
		case Hit: actionStr = "hit"; break;
		case Destroy: actionStr = "destroy"; break;
		}
		Array<String> playerActionValues = {
		"'" + Logger::formatFileTime(action.time) + "'",
		String(std::to_string(action.viewDirection.x)),
		String(std::to_string(action.viewDirection.y)),
		String(std::to_string(action.position.x)),
		String(std::to_string(action.position.y)),
		String(std::to_string(action.position.z)),
		"'" + actionStr + "'",
		"'" + action.targetName + "'",
		};
		rows.append(playerActionValues);
	}
	insertRowsIntoDB(m_db, "Player_Action", rows);
}

void Logger::recordTargetLocations(const Array<TargetLocation>& locations) {
	Array<RowEntry> rows;
	for (const auto& loc : locations) {
		Array<String> targetTrajectoryValues = {
			"'" + Logger::formatFileTime(loc.time) + "'",
			"'" + loc.name + "'",
			String(std::to_string(loc.position.x)),
			String(std::to_string(loc.position.y)),
			String(std::to_string(loc.position.z)),
		};
		rows.append(targetTrajectoryValues);
	}
	insertRowsIntoDB(m_db, "Target_Trajectory", rows);
}

void Logger::loggerThreadEntry()
{
	std::unique_lock<std::mutex> lk(m_queueMutex);
	while (m_running) {

		m_queueCV.wait(lk, [this]{
			return !m_running || m_flushNow || getTotalQueueBytes() >= m_bufferLimit;
		});

		// Move all the queues into temporary local copies.
		// This is so we can release the lock and allow the queues to grow again while writing out the results.
		// Also allocate new storage for each.
		decltype(m_frameInfo) frameInfo;
		frameInfo.swap(m_frameInfo, frameInfo);
		m_frameInfo.reserve(frameInfo.size() * 2);

		decltype(m_playerActions) playerActions;
		playerActions.swap(m_playerActions, playerActions);
		m_playerActions.reserve(playerActions.size() * 2);

		decltype(m_questions) questions;
		questions.swap(m_questions, questions);
		m_questions.reserve(questions.size() * 2);

		decltype(m_targetLocations) targetLocations;
		targetLocations.swap(m_targetLocations, targetLocations);
		m_targetLocations.reserve(targetLocations.size() * 2);

		decltype(m_targets) targets;
		targets.swap(m_targets, targets);
		m_targets.reserve(targets.size() * 2);

		decltype(m_trials) trials;
		trials.swap(m_trials, trials);
		m_trials.reserve(trials.size() * 2);

		decltype(m_users) users;
		users.swap(m_users, users);
		m_users.reserve(users.size() * 2);

		// Unlock all the now-empty queues and write out our temporary copies
		lk.unlock();

		recordFrameInfo(frameInfo);
		recordPlayerActions(playerActions);
		recordTargetLocations(targetLocations);

		insertRowsIntoDB(m_db, "Questions", questions);
		insertRowsIntoDB(m_db, "Targets", targets);
		insertRowsIntoDB(m_db, "Users", users);
		insertRowsIntoDB(m_db, "Trials", trials);

		lk.lock();
	}
}

Logger::Logger(String filename, String subjectID, String sessionID, String description) : m_db(nullptr) {
	// secure vector capacity large enough so as to avoid memory allocation time.
	m_playerActions.reserve(5000);
	m_targetLocations.reserve(5000);
	
	createResultsFile(filename, subjectID,  sessionID, description);

	m_running = true;
	m_thread = std::thread(&Logger::loggerThreadEntry, this);
}

Logger::~Logger()
{
	{
		std::lock_guard<std::mutex> lk(m_queueMutex);
		m_running = false;
	}
	m_queueCV.notify_one();
	m_thread.join();

	closeResultsFile();
}

void Logger::flush(bool blockUntilDone)
{
	// Not implemented. Make another condition variable if this is needed.
	assert(!blockUntilDone);
	
	{
		std::lock_guard<std::mutex> lk(m_queueMutex);
		m_flushNow = true;
	}
	m_queueCV.notify_one();
}

void Logger::addTarget(String name, shared_ptr<TargetConfig> config, float refreshRate, int addedFrameLag) {
	const String type = (config->destinations.size() > 0) ? "waypoint" : "parametrized";
	const String jumpEnabled = config->jumpEnabled ? "True" : "False";
	const String modelName = config->modelSpec["filename"];
	const RowEntry targetValues = {
		"'" + name + "'",
		"'" + config->id + "'",
		"'" + type + "'",
		"'" + config->destSpace + "'",
		String(std::to_string(refreshRate)),
		String(std::to_string(addedFrameLag)),
		String(std::to_string(config->size[0])),
		String(std::to_string(config->size[1])),
		String(std::to_string(config->eccH[0])),
		String(std::to_string(config->eccH[1])),
		String(std::to_string(config->eccV[0])),
		String(std::to_string(config->eccV[1])),
		String(std::to_string(config->speed[0])),
		String(std::to_string(config->speed[1])),
		String(std::to_string(config->motionChangePeriod[0])),
		String(std::to_string(config->motionChangePeriod[1])),
		"'" + jumpEnabled + "'",
		"'" + modelName + "'"
	};
	logTargetInfo(targetValues);
}

void Logger::addQuestion(Question q, String session) {
	RowEntry rowContents = {
		"'" + session + "'",
		"'" + q.prompt + "'",
		"'" + q.result + "'"
	};
	logQuestionResult(rowContents);
}

void Logger::logUserConfig(const UserConfig& user, const String session_ref, const String position) {
	RowEntry row = {
		"'" + user.id + "'",
		"'" + session_ref + "'",
		"'" + position + "'",
		String(std::to_string(user.cmp360)),
		String(std::to_string(user.mouseDPI)),
		String(std::to_string(user.reticleIndex)),
		String(std::to_string(user.reticleScale[0])),
		String(std::to_string(user.reticleScale[1])),
		"'" + user.reticleColor[0].toString() + "'",
		"'" + user.reticleColor[1].toString() + "'",
		String(std::to_string(user.turnScale.x)),
		String(std::to_string(user.turnScale.y))
	};
	m_users.append(row);
}

void Logger::closeResultsFile() {
	sqlite3_close(m_db);
}
