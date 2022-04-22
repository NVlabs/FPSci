#include "Logger.h"
#include "Session.h"
#include "FPSciApp.h"

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

void FPSciLogger::initResultsFile(const String& filename, 
	const String& subjectID, 
	const shared_ptr<SessionConfig>& sessConfig, 
	const String& description)
{
	const bool createNewFile = !FileSystem::exists(filename);

	// Open the file
	if (sqlite3_open(filename.c_str(), &m_db)) {
		logPrintf(("Error opening log file: " + filename).c_str());					// Write an error to the log
	}

	// Create tables if a new log file is opened
	if (createNewFile) {
		createSessionsTable(sessConfig);
		createTargetTypeTable();
		createTargetsTable();
		createTrialsTable();
		createTargetTrajectoryTable();
		createPlayerActionTable();
		createFrameInfoTable();
		createQuestionsTable();
		createUsersTable();
	}

	// Add the session info to the sessions table
	m_openTimeStr = genUniqueTimestamp();
	RowEntry sessValues = {
		"'" + sessConfig->id + "'",
		"'" + m_openTimeStr + "'",
		"'" + m_openTimeStr + "'",
		"'" + subjectID + "'",
		"'" + description + "'",
		"false",
		"'0'"
	};

	// Create any table to do lookup here
	Any a = sessConfig->toAny(true);
	// Add the looked up values
	for (String name : sessConfig->logger.sessParamsToLog) { sessValues.append("'" + a[name].unparse() + "'"); }
	// add header row
	insertRowIntoDB(m_db, "Sessions", sessValues);

}

void FPSciLogger::createSessionsTable(const shared_ptr<SessionConfig>& sessConfig) {

	String createTableC = "CREATE TABLE IF NOT EXISTS Sessions ( ";
	createTableC += "session_id TEXT NOT NULL, ";
	createTableC += "start_time TEXT NOT NULL, ";
	createTableC += "end_time TEXT NOT NULL, ";
	createTableC += "subject_id TEXT NOT NULL, ";
	createTableC += "description TEXT NOT NULL, ";
	createTableC += "complete BOOLEAN NOT NULL, ";
	createTableC += "trials_complete TEXT NOT NULL";
	for (String name : sessConfig->logger.sessParamsToLog) { 
		createTableC += ", '" + name + "' TEXT NOT NULL" ; 
	}
	createTableC += ");";


	logPrintf("createSessionsTable SQL query:%s\n\n", createTableC.c_str());
	char* errmsg;
	int ret = sqlite3_exec(m_db, createTableC.c_str(), 0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		logPrintf("Error in createSessionsTable statement (%s): %s\n", createTableC, errmsg);
	}

	/// Comparison: Above is recommended all in one place, below is previous implementation

	// Session description (time and subject ID)
	//Columns sessColumns = {
	//	// format: column name, data type, sqlite modifier(s)
	//	{ "session_id", "text", "NOT NULL"},
	//	{ "start_time", "text", "NOT NULL" },
	//	{ "end_time", "text", "NOT NULL" },
	//	{ "subject_id", "text", "NOT NULL" },
	//	{ "description", "text"},
	//	{ "complete", "boolean"},
	//	{ "trials_complete", "integer" }
	//};
	//// add any user-specified parameters as headers
	//for (String name : sessConfig->logger.sessParamsToLog) { sessColumns.append({ "'" + name + "'", "text", "NOT NULL" }); }
	//createTableInDB(m_db, "Sessions", sessColumns); // no need of Primary Key for this table.

}

void FPSciLogger::updateSessionEntry(bool complete, int trialCount) {
	if (m_openTimeStr.empty()) return;		// Need an "open" session
	const String completeStr = complete ? "true" : "false";
	const String trialCountStr = String(std::to_string(trialCount));
	char* errMsg;
	String updateQ = "UPDATE Sessions SET end_time = '" + genUniqueTimestamp() + "', complete = " + completeStr + ", trials_complete = '" + trialCountStr + "' WHERE start_time = '" + m_openTimeStr + "'";
	int ret = sqlite3_exec(m_db, updateQ.c_str(), 0, 0, &errMsg);
	if (ret != SQLITE_OK) { logPrintf("Error in UPDATE statement (%s): %s\n", updateQ, errMsg); }
}

