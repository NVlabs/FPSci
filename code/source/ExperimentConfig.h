#pragma once

#include <G3D/G3D.h>
#include "SingleThresholdMeasurement.h"

/** Configure how the application should start */
class StartupConfig {
public:
    bool playMode = true;
    String experimentConfigPath = "";
    String userConfigPath = "";

    StartupConfig() {};

    StartupConfig(const Any& any) {
        int settingsVersion = 1;
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("playMode", playMode);
            reader.getIfPresent("experimentConfigPath", experimentConfigPath);
            reader.getIfPresent("userConfigPath", userConfigPath);
            break;
        default:
            debugPrintf("Settings version '%d' not recognized in StartupConfig.\n", settingsVersion);
            break;
        }
    }

    Any toAny(const bool forceAll = true) const {
        Any a(Any::TABLE);
        a["playMode"] = playMode;
        a["experimentConfigPath"] = experimentConfigPath;
        a["userConfigPath"] = userConfigPath;
        return a;
    }

    /** filename with given path to experiment config file */
    String experimentConfig() {
        return experimentConfigPath + "experimentconfig.Any";
    }

    /** filename with given path to user config file */
    String userConfig() {
        return userConfigPath + "userconfig.Any";
    }
};

// This is a write-only structure to log information affiliated with a system
class SystemConfig {
public:
	// Output/runtime read parameters
	String cpuName;
	int coreCount;
	String gpuName;
	long memCapacityMB;
	String displayName;
	int displayXRes;
	int displayYRes;
	int displayXSize;
	int displayYSize;

	bool hasLogger;
	String loggerComPort;
	bool hasSync;
	String syncComPort;

	SystemConfig() {};

	SystemConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("HasLogger", hasLogger);
			reader.getIfPresent("LoggerComPort", loggerComPort);
			reader.get("HasSync", hasSync);
			reader.getIfPresent("SyncComPort", syncComPort);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SystemConfig.\n", settingsVersion);
			break;
		}
		// Get the system info
		getSystemInfo();		
	}

	Any toAny(const bool forceAll = true) const{
		Any a(Any::TABLE);
		a["CPU"] = cpuName;
		a["GPU"] = gpuName;
		a["CoreCount"] = coreCount;
		a["MemoryCapacityMB"] = memCapacityMB;
		a["DisplayName"] = displayName;
		a["DisplayResXpx"] = displayXRes;
		a["DisplayResYpx"] = displayYRes;
		a["DisplaySizeXmm"] = displayXSize;
		a["DisplaySizeYmm"] = displayYSize;
		a["HasLogger"] = hasLogger;
		a["LoggerComPort"] = loggerComPort;
		a["HasSync"] = hasSync;
		a["SyncComPort"] = syncComPort;
		return a;
	}

	static SystemConfig load() {
		if (!FileSystem::exists("systemconfig.Any")) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEsystemconfig.Any"), "systemconfig.Any");
		}
		return Any::fromFile(System::findDataFile("systemconfig.Any"));
	}

	void getSystemInfo(void) {
		SystemConfig system;

		// Get CPU name string
		int cpuInfo[4] = { -1 };
		unsigned nExIds, i = 0;
		char cpuBrandString[0x40];
		__cpuid(cpuInfo, 0x80000000);
		nExIds = cpuInfo[0];
		for (unsigned int i = 0x80000000; i <= nExIds; i++) {
			__cpuid(cpuInfo, i);
			// Interpret CPU brand string
			switch (i) {
			case 0x80000002:
				memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
				break;
			case 0x80000003:
				memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
				break;
			case 0x80000004:
				memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
				break;
			default:
				logPrintf("Couldn't get system info...\n");
			}
		}
		cpuName = cpuBrandString;

		// Get CPU core count
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		coreCount = sysInfo.dwNumberOfProcessors;

		// Get memory size
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		memCapacityMB = (long)(statex.ullTotalPhys / (1024 * 1024));

		// Get GPU name string
		String gpuVendor = String((char*)glGetString(GL_VENDOR)).append(" ");
		String gpuRenderer = String((char*)glGetString(GL_RENDERER));
		gpuName = gpuVendor.append(gpuRenderer);

		// Get display information (monitor name)
		/*DISPLAY_DEVICE dd;
		int deviceIndex = 0;
		int monitorIndex = 0;
		EnumDisplayDevices(0, deviceIndex, &dd, 0);
		std::string deviceName = dd.DeviceName;
		EnumDisplayDevices(deviceName.c_str(), monitorIndex, &dd, 0);
		displayName = String(dd.DeviceString);*/
		displayName = String("TODO");

		// Get screen resolution
		displayXRes = GetSystemMetrics(SM_CXSCREEN);
		displayYRes = GetSystemMetrics(SM_CYSCREEN);

		// Get display size
		HWND const hwnd = 0;
		HDC const hdc = GetDC(hwnd);
		assert(hdc);
		displayXSize = GetDeviceCaps(hdc, HORZSIZE);
		displayYSize = GetDeviceCaps(hdc, VERTSIZE);
	}

	void printSystemInfo() {
		// Print system info to log
		logPrintf("System Info: \n\tProcessor: %s\n\tCore Count: %d\n\tMemory: %dMB\n\tGPU: %s\n\tDisplay: %s\n\tDisplay Resolution: %d x %d (px)\n\tDisplay Size: %d x %d (mm)\n",
			cpuName, coreCount, memCapacityMB, gpuName, displayName, displayXRes, displayYRes, displayXSize, displayYSize);
		logPrintf("Logger Present: %s\nLogger COM Port: %s\nSync Card Present: %s\nSync COM Port: %s\n",
			hasLogger ? "True" : "False", loggerComPort, hasSync ? "True" : "False", syncComPort);
	}
};

