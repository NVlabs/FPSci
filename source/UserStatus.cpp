#include "UserStatus.h"
#include "FPSciAnyTableReader.h"

UserSessionStatus::UserSessionStatus(const Any& any) {
	FPSciAnyTableReader reader(any);

	// Require a user ID
	reader.get("id", id, "All user status fields must include the user ID!");
	// Setup default session order, then overwrite if specified	
	sessionOrder = defaultSessionOrder;
	if (randomizeDefaults) sessionOrder.randomize();
	reader.getIfPresent("sessions", sessionOrder);			// Override the default session order if one is provided for this user
	if (sessionOrder.length() == 0) {						// Check for sessions in list
		throw format("Must provide \"sessions\" array (or default) for User ID:\"%s\" in user status!", id);
	}
}

Any UserSessionStatus::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	a["id"] = id;									// populate id
	a["sessions"] = sessionOrder;					// populate session order
	return a;
}

UserStatusTable::UserStatusTable(const Any& any) {
	FPSciAnyTableReader reader(any);

	reader.getIfPresent("currentUser", currentUser);
	reader.getIfPresent("allowRepeat", allowRepeat);
	reader.getIfPresent("sessions", defaultSessionOrder);
	UserSessionStatus::defaultSessionOrder = defaultSessionOrder;				// Set the default order here
	reader.getIfPresent("randomizeSessionOrder", randomizeDefaults);
	UserSessionStatus::randomizeDefaults = randomizeDefaults;					// Set whether default session order is randomized
	reader.get("users", userInfo, "Issue in the (required) \"users\" array from the user status file!");
	reader.getIfPresent("completedLogFilename", completedLogFilename);
}

UserStatusTable UserStatusTable::load(const String& filename, bool saveJSON) {
	UserStatusTable status;
	if (!FileSystem::exists(filename)) {						// if file not found, create a default
		UserSessionStatus user;
		user.sessionOrder = Array<String>({ "60Hz", "30Hz" });	// Add "default" sessions we add to
		status.userInfo.append(user);							// Add single "default" user
		status.currentUser = user.id;							// Set "default" user as current user
		status.save(filename, saveJSON);						// Save .any file
	}
	else status = Any::fromFile(filename);

	// Populate completed session log name if missing (use user status filename as base)
	if (status.completedLogFilename.empty()) {
		status.completedLogFilename = filename.substr(0, filename.find_last_of('.')) + ".sessions.csv";
	}

	// Load into completedSessions
	if (FileSystem::exists(status.completedLogFilename)) {
		std::ifstream log;
		log.open(status.completedLogFilename.c_str());
		std::string line;
		while (std::getline(log, line)) {
			size_t commaIdx = line.find(',');
			String userID = String(line).substr(0, commaIdx);
			String sessID = String(line).substr(commaIdx + 1, line.length() - commaIdx - 1);
			for (UserSessionStatus& info : status.userInfo) {
				if (!info.id.compare(userID)) { 
					info.completedSessions.append(sessID); 
				}
			}
		}
		log.close();
	}

	// Open for writing
	status.completedLog.open(status.completedLogFilename.c_str(), std::ios_base::app);
	if (!status.completedLog.is_open()) {
		logPrintf("Failed to open user completed session log!");
	}

	return status;
}

Any UserStatusTable::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	UserStatusTable def;
	a["settingsVersion"] = 1;						// Create a version 1 file
	a["currentUser"] = currentUser;
	if (forceAll || def.allowRepeat != allowRepeat)				a["allowRepeat"] = allowRepeat;
	if (forceAll || def.randomizeDefaults != randomizeDefaults)	a["randomizeSessionOrder"] = randomizeDefaults;
	a["sessions"] = defaultSessionOrder;
	a["users"] = userInfo;							// Include updated subject table
	return a;
}

shared_ptr<UserSessionStatus> UserStatusTable::getUserStatus(const String& id) {
	for (UserSessionStatus& user : userInfo) {
		if (!user.id.compare(id)) return std::make_shared<UserSessionStatus>(user);
	}
	return nullptr;
}