void FPSciLogger::createTargetTypeTable() {
	// Targets Type table (written once per session)
	Columns targetTypeColumns = {
		{ "target_type", "text" },
		{ "motion_type", "text"},
		{ "dest_space", "text"},
		{ "min_size", "real"},
		{ "max_size", "real"},
		{ "symmetric_ecc_h", "boolean" },
		{ "symmetric_ecc_v", "boolean" },
		{ "min_ecc_h", "real" },
		{ "min_ecc_v", "real" },
		{ "max_ecc_h", "real" },
		{ "max_ecc_v", "real" },
		{ "min_speed", "real" },
		{ "max_speed", "real" },
		{ "min_motion_change_period", "real" },
		{ "max_motion_change_period", "real" },
		{ "jump_enabled", "boolean" },
		{ "model_file", "text" }
	};
	createTableInDB(m_db, "Target_Types", targetTypeColumns); // Primary Key needed for this table.
}

// Log target parameters into Target_Types table
void FPSciLogger::logTargetTypes(const Array<shared_ptr<TargetConfig>>& targets) {
	Array<RowEntry> rows;
	for (auto config : targets) {
		const String type = (config->destinations.size() > 0) ? "waypoint" : "parametrized";
		const String modelName = config->modelSpec["filename"];
		const RowEntry targetTypeRow = {
			"'" + config->id + "'",
			"'" + type + "'",
			"'" + config->destSpace + "'",
			String(std::to_string(config->size[0])),
			String(std::to_string(config->size[1])),
			config->symmetricEccH ? "true" : "false",
			config->symmetricEccV ? "true" : "false",
			String(std::to_string(config->eccH[0])),
			String(std::to_string(config->eccH[1])),
			String(std::to_string(config->eccV[0])),
			String(std::to_string(config->eccV[1])),
			String(std::to_string(config->speed[0])),
			String(std::to_string(config->speed[1])),
			String(std::to_string(config->motionChangePeriod[0])),
			String(std::to_string(config->motionChangePeriod[1])),
			config->jumpEnabled ? "true" : "false",
			"'" + modelName + "'"
		};
		rows.append(targetTypeRow);
	}
	insertRowsIntoDB(m_db, "Target_Types", rows);
}

