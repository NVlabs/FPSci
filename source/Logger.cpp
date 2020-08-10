#include "Logger.h"
#include "Session.h"

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
String FPSciLogger::genUniqueTimestamp() {
	return formatFileTime(getFileTime());
}

FILETIME FPSciLogger::getFileTime() {
	FILETIME ft;
	GetSystemTimePreciseAsFileTime(&ft);
	return ft;
}

String FPSciLogger::formatFileTime(FILETIME ft) {
	unsigned long long usecsinceepoch = (static_cast<unsigned long long>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;		// Get time since epoch in usec
	int usec = usecsinceepoch % 1000000;

	SYSTEMTIME datetime;
	FileTimeToSystemTime(&ft, &datetime);

	char tmCharArray[30] = { 0 };
	sprintf(tmCharArray, "%04d-%02d-%02d %02d:%02d:%02d.%06d", datetime.wYear, datetime.wMonth, datetime.wDay, datetime.wHour, datetime.wMinute, datetime.wSecond, usec);
	std::string timeStr(tmCharArray);
	return String(timeStr);
}


String FPSciLogger::genFileTimestamp() {
	_SYSTEMTIME t;
	GetSystemTime(&t);
	char tmCharArray[30] = { 0 };
	sprintf(tmCharArray, "%04d_%02d_%02d-%02d_%02d_%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	std::string timeStr(tmCharArray);
	return String(timeStr);
}

void FPSciLogger::createResultsFile(const String& filename, 
	const String& subjectID, 
	const shared_ptr<SessionConfig>& sessConfig, 
	const String& description)
{
	// create a unique file name (can bring this back if desired)
	String timeStr = genUniqueTimestamp();

	// create the file
	if (sqlite3_open(filename.c_str(), &m_db)) {
		// Write an error to the log
		logPrintf(("Error creating log file: " + filename).c_str());
	}

	// Session description (time and subject ID)
	Columns sessColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "sessionID", "text", "NOT NULL"},
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
			{ "appendingDescription", "text"}
	};
	// add any user-specified parameters as headers
	for (String name : sessConfig->logger.sessParamsToLog) { sessColumns.append({ "'" + name + "'", "text", "NOT NULL" }); }
	createTableInDB(m_db, "Sessions", sessColumns); // no need of Primary Key for this table.

	// populate table
	RowEntry sessValues = {
		"'" + sessConfig->id + "'",
		"'" + timeStr + "'",
		"'" + subjectID + "'",
		"'" + description + "'"
	};
	// Create any table to do lookup here
	Any a = sessConfig->toAny(true);
	// Add the looked up values
	for (String name : sessConfig->logger.sessParamsToLog) { sessValues.append("'" + a[name].unparse() + "'"); }
	// add header row
	insertRowIntoDB(m_db, "Sessions", sessValues);

	// Targets Type table (written once per session)
	Columns targetTypeColumns = {
			{ "name", "text" },
			{ "motion_type", "text"},
			{ "dest_space", "text"},
			{ "min_size", "real"},
			{ "max_size", "real"},
			{ "min_ecc_h", "real" },
			{ "min_ecc_v", "real" },
			{ "max_ecc_h", "real" },
			{ "max_ecc_v", "real" },
			{ "min_speed", "real" },
			{ "max_speed", "real" },
			{ "min_motion_change_period", "real" },
			{ "max_motion_change_period", "real" },
			{ "jump_enabled", "text" },
			{ "model_file", "text" }
	};
	createTableInDB(m_db, "Target_Types", targetTypeColumns); // Primary Key needed for this table.

	// Targets table
	Columns targetColumns = {
		{ "name", "text" },
		{ "target_type_name", "text"},
		{ "spawn_time", "text"},
		{ "size", "real"},
		{ "spawn_ecc_h", "real"},
		{ "spawn_ecc_v", "real"},
	};
	createTableInDB(m_db, "Targets", targetColumns);

	// Trials table
	Columns trialColumns = {
			{ "trial_id", "integer" },
			{ "session_id", "text" },
			{ "session_mode", "text" },
			{ "block_id", "text"},
			{ "start_time", "text" },
			{ "end_time", "text" },
			{ "task_execution_time", "real" },
			{ "destroyed_targets", "real" },
			{ "total_targets", "real" }
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// Target_Trajectory, only need to create the table.
	Columns targetTrajectoryColumns = {
			{ "time", "text" },
			{ "target_id", "text"},
			{ "position_x", "real" },
			{ "position_y", "real" },
			{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);

	// Player_Action table
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

	// Frame_Info table
	Columns frameInfoColumns = {
			{"time", "text"},
			//{"idt", "real"},
			{"sdt", "real"},
	};
	createTableInDB(m_db, "Frame_Info", frameInfoColumns);

	// Questions table
	Columns questionColumns = {
		{"session", "text"},
		{"question", "text"},
		{"response", "text"}
	};
	createTableInDB(m_db, "Questions", questionColumns);

	// Users table
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
		{"reticleChangeTime", "real"},
		{"userTurnScaleX", "real"},
		{"userTurnScaleY", "real"},
		{"sessTurnScaleX", "real"},
		{"sessTurnScaleY", "real"},
		{"sensitivityX", "real"},
		{"sensitivityY", "real"}
	};
	createTableInDB(m_db, "Users", userColumns);
}

void FPSciLogger::recordFrameInfo(const Array<FrameInfo>& frameInfo) {
	Array<RowEntry> rows;
	for (FrameInfo info : frameInfo) {
		Array<String> frameValues = {
			"'" + FPSciLogger::formatFileTime(info.time) + "'",
			//String(std::to_string(info.idt)),
			String(std::to_string(info.sdt))
		};
		rows.append(frameValues);
	}
	insertRowsIntoDB(m_db, "Frame_Info", rows);
}

void FPSciLogger::recordPlayerActions(const Array<PlayerAction>& actions) {
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
		"'" + FPSciLogger::formatFileTime(action.time) + "'",
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

void FPSciLogger::recordTargetLocations(const Array<TargetLocation>& locations) {
	Array<RowEntry> rows;
	for (const auto& loc : locations) {
		Array<String> targetTrajectoryValues = {
			"'" + FPSciLogger::formatFileTime(loc.time) + "'",
			"'" + loc.name + "'",
			String(std::to_string(loc.position.x)),
			String(std::to_string(loc.position.y)),
			String(std::to_string(loc.position.z)),
		};
		rows.append(targetTrajectoryValues);
	}
	insertRowsIntoDB(m_db, "Target_Trajectory", rows);
}

void FPSciLogger::loggerThreadEntry()
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

FPSciLogger::FPSciLogger(const String& filename, 
	const String& subjectID, 
	const shared_ptr<SessionConfig>& sessConfig, 
	const String& description 
	) : m_db(nullptr) 
{
	// Reserve some space in these arrays here
	m_playerActions.reserve(5000);
	m_targetLocations.reserve(5000);
	
	// Create the results file
	createResultsFile(filename, subjectID,  sessConfig, description);

	// Thread management
	m_running = true;
	m_thread = std::thread(&FPSciLogger::loggerThreadEntry, this);
}

FPSciLogger::~FPSciLogger()
{
	{
		std::lock_guard<std::mutex> lk(m_queueMutex);
		m_running = false;
	}
	m_queueCV.notify_one();
	m_thread.join();

	closeResultsFile();
}

void FPSciLogger::flush(bool blockUntilDone)
{
	// Not implemented. Make another condition variable if this is needed.
	assert(!blockUntilDone);
	
	{
		std::lock_guard<std::mutex> lk(m_queueMutex);
		m_flushNow = true;
	}
	m_queueCV.notify_one();
}

// Log target parameters into Target_Types table
void FPSciLogger::logTargetTypes(const Array<shared_ptr<TargetConfig>>& targets) {
	Array<RowEntry> rows;
	for (auto config : targets) {
		const String type = (config->destinations.size() > 0) ? "waypoint" : "parametrized";
		const String jumpEnabled = config->jumpEnabled ? "True" : "False";
		const String modelName = config->modelSpec["filename"];
		const RowEntry targetTypeRow = {
			"'" + config->id + "'",
			"'" + type + "'",
			"'" + config->destSpace + "'",
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
		rows.append(targetTypeRow);
	}
	insertRowsIntoDB(m_db, "Target_Types", rows);
}

void FPSciLogger::addTarget(const String& name, const shared_ptr<TargetConfig>& config, const String& spawnTime, const float& size, const Point2& spawnEcc) {
	const RowEntry targetValues = {
		"'" + name + "'",
		"'" + config->id + "'",
		"'" + spawnTime + "'",
		String(std::to_string(size)),
		String(std::to_string(spawnEcc.x)),
		String(std::to_string(spawnEcc.y)),
	};
	logTargetInfo(targetValues);
}

void FPSciLogger::addQuestion(Question q, String session) {
	RowEntry rowContents = {
		"'" + session + "'",
		"'" + q.prompt + "'",
		"'" + q.result + "'"
	};
	logQuestionResult(rowContents);
}

void FPSciLogger::logUserConfig(const UserConfig& user, const String& session_ref, const String& position, const Vector2& sessTurnScale) {
	// Collapse Y-inversion into per-user turn scale (no need to complicate the log)
	const float userYTurnScale = user.invertY ? -user.turnScale.y : user.turnScale.y;
	const Vector2 sensitivity = (float)user.cmp360 * user.turnScale * sessTurnScale;

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
		String(std::to_string(user.reticleChangeTimeS)),
		String(std::to_string(user.turnScale.x)),
		String(std::to_string(userYTurnScale)),
		String(std::to_string(sessTurnScale.x)),
		String(std::to_string(sessTurnScale.y)),
		String(std::to_string(sensitivity.x)),
		String(std::to_string(sensitivity.y))
	};
	m_users.append(row);
}

void FPSciLogger::closeResultsFile() {
	sqlite3_close(m_db);
}