String UserStatusTable::getNextSession(String userId) {
	// Return the first valid session that has not been completed
	if (userId.empty()) { userId = currentUser; }
	shared_ptr<UserSessionStatus> status = getUserStatus(userId);
	// Handle sequence mode here (can be repeats)
	if (allowRepeat) {
		int j = 0;
		for (int i = 0; i < status->sessionOrder.size(); i++) {
			if (status->completedSessions.size() <= i) {						// If there aren't enough entries in completed sessions to support this
				return status->sessionOrder[i];
			}
			// In the future consider cases where completedSessions doesn't exactly match sessionOrder here... (fine for now?)
		}
	}
	// Default mode here (no repeats)
	else {
		for (auto sess : status->sessionOrder) {
			if (!status->completedSessions.contains(sess)) return sess;
		}
	}
	// If all sessions are complete return empty string
	return "";
}

void UserStatusTable::addCompletedSession(const String& userId, const String& sessId) {
	// Update the user info
	for (int i = 0; i < userInfo.length(); i++) {
		if (!userInfo[i].id.compare(userId)) {
			userInfo[i].completedSessions.append(sessId);
		}
	}

	// Log the completed session to the session log
	completedLog << userId.c_str() << "," << sessId.c_str() << "\n";
	completedLog.flush();
}

void UserStatusTable::validate(const Array<String>& sessions, const Array<String>& users) {
	bool noSessions = true;	// Flag to demark no sessions are present
	// Build a string list of valid options for session IDs from the experiment
	String expSessions = "[";
	for (String sess : sessions) expSessions += "\"" + sess + "\", ";
	expSessions = expSessions.substr(0, expSessions.size() - 2);
	expSessions += "]";

	// Check default sessions for valid ids
	for (String defSessId : defaultSessionOrder) {
		noSessions = false;
		if (!sessions.contains(defSessId)) {
			throw format("Default session config in user status has session with ID: \"%s\". This session ID does not appear in the experiment config file's \"sessions\" array. Valid options are: %s", defSessId, expSessions);
		}
	}

	// Check each user for valid options
	Array<String> userStatusIds;
	for (UserSessionStatus userStatus : userInfo) {
		userStatusIds.append(userStatus.id);
		// Check all of this user's sessions appear in the session array
		for (String userSessId : userStatus.sessionOrder) {
			noSessions = false;
			if (!sessions.contains(userSessId)) {
				throw format("User \"%s\" has session with ID: \"%s\" in their User Status \"sessions\" Array. This session ID does not appear in the experiment config file's \"sessions\" array. Valid options are: %s", userStatus.id, userSessId, expSessions);
			}
		}
	}

	// Check current user has a valid config
	if (currentUser.empty()) {
		throw "\"currentUser\" field is not specified in the user status file!\nIf you are migrating from an older version of FPSci, please cut the \"currentUser = ...\" line\nfrom the user config file and paste it in the user status file.";
	}
	else if (!users.contains(currentUser)) {
		throw format("Current user \"%s\" does not have a valid entry in the user config file!", currentUser);
	}

	// Check if no default/per user sessions are present
	if (noSessions) {
		throw "Found no sessions in the user status file!";
	}
}

void UserStatusTable::printToLog() const {
	for (UserSessionStatus status : userInfo) {
		String sessOrder = "";
		for (String sess : status.sessionOrder) {
			sessOrder += sess + ", ";
		}
		sessOrder = sessOrder.substr(0, sessOrder.length() - 2);
		String completedSess = "";
		for (String sess : status.completedSessions) {
			completedSess += sess + ", ";
		}
		completedSess = completedSess.substr(0, completedSess.length() - 2);

		logPrintf("Subject ID: %s\nSession Order: [%s]\nCompleted Sessions: [%s]\n", status.id.c_str(), sessOrder.c_str(), completedSess.c_str());
	}
}