void FPSciLogger::createTargetsTable() {
	// Targets table
	Columns targetColumns = {
		{ "target_id", "text" },
		{ "target_type", "text"},
		{ "spawn_time", "text"},
		{ "size", "real"},
		{ "spawn_ecc_h", "real"},
		{ "spawn_ecc_v", "real"},
	};
	createTableInDB(m_db, "Targets", targetColumns);
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

void FPSciLogger::createTrialsTable() {
	// Trials table
	Columns trialColumns = {
		{ "session_id", "text" },
		{ "trial_id", "integer" },
		{ "trial_index", "integer"},
		{ "block_id", "text"},
		{ "start_time", "text" },
		{ "end_time", "text" },
		{ "pretrial_duration", "real" },
		{ "task_execution_time", "real" },
		{ "destroyed_targets", "real" },
		{ "total_targets", "real" }
	};
	createTableInDB(m_db, "Trials", trialColumns);
}

void FPSciLogger::createTargetTrajectoryTable() {
	// Target_Trajectory, only need to create the table.
	Columns targetTrajectoryColumns = {
		{ "time", "text" },
		{ "target_id", "text"},
		{ "state", "text"},
		{ "position_x", "real" },
		{ "position_y", "real" },
		{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);
}

void FPSciLogger::recordTargetLocations(const Array<TargetLocation>& locations) {
	Array<RowEntry> rows;
	for (const auto& loc : locations) {
		String stateStr = presentationStateToString(loc.state);
		Array<String> targetTrajectoryValues = {
			"'" + FPSciLogger::formatFileTime(loc.time) + "'",
			"'" + loc.name + "'",
			"'" + stateStr + "'",
			String(std::to_string(loc.position.x)),
			String(std::to_string(loc.position.y)),
			String(std::to_string(loc.position.z)),
		};
		rows.append(targetTrajectoryValues);
	}
	insertRowsIntoDB(m_db, "Target_Trajectory", rows);
}

void FPSciLogger::createPlayerActionTable() {
	// Player_Action table
	Columns viewTrajectoryColumns = {
		{ "time", "text" },
		{ "position_az", "real" },
		{ "position_el", "real" },
		{ "position_x", "real"},
		{ "position_y", "real"},
		{ "position_z", "real"},
		{ "state", "text"},
		{ "event", "text" },
		{ "target_id", "text" },
	};
	createTableInDB(m_db, "Player_Action", viewTrajectoryColumns);
}

void FPSciLogger::recordPlayerActions(const Array<PlayerAction>& actions) {
	Array<RowEntry> rows;
	for (PlayerAction action : actions) {
		String stateStr = presentationStateToString(action.state);

		String actionStr = "";
		switch (action.action) {
		case FireCooldown: actionStr = "fireCooldown"; break;
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
		"'" + stateStr + "'",
		"'" + actionStr + "'",
		"'" + action.targetName + "'",
		};
		rows.append(playerActionValues);
	}
	insertRowsIntoDB(m_db, "Player_Action", rows);
}

void FPSciLogger::createFrameInfoTable() {
	// Frame_Info table
	Columns frameInfoColumns = {
		{"time", "text"},
		//{"idt", "real"},
		{"sdt", "real"},
	};
	createTableInDB(m_db, "Frame_Info", frameInfoColumns);
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

void FPSciLogger::createQuestionsTable() {
	// Questions table
	Columns questionColumns = {
		{"time", "text"},
		{"session_id", "text"},
		{"question", "text"},
		{"responseArray", "text"},
		{"keyArray", "text"},
		{"presentedResponses", "text"},
		{"response", "text"}
	};
	createTableInDB(m_db, "Questions", questionColumns);
}

void FPSciLogger::addQuestion(Question q, String session, const shared_ptr<DialogBase>& dialog) {
	const String time = genUniqueTimestamp();
	const String optStr = Any(q.options).unparse();
	const String keyStr = Any(q.optionKeys).unparse();
	String orderStr = "";
	if (q.type == Question::Type::MultipleChoice || q.type == Question::Type::Rating) {
		orderStr = Any(dynamic_pointer_cast<SelectionDialog>(dialog)->options()).unparse();
	}
	RowEntry rowContents = {
		"'" + time + "'",
		"'" + session + "'",
		"'" + q.prompt + "'",
		"'" + optStr + "'",
		"'" + keyStr + "'",
		"'" + orderStr + "'",
		"'" + q.result + "'"
	};
	logQuestionResult(rowContents);
}

void FPSciLogger::createUsersTable() {
	// Users table
	Columns userColumns = {
		{"subject_id", "text"},
		{"session_id", "text"},
		{"time", "text"},
		{"cmp360", "real"},
		{"mouse_deg_per_mm", "real"},
		{"mouse_dpi", "real"},
		{"reticle_index", "int"},
		{"min_reticle_scale", "real"},
		{"max_reticle_scale", "real"},
		{"min_reticle_color", "text"},
		{"max_reticle_color", "text"},
		{"reticle_change_time", "real"},
		{"user_turn_scale_x", "real"},
		{"user_turn_scale_y", "real"},
		{"sess_turn_scale_x", "real"},
		{"sess_turn_scale_y", "real"},
		{"sensitivity_x", "real"},
		{"sensitivity_y", "real"}
	};	
	createTableInDB(m_db, "Users", userColumns);
}

void FPSciLogger::logUserConfig(const UserConfig& user, const String& sessId, const Vector2& sessTurnScale) {
	if (!m_config.logUsers) return;
	// Collapse Y-inversion into per-user turn scale (no need to complicate the log)
	const float userYTurnScale = user.invertY ? -user.turnScale.y : user.turnScale.y;
	const float cmp360 = 36.f / (float)user.mouseDegPerMm;
	const Vector2 sensitivity = cmp360 * user.turnScale * sessTurnScale;
	const String time = genUniqueTimestamp();

	RowEntry row = {
		"'" + user.id + "'",
		"'" + sessId + "'",
		"'" + time + "'",
		String(std::to_string(cmp360)),
		String(std::to_string(user.mouseDegPerMm)),
		String(std::to_string(user.mouseDPI)),
		String(std::to_string(user.reticle.index)),
		String(std::to_string(user.reticle.scale[0])),
		String(std::to_string(user.reticle.scale[1])),
		"'" + user.reticle.color[0].toString() + "'",
		"'" + user.reticle.color[1].toString() + "'",
		String(std::to_string(user.reticle.changeTimeS)),
		String(std::to_string(user.turnScale.x)),
		String(std::to_string(userYTurnScale)),
		String(std::to_string(sessTurnScale.x)),
		String(std::to_string(sessTurnScale.y)),
		String(std::to_string(sensitivity.x)),
		String(std::to_string(sensitivity.y))
	};
	m_users.append(row);
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
	) : m_db(nullptr), m_config(sessConfig->logger)
{
	// Reserve some space in these arrays here
	m_playerActions.reserve(5000);
	m_targetLocations.reserve(5000);
	
	// Create the results file
	initResultsFile(filename, subjectID,  sessConfig, description);

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

void FPSciLogger::closeResultsFile() {
	sqlite3_close(m_db);
}