class UserConfig {
public:
    String id = "anon";						// Subject ID (as recorded in output DB)
    double mouseDPI = 800.0;				// Mouse DPI setting
    double cmp360 = 12.75;					// Mouse sensitivity, reported as centimeters per 360�
	int currentSession = 0;					// Currently selected session
    UserConfig() {}

    UserConfig(const Any& any) {
        int settingsVersion = 1; // used to allow different version numbers to be loaded differently
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("id", id);
            reader.getIfPresent("mouseDPI", mouseDPI);
            reader.getIfPresent("cmp360", cmp360);
			break;
        default:
            debugPrintf("Settings version '%d' not recognized in UserConfig.\n", settingsVersion);
            break;
        }
        // fine to have extra entries not read
        //reader.verifyDone();
    }

	// Simple method for conversion to Any (writing output file)
	Any toAny(const bool forceAll=true) const {
		Any a(Any::TABLE);
		a["id"] = id;										// Include subject ID
		a["mouseDPI"] = mouseDPI;							// Include mouse DPI
		a["cmp360"] = cmp360;								// Include cm/360
		return a;
	}
};

class UserTable {
public:
	String currentUser = "None";
	Array<UserConfig> users = {};

	UserTable() {};

	UserTable(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("currentUser", currentUser);
			reader.get("users", users);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserTable.\n", settingsVersion);
			break;
		}
	}

    UserConfig* getCurrentUser() {
        for (int i = 0; i < users.length(); ++i) {
            if (!users[i].id.compare(currentUser)) return &(users[i]);
        }
        // return the first user by default and set the value
        currentUser = users[0].id;
        return &(users[0]);
    }

    int getCurrentUserIndex() {
        for (int i = 0; i < users.length(); ++i) {
            if (!users[i].id.compare(currentUser)) return i;
        }
        // return the first user by default
        return 0;
    }

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = 1;						// Create a version 1 file
		a["currentUser"] = currentUser;					// Include current subject ID
		a["users"] = users;								// Include updated subject table
		return a;
	}

	shared_ptr<UserConfig> getUserById(String id) {
		for (UserConfig user : users) {
			if (!user.id.compare(id)) return std::make_shared<UserConfig>(user);
		}
		return nullptr;
	}

	Array<String> getIds() {
		Array<String> ids;
		for (UserConfig user : users) ids.append(user.id);
		return ids;
	}

	// Simple rotine to get the user configuration from file
	static Any load(String filename) {
		// load user setting from file
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEuserconfig.Any").c_str(), "userconfig.Any");
		}
		return Any::fromFile(System::findDataFile(filename));
	}
};

class UserSessionStatus {
public:
	String id;
	Array<String> sessionOrder = {};
	Array<String> completedSessions = {};

	UserSessionStatus() {}

	UserSessionStatus(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.getIfPresent("sessions", sessionOrder);
			reader.getIfPresent("completedSessions", completedSessions);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserSessionStatus.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["id"] = id;									// populate id
		a["sessions"] = sessionOrder;					// populate session order
		a["completedSessions"] = completedSessions; 	// Include updated subject table
		return a;
	}
};

class UserStatusTable {
public:
	Array<UserSessionStatus> userInfo = {};

	UserStatusTable() {}

	UserStatusTable(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("users", userInfo);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserStatus.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = 1;						// Create a version 1 file
		a["users"] = userInfo;							// Include updated subject table
		return a;
	}

	// Get the experiment config from file
	static UserStatusTable load(void) {
		if (!FileSystem::exists("userstatus.Any")) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEuserstatus.Any"), "userstatus.Any");
		}
		return Any::fromFile(System::findDataFile("userstatus.Any"));
	}

	shared_ptr<UserSessionStatus> getUserStatus(String id) {
		for (UserSessionStatus user : userInfo) {
			if (!user.id.compare(id)) return std::make_shared<UserSessionStatus>(user);
		}
		return nullptr;
	}

	String getNextSession(String userId) {
		// Return the first valid session that has not been completed
		shared_ptr<UserSessionStatus> status = getUserStatus(userId);
		for (auto sess : status->sessionOrder) {
			if (!status->completedSessions.contains(sess)) return sess;
		}
		// If all sessions are complete return empty string
		return "";
	}

	void addCompletedSession(String userId, String sessId) {
		for (int i = 0; i < userInfo.length(); i++) {
			if (!userInfo[i].id.compare(userId)) {
				userInfo[i].completedSessions.append(sessId);
			}
		}
	}
};


class ReactionConfig {
public:
	String id;												// ID to refer to this reaction config from trial runs table
	float minimumForeperiod = 1.5f;							// Minimum time to wait before a reaction time transition
	Array<float> intensities;								// Intensities
	Array<int> intensityCounts;								// Count of each intensity

	ReactionConfig() : minimumForeperiod(1.5f) {}

	ReactionConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("id", id);
			reader.getIfPresent("minimumForeperiod", minimumForeperiod);
			reader.get("intensities", intensities);
			reader.get("intensityCounts", intensityCounts);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ReactionConfig.\n", settingsVersion);
			break;
		}
	}
};

class TargetConfig {
public:
	String id;												// Trial ID to indentify affiliated trial runs
	bool elevLocked = false;								// Elevation locking
	Array<float> distance = { 30.0f, 40.0f };				// Distance to the target
	Array<float> motionChangePeriod = { 1.0f, 1.0f };		// Range of motion change period in seconds
	Array<float> speed = { 0.0f, 5.5f };					// Range of angular velocities for target
	Array<float> eccH = { 5.0f, 15.0f };					// Range of initial horizontal eccentricity
	Array<float> eccV = { 0.0f, 2.0f };						// Range of initial vertical eccentricity
	Array<float> visualSize = { 0.02f, 0.02f };				// Visual size of the target (in degrees)
	bool jumpEnabled = false;
	Array<float> jumpPeriod = { 2.0f, 2.0f };
	Array<float> jumpSpeed = { 2.0f, 5.5f };
	Array<float> accelGravity = { 9.8f, 9.8f };
	//Array<Vector3> path;		// Unused, to dictate a motion path...

	TargetConfig() {}

	TargetConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.getIfPresent("elevationLocked", elevLocked);
			reader.getIfPresent("distance", distance);
			reader.getIfPresent("motionChangePeriod", motionChangePeriod);
			reader.getIfPresent("speed", speed);
			reader.getIfPresent("visualSize", visualSize);
			reader.getIfPresent("eccH", eccH);
			reader.getIfPresent("eccV", eccV);
			reader.getIfPresent("jumpEnabled", jumpEnabled);
			reader.getIfPresent("jumpSpeed", jumpSpeed);
			reader.getIfPresent("jumpPeriod", jumpPeriod);
			reader.getIfPresent("accelGravity", accelGravity);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in TargetConfig.\n", settingsVersion);
			break;
		}
		//reader.verifyDone();
	}
};

// Trial count class (optional for alternate TargetConfig/count table lookup)
class TrialCount {
public:
	String id;
	unsigned int count = 0;

	TrialCount() {};

	TrialCount(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.get("count", count);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}
};

class SessionConfig {
public:
	String id;
	float	frameRate = 240.0f;					// Target (goal) frame rate (in Hz)
	unsigned int frameDelay = 0;				// Integer frame delay (in frames)
	String  expMode = "training";				// String indicating whether session is training or real
	String	selectionOrder = "random";			// "Random", "Round Robbin", "In Order"
	Array<TrialCount> trials;

	SessionConfig() : frameRate(240.0f), frameDelay(0), selectionOrder("random") {}

	SessionConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("id", id);
			reader.getIfPresent("frameRate", frameRate);
			reader.getIfPresent("frameDelay", frameDelay);
			reader.getIfPresent("selectionOrder", selectionOrder);
			reader.getIfPresent("expMode", expMode);
			reader.get("trials", trials);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
		//reader.verifyDone();
	}


};

class ExperimentConfig {
public:
	String	taskType = "reaction";					// "Reaction" or "Target"
	String	appendingDescription = "ver0";			// Short text field for description
	String  sceneName = "eSports Simple Hallway";	// For target experiment
	float feedbackDuration = 1.0f;
	float readyDuration = 0.5f;
	float taskDuration = 100000.0f;
	int maxClicks = 10000;							// Maximum number of clicks to allow in a trial
	float fireRate = 100.0;							// Maximum fire rate
	Array<SessionConfig> sessions;					// Array of sessions
	String sessionOrder = "random";					// Order in which to run sessions?
	Array<TargetConfig> targets;					// Array of trial configs
	Array<ReactionConfig> reactions;				// Array of reaction configs
	bool renderDecals = true;						// If bullet decals are on
	bool renderMuzzleFlash = false;					// Muzzle flash
    bool renderWeaponStatus = true;                 // Display weapon cooldown

	ExperimentConfig() {}
	
	ExperimentConfig(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("taskType", taskType);
			reader.getIfPresent("appendingDescription", appendingDescription);
			reader.getIfPresent("sceneName", sceneName);
			reader.getIfPresent("sessionOrder", sessionOrder);
			reader.get("sessions", sessions);
			reader.get("targets", targets);
			reader.getIfPresent("feedbackDuration", feedbackDuration);
			reader.getIfPresent("readyDuration", readyDuration);
			reader.getIfPresent("taskDuration", taskDuration);
			reader.getIfPresent("maxClicks", maxClicks);
			reader.getIfPresent("fireRate", fireRate);
			reader.getIfPresent("renderDecals", renderDecals);
            reader.getIfPresent("renderMuzzleFlash", renderMuzzleFlash);
            reader.getIfPresent("renderWeaponStatus", renderWeaponStatus);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
		// fine to have extra entries not read
		//reader.verifyDone();
	}



	// Start of enumeration for task type
	enum taskType {
		reaction = 0,
		target = 1
	};
	//const String taskTypes[2] = { "reaction", "target" };

	Array<String> getSessIds() {
		Array<String> ids;
		for (auto sess : sessions) {
			ids.append(sess.id);
		}
		return ids;
	}

	shared_ptr<SessionConfig> getSessionConfigById(String id) {
		for (int i = 0; i < sessions.size(); i++) {
			if (!sessions[i].id.compare(id)) return std::make_shared<SessionConfig>(sessions[i]);
		}
		return nullptr;
	}

	int getSessionIndex(String id) {
		for (int i = 0; i < sessions.size(); i++) {
			auto aa = sessions[i];
			if (!sessions[i].id.compare(id)) return i;
		}
        // wasn't found, probably will segfault
        return -1;
	}
	
	// Get a pointer to a target config by ID
	shared_ptr<TargetConfig> getTargetConfigById(String id) {
		for (int i = 0; i < targets.size(); i++) {
			if (!targets[i].id.compare(id)) return std::make_shared<TargetConfig>(targets[i]);
		}
		return nullptr;
	}

	// Get a pointer to a reaction config by ID
	shared_ptr<ReactionConfig> getReactionConfigById(String id) {
		for (int i = 0; i < reactions.size(); i++) {
			if (!reactions[i].id.compare(id)) return std::make_shared<ReactionConfig>(reactions[i]);
		}
		return nullptr;
	}

	Array<Param> getTargetExpConditions(String id) {
		int idx = getSessionIndex(id);
		return getTargetExpConditions(idx);
	}

	// This is a kludge to quickly create experiment conditions w/ appropriate parameters
	Array<Param> getTargetExpConditions(int sessionIndex) {
		Array<Param> params;
		for (int j = 0; j < sessions[sessionIndex].trials.size(); j++) {
			String id = sessions[sessionIndex].trials[j].id;
			// Append training trial
			Param p;
			p.add("minEccH", getTargetConfigById(id)->eccH[0]);
			p.add("minEccV", getTargetConfigById(id)->eccV[0]);
			p.add("maxEccH", getTargetConfigById(id)->eccH[1]);
			p.add("maxEccV", getTargetConfigById(id)->eccV[1]);
			p.add("targetFrameRate", sessions[sessionIndex].frameRate);
			p.add("targetFrameLag", (float)sessions[sessionIndex].frameDelay);
			p.add("minVisualSize", getTargetConfigById(id)->visualSize[0]);
			p.add("maxVisualSize", getTargetConfigById(id)->visualSize[0]);
			p.add("minMotionChangePeriod", getTargetConfigById(id)->motionChangePeriod[0]);
			p.add("maxMotionChangePeriod", getTargetConfigById(id)->motionChangePeriod[0]);
			p.add("minSpeed", getTargetConfigById(id)->speed[0]);
			p.add("maxSpeed", getTargetConfigById(id)->speed[1]);
			p.add("minDistance", getTargetConfigById(id)->distance[0]);
			p.add("maxDistance", getTargetConfigById(id)->distance[1]);
			p.add("minJumpPeriod", getTargetConfigById(id)->jumpPeriod[0]);
			p.add("maxJumpPeriod", getTargetConfigById(id)->jumpPeriod[1]);
			p.add("minJumpSpeed", getTargetConfigById(id)->jumpSpeed[0]);
			p.add("maxJumpSpeed", getTargetConfigById(id)->jumpSpeed[1]);
			p.add("minGravity", getTargetConfigById(id)->accelGravity[0]);
			p.add("maxGravity", getTargetConfigById(id)->accelGravity[1]);
			p.add("trialCount", (float)sessions[sessionIndex].trials[j].count);
			if (getTargetConfigById(id)->jumpEnabled) {
				p.add("jumpEnabled", "true");
			}
			else {
				p.add("jumpEnabled", "false");
			}
			params.append(p);
		}
		return params;
	}

	Array<Param> getReactionExpConditions(String id) {
		int idx = getSessionIndex(id);
		return getReactionExpConditions(idx);
	}

	Array<Param> getReactionExpConditions(int sessionIndex) {
		Array<Param> params;
		for (int j = 0; j < sessions[sessionIndex].trials.size(); j++) {
			for (float intensity : getReactionConfigById(sessions[sessionIndex].trials[j].id)->intensities) {
				Param p;
				p.add("intensity", intensity);
				p.add("targetFrameRate", sessions[sessionIndex].frameRate);
				p.add("targetFrameLag", (float)sessions[sessionIndex].frameDelay);
				p.add("trialCount", (float)sessions[sessionIndex].trials[j].count);
				params.append(p);
			}
		}
		return params;
	}

	// Get the experiment config from file
	static ExperimentConfig load(String filename) {
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEexperimentconfig.Any"), "experimentconfig.Any");
		}
		return Any::fromFile(System::findDataFile(filename));
	}
};