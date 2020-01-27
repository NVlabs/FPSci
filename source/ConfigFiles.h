#pragma once

#include <G3D/G3D.h>
#include "TargetEntity.h"

/** Configure how the application should start */
class StartupConfig {
public:
    bool	developerMode = false;				///< Sets whether the app is run in "developer mode" (i.e. w/ extra menus)
	bool	waypointEditorMode = false;			///< Sets whether the app is run w/ the waypoint editor available
	bool	fullscreen = true;					///< Whether the app runs in windowed mode
	Vector2 windowSize = { 1920, 980 };			///< Window size (when not run in fullscreen)
    String	experimentConfigPath = "";			///< Optional path to an experiment config file (if "experimentconfig.Any" will not be this file)
    String	userConfigPath = "";				///< Optional path to a user config file (if "userconfig.Any" will not be this file)
    bool	audioEnable = true;					///< Audio on/off

    StartupConfig() {};

	/** Construct from any here */
    StartupConfig(const Any& any) {
        int settingsVersion = 1;
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("developerMode", developerMode);
			reader.getIfPresent("waypointEditorMode", waypointEditorMode);
			reader.getIfPresent("fullscreen", fullscreen);
			reader.getIfPresent("windowSize", windowSize);
            reader.getIfPresent("experimentConfigPath", experimentConfigPath);
            reader.getIfPresent("userConfigPath", userConfigPath);
            reader.getIfPresent("audioEnable", audioEnable);
            break;
        default:
            debugPrintf("Settings version '%d' not recognized in StartupConfig.\n", settingsVersion);
            break;
        }
    }

	/** Allow this to be converted back to any */
    Any toAny(const bool forceAll = true) const {
        Any a(Any::TABLE);
        a["developerMode"] = developerMode;
		a["waypointEditorMode"] = waypointEditorMode;
		a["fullscreen"] = fullscreen;
        a["experimentConfigPath"] = experimentConfigPath;
        a["userConfigPath"] = userConfigPath;
        a["audioEnable"] = audioEnable;
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

/** Key mapping */
class KeyMapping {
public:
	Table<String, Array<GKey>> map;
	Table<GKey, UserInput::UIFunction> uiMap;

	// Default key mapping
	KeyMapping() {
		map.set("moveForward", Array<GKey>{ (GKey)'w', GKey::UP });
		map.set("strafeLeft", Array<GKey>{ (GKey)'a', GKey::LEFT });
		map.set("moveBackward", Array<GKey>{ (GKey)'s', GKey::DOWN });
		map.set("strafeRight", Array<GKey>{ (GKey)'d', GKey::RIGHT });
		map.set("openMenu", Array<GKey>{ GKey::ESCAPE, GKey::TAB });
		map.set("quit", Array<GKey>{ GKey::MINUS });
		map.set("crouch", Array<GKey>{ GKey::LCTRL });
		map.set("jump", Array<GKey>{ GKey::SPACE });
		map.set("shoot", Array<GKey>{ GKey::LEFT_MOUSE });
		map.set("dummyShoot", Array<GKey>{ GKey::LSHIFT });
		map.set("dropWaypoint", Array<GKey>{ (GKey)'q' });
		map.set("toggleRecording", Array<GKey>{ (GKey)'r' });
		map.set("toggleRenderWindow", Array<GKey>{ (GKey)'1' });
		map.set("togglePlayerWindow", Array<GKey>{ (GKey)'2' });
		map.set("toggleWeaponWindow", Array<GKey>{ (GKey)'3' });
		map.set("toggleWaypointWindow", Array<GKey>{ (GKey)'4' });
		map.set("moveWaypointUp", Array<GKey>{ GKey::PAGEUP });
		map.set("moveWaypointDown", Array<GKey>{ GKey::PAGEDOWN });
		map.set("moveWaypointIn", Array<GKey>{ GKey::HOME });
		map.set("moveWaypointOut", Array<GKey>{ GKey::END });
		map.set("moveWaypointRight", Array<GKey>{ GKey::INSERT });
		map.set("moveWaypointLeft", Array<GKey>{ GKey::DELETE });
		getUiKeyMapping();
	};

	KeyMapping(const Any& any) : KeyMapping() {
		AnyTableReader reader = AnyTableReader(any);
		for (String actionName : map.getKeys()) {
			reader.getIfPresent(actionName, map[actionName]);
		}
		getUiKeyMapping();
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		for (String actionName : map.getKeys()) {
			a[actionName] = map[actionName];
		}
		return a;
	}

	static KeyMapping load(String filename = "keymap.Any") {
		if (!FileSystem::exists(System::findDataFile(filename, false))) {
			KeyMapping mapping = KeyMapping();
			mapping.toAny().save("keymap.Any");
			return mapping;
		}
		return Any::fromFile(System::findDataFile(filename));
	}

	void getUiKeyMapping() {
		// Bind keys to UIFunctions here
		uiMap.clear();
		for (GKey key : map["moveForward"])	uiMap.set(key, UserInput::UIFunction::UP);
		for (GKey key : map["strafeLeft"])	uiMap.set(key, UserInput::UIFunction::LEFT);
		for (GKey key : map["strafeRight"])	uiMap.set(key, UserInput::UIFunction::RIGHT);
		for (GKey key : map["moveBackward"])	uiMap.set(key, UserInput::UIFunction::DOWN);
	}
};

/** System configuration control and logging */
class SystemConfig {
public:
	// Output/runtime read parameters
	String	cpuName;			///< The vendor name of the CPU being used
	int		coreCount;			///< Core count for the CPU being used
	String	gpuName;			///< The vendor name of the GPU being used
	long	memCapacityMB;		///< The capacity of memory (RAM) in MB
	String	displayName;		///< The vendor name of the display (not currently working)
	int		displayXRes;		///< The horizontal size of the display in pixels
	int		displayYRes;		///< The vertical size of the display in pixels
	int		displayXSize;		///< The horizontal size of the display in mm
	int		displayYSize;		///< The vertical size of the display in mm

	// Input parameters
	bool	hasLogger = false;			///< Indicates that a hardware logger is present in the system
	String	loggerComPort = "";		///< Indicates the COM port that the logger is on when hasLogger = True
	bool	hasSync = false;			///< Indicates that a hardware sync will occur via serial card DTR signal
	String	syncComPort = "";		///< Indicates the COM port that the sync is on when hasSync = True

	SystemConfig() {};

	/** Construct from Any */
	SystemConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("HasLogger", hasLogger, "System config must specify the \"HasLogger\" flag!");
			reader.get("HasSync", hasSync, "System config must specify the \"HasSync\" flag!");
			reader.getIfPresent("LoggerComPort", loggerComPort);
			reader.getIfPresent("SyncComPort", syncComPort);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SystemConfig.\n", settingsVersion);
			break;
		}
		// Get the system info
		getSystemInfo();		
	}

	/** Serialize to Any */
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

	/** Load a system config from file */
	static SystemConfig load() {
		// if file not found, create a default system config
		if (!FileSystem::exists("systemconfig.Any")) { 
			SystemConfig config = SystemConfig();		// Create the default
			config.getSystemInfo();						// Get system info
			config.toAny().save("systemconfig.Any");	// Save a file
			return config;
		}
		return Any::fromFile(System::findDataFile("systemconfig.Any"));
	}

	/** Get the system info using (windows) calls */
	void getSystemInfo(void) {
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
		// This seems to break on many systems/provide less than descriptive names!!!
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

	/** Print the system info to log.txt */
	void printToLog() {
		// Print system info to log
		logPrintf("System Info: \n\tProcessor: %s\n\tCore Count: %d\n\tMemory: %dMB\n\tGPU: %s\n\tDisplay: %s\n\tDisplay Resolution: %d x %d (px)\n\tDisplay Size: %d x %d (mm)\n",
			cpuName, coreCount, memCapacityMB, gpuName, displayName, displayXRes, displayYRes, displayXSize, displayYSize);
		logPrintf("Logger Present: %s\nLogger COM Port: %s\nSync Card Present: %s\nSync COM Port: %s\n",
			hasLogger ? "True" : "False", loggerComPort, hasSync ? "True" : "False", syncComPort);
	}
};

/**Class for managing user configuration*/
class UserConfig {
public:
    String			id					= "anon";						///< Subject ID (as recorded in output DB)
    double			mouseDPI			= 800.0;						///< Mouse DPI setting
    double			cmp360				= 12.75;						///< Mouse sensitivity, reported as centimeters per 360�
	Vector2			turnScale			= Vector2(1.0f, 1.0f);			///< Turn scale for player, can be used to invert controls in either direction

	int				currentSession		= 0;							///< Currently selected session
		
	int				reticleIndex		= -1;							///< Reticle to show for this user
	Array<float>	reticleScale		= { 1.0f, 1.0f };				///< Scale for the user's reticle
	Array<Color4>	reticleColor		= {Color4(1.0, 0.0, 0.0, 1.0),	///< Color for the user's reticle
										Color4(1.0, 0.0, 0.0, 1.0)};	
	float			reticleShrinkTimeS	= 0.3f;							///< Time for reticle to contract after expand on shot (in seconds)


	UserConfig() {};

	/** Load from Any */
    UserConfig(const Any& any) {
        int settingsVersion = 1; // used to allow different version numbers to be loaded differently
        AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);
        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("id", id);
            reader.getIfPresent("mouseDPI", mouseDPI);
            reader.getIfPresent("cmp360", cmp360);
			reader.getIfPresent("reticleIndex", reticleIndex);
			reader.getIfPresent("reticleScale", reticleScale);
			reader.getIfPresent("reticleColor", reticleColor);
			reader.getIfPresent("reticleShrinkTime", reticleShrinkTimeS);
			reader.getIfPresent("turnScale", turnScale);
			break;
        default:
            debugPrintf("Settings version '%d' not recognized in UserConfig.\n", settingsVersion);
            break;
        }
    }
	
	/** Serialize to Any */
	Any toAny(const bool forceAll=true) const {
		Any a(Any::TABLE);
		a["id"] = id;										// Include subject ID
		a["mouseDPI"] = mouseDPI;							// Include mouse DPI
		a["cmp360"] = cmp360;								// Include cm/360
		a["reticleIndex"] = reticleIndex;
		a["reticleScale"] = reticleScale;
		a["reticleColor"] = reticleColor;
		a["reticleShrinkTime"] = reticleShrinkTimeS;
		a["turnScale"] = turnScale;
		return a;
	}
};

/** Class for loading a user table and getting user info */
class UserTable {
public:
	String					currentUser = "None";			///< The currently active user
	Array<UserConfig>		users = {};						///< A list of valid users

	UserTable() {};

	/** Load from Any */
	UserTable(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("currentUser", currentUser);
			reader.get("users", users, "Issue in the (required) \"users\" array in the user config file!");
			if (users.size() == 0) {
				throw "At least 1 user must be specified in the \"users\" array within the user configuration file!";
			}
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserTable.\n", settingsVersion);
			break;
		}
	}

	/** Get the current user's config */
    UserConfig* getCurrentUser() {
        for (int i = 0; i < users.length(); ++i) {
            if (!users[i].id.compare(currentUser)) return &(users[i]);
        }
        // return the first user by default and set the value
        currentUser = users[0].id;
        return &(users[0]);
    }

	/** Get the index of the current user from the user table */
    int getCurrentUserIndex() {
        for (int i = 0; i < users.length(); ++i) {
            if (!users[i].id.compare(currentUser)) return i;
        }
        // return the first user by default
        return 0;
    }

	/** Serialize to Any */
	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = 1;						///< Create a version 1 file
		a["currentUser"] = currentUser;					///< Include current subject ID
		a["users"] = users;								///< Include updated subject table
		return a;
	}

	/** Get a user config based on a user ID */
	shared_ptr<UserConfig> getUserById(String id) {
		for (UserConfig user : users) {
			if (!user.id.compare(id)) return std::make_shared<UserConfig>(user);
		}
		return nullptr;
	}

	/** Get an array of user IDs */
	Array<String> getIds() {
		Array<String> ids;
		for (UserConfig user : users) ids.append(user.id);
		return ids;
	}

	/** Simple rotine to get the UserTable Any structure from file */
	static UserTable load(String filename) {
		// Create default UserConfig file
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, generate a default user config table
			UserTable defTable = UserTable();
			defTable.users.append(UserConfig());			// Append one default user
			defTable.currentUser = defTable.users[0].id;	// Set this as the current user
			defTable.toAny().save("userconfig.Any");		// Save the .any file
			return defTable;
		}
		return Any::fromFile(System::findDataFile(filename));
	}

	/** Print the user table to the log */
	void printToLog() {
		logPrintf("Current User: %s\n", currentUser);
		for (UserConfig user : users) {
			logPrintf("\tUser ID: %s, cmp360 = %f, mouseDPI = %d\n", user.id, user.cmp360, user.mouseDPI);
		}
	}
};

/** Class for handling user status */
class UserSessionStatus {
public:
	String					id = "anon";					///< User ID
	Array<String>			sessionOrder;					///< Array containing session ordering
	Array<String>			completedSessions;				///< Array containing all completed session ids for this user
	static Array<String>	defaultSessionOrder;			///< Default session order
	static bool				randomizeDefaults;				///< Randomize default session order when applying to individual?

	UserSessionStatus() {}

	/** Load user status from Any */
	UserSessionStatus(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			// Require a user ID
			reader.get("id", id, "All user status fields must include the user ID!");
			// Setup default session order, then overwrite if specified	
			sessionOrder = defaultSessionOrder;
			if (randomizeDefaults) sessionOrder.randomize();
			reader.getIfPresent("sessions", sessionOrder);			// Override the default session order if one is provided for this user
			if (sessionOrder.length() == 0) {						// Check for sessions in list
				throw format("Must provide \"sessions\" array (or default) for User ID:\"%s\" in user status!", id);
			}
			// Get the completed sessions array
			reader.getIfPresent("completedSessions", completedSessions);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserSessionStatus.\n", settingsVersion);
			break;
		}
	}

	/** Serialize to Any */
	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["id"] = id;									// populate id
		a["sessions"] = sessionOrder;					// populate session order
		a["completedSessions"] = completedSessions; 	// Include updated subject table
		return a;
	}
};

/** Class for representing user status tables */
class UserStatusTable {
public:
	bool allowRepeat = false;							///< Flag for whether to (strictly) sequence these experiments (allow duplicates)
	bool randomizeDefaults = false;						///< Randomize from default session order when applying to user
	Array<String> defaultSessionOrder = {};				///< Default session ordering (for all unspecified users)
	Array<UserSessionStatus> userInfo = {};				///< Array of user status

	UserStatusTable() {}

	/** Load from Any */
	UserStatusTable(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("allowRepeat", allowRepeat);
			reader.getIfPresent("sessions", defaultSessionOrder);
			UserSessionStatus::defaultSessionOrder = defaultSessionOrder;				// Set the default order here
			reader.getIfPresent("randomizeSessionOrder", randomizeDefaults);			
			UserSessionStatus::randomizeDefaults = randomizeDefaults;					// Set whether default session order is randomized
			reader.get("users", userInfo, "Issue in the (required) \"users\" array from the user status file!");
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in UserStatus.\n", settingsVersion);
			break;
		}
	}

	/** Serialzie to Any */
	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = 1;						// Create a version 1 file
		a["allowRepeat"] = allowRepeat;
		a["randomizeSessionOrder"] = randomizeDefaults;
		a["sessions"] = defaultSessionOrder;
		a["users"] = userInfo;							// Include updated subject table
		return a;
	}

	/** Get the user status table from file */
	static UserStatusTable load(void) {
		if (!FileSystem::exists("userstatus.Any")) { // if file not found, create a default userstatus.Any
			UserStatusTable defStatus = UserStatusTable();			// Create empty status
			defStatus.userInfo.append(UserSessionStatus());			// Add single "default" user
			defStatus.toAny().save("userstatus.Any");				// Save .any file
			return defStatus;
		}
		return Any::fromFile(System::findDataFile("userstatus.Any"));
	}

	/** Get a given user's status from the table by ID */
	shared_ptr<UserSessionStatus> getUserStatus(String id) {
		for (UserSessionStatus user : userInfo) {
			if (!user.id.compare(id)) return std::make_shared<UserSessionStatus>(user);
		}
		return nullptr;
	}

	/** Get the next session ID for a given user (by ID) */
	String getNextSession(String userId) {
		// Return the first valid session that has not been completed
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

	/** Add a completed session to a given user's completedSessions array */
	void addCompletedSession(String userId, String sessId) {
		for (int i = 0; i < userInfo.length(); i++) {
			if (!userInfo[i].id.compare(userId)) {
				userInfo[i].completedSessions.append(sessId);
			}
		}
	}

	void validate(Array<String> sessions) {
		// Build a string list of valid options for session IDs from the experiment
		String expSessions = "[";
		for (String sess : sessions) expSessions += "\"" + sess + "\", ";
		expSessions = expSessions.substr(0, expSessions.size() - 2);
		expSessions += "]";

		// Check default sessions for valid ids
		for(String defSessId : defaultSessionOrder) {
			if (!sessions.contains(defSessId)) {
				throw format("Default session config in user status has session with ID: \"%s\". This session ID does not appear in experimentconfig.Any's \"sessions\" array. Valid options are: %s", defSessId, expSessions);
			}
		}

		// Check each user for valid options
		for (UserSessionStatus userStatus : userInfo) {
			for (String userSessId : userStatus.sessionOrder) {
				if (!sessions.contains(userSessId)) {
					throw format("User \"%s\" has session with ID: \"%s\" in their User Status \"sessions\" Array. This session ID does not appear in the experimentconfig.Any \"sessions\" array. Valid options are: %s", userStatus.id, userSessId, expSessions);
				}
			}
		}
	}

	/** Print the user status table to the log */
	void printToLog() {
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

			logPrintf("Subject ID: %s\nSession Order: [%s]\nCompleted Sessions: [%s]\n", status.id, sessOrder, completedSess);
		}
	}
};

/** Weapon configuration class */
class WeaponConfig {
public:
	String	id = "default";												///< Id by which to refer to this weapon
	int		maxAmmo = 10000;											///< Max ammo (clicks) allowed per trial (set large for laser mode)
	float	firePeriod = 0.5;											///< Minimum fire period (set to 0 for laser mode)
	bool	autoFire = false;											///< Fire repeatedly when mouse is held? (set true for laser mode)
	float	damagePerSecond = 2.0f;										///< Damage per second delivered (compute shot damage as damagePerSecond/firePeriod)
	String	fireSound = "sound/42108__marcuslee__Laser_Wrath_6.wav"; 	///< Sound to play on fire
	float	fireSoundVol = 0.5f;										///< Volume for fire sound
	bool	renderModel = false;										///< Render a model for the weapon?
	Vector3	muzzleOffset = Vector3(0, 0, 0);							///< Offset to the muzzle of the weapon model
	bool	renderMuzzleFlash = false;									///< Render a muzzle flash when the weapon fires?
	bool	renderDecals = true;										///< Render decals when the shots miss?
	bool	renderBullets = false;										///< Render bullets leaving the weapon
	float	bulletSpeed = 1.0f;											///< Speed to draw at for rendered rounds (in m/s)
	//String missDecal = "bullet-decal-256x256.png";					///< The decal to place where the shot misses
	float	fireSpread = 0;												///< The spread of the fire
	float	damageRollOffAim = 0;										///< Damage roll off w/ aim
	float	damageRollOffDistance = 0;									///< Damage roll of w/ distance
	//String reticleImage;												///< Reticle image to show for this weapon
	ArticulatedModel::Specification modelSpec;							///< Model to use for the weapon (must be specified when renderModel=true)

	WeaponConfig() {
	// Suggested any for "default" model, leaving this here for now... breaks debug config if uncommented
			//PARSE_ANY(ArticulatedModel::Specification{
			//	filename = "model/sniper/sniper.obj";
			//	preprocess = {
			//		transformGeometry(all(), Matrix4::yawDegrees(90));
			//		transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
			//	};
			//	scale = 0.25;
			//	};)
	}

	/** Load from Any */
	WeaponConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("id", id);

			reader.getIfPresent("maxAmmo", maxAmmo);
			reader.getIfPresent("firePeriod", firePeriod);
			reader.getIfPresent("autoFire", autoFire);
			reader.getIfPresent("damagePerSecond", damagePerSecond);
			reader.getIfPresent("fireSound", fireSound);
			reader.getIfPresent("renderModel", renderModel);
			if (renderModel) {
				reader.get("modelSpec", modelSpec, "If \"renderModel\" is set to true within a weapon config then a \"modelSpec\" must be provided!");
			}
			else {
				reader.getIfPresent("modelSpec", modelSpec);
			}
			reader.getIfPresent("muzzleOffset", muzzleOffset);
			reader.getIfPresent("renderMuzzleFlash", renderMuzzleFlash);
			reader.getIfPresent("renderDecals", renderDecals);
			reader.getIfPresent("renderBullets", renderBullets);
			reader.getIfPresent("bulletSpeed", bulletSpeed);
			//reader.getIfPresent("missDecal", missDecal);
			reader.getIfPresent("fireSpread", fireSpread);
			reader.getIfPresent("damageRollOffAim", damageRollOffAim);
			reader.getIfPresent("damageRollOffDistance", damageRollOffDistance);
			//reader.getIfPresent("recticleImage", reticleImage);
		default:
			debugPrintf("Settings version '%d' not recognized in TargetConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["id"] = id;
		a["maxAmmo"] = maxAmmo;
		a["firePeriod"] = firePeriod;
		a["autoFire"] = autoFire;
		a["damagePerSecond"] = damagePerSecond;
		a["fireSound"] = fireSound;
		a["renderModel"] = renderModel;
		a["modelSpec"] = modelSpec;
		a["muzzleOffset"] = muzzleOffset;
		a["renderMuzzleFlash"] = renderMuzzleFlash;
		a["renderDecals"] = renderDecals;
		a["renderBullets"] = renderBullets;
		a["bulletSpeed"] = bulletSpeed;
		a["fireSpread"] = fireSpread;
		a["damageRollOffAim"] = damageRollOffAim;
		a["damageRollOffDistance"] = damageRollOffDistance;
		return a;
	}
};

/** Class for representing a given target configuration */
class TargetConfig : public ReferenceCountedObject {
public:
	String			id;										///< Trial ID to indentify affiliated trial runs
	//bool    		elevLocked = false;						///< Elevation locking
	bool			upperHemisphereOnly = false;            ///< Limit flying motion to upper hemisphere only
	bool			logTargetTrajectory = true;				///< Log this target's trajectory
	Array<float>	distance = { 30.0f, 40.0f };			///< Distance to the target
	Array<float>	motionChangePeriod = { 1.0f, 1.0f };	///< Range of motion change period in seconds
	Array<float>	speed = { 0.0f, 5.5f };					///< Range of angular velocities for target
	Array<float>	eccH = { 5.0f, 15.0f };					///< Range of initial horizontal eccentricity
	Array<float>	eccV = { 0.0f, 2.0f };					///< Range of initial vertical eccentricity
	Array<float>	size = { 0.2f, 0.2f };					///< Visual size of the target (in degrees)
	bool			jumpEnabled = false;					///< Flag indicating whether the target jumps
	Array<float>	jumpPeriod = { 2.0f, 2.0f };			///< Range of time period between jumps in seconds
	Array<float>	jumpSpeed = { 2.0f, 5.5f };				///< Range of jump speeds in meters/s
	Array<float>	accelGravity = { 9.8f, 9.8f };			///< Range of acceleration due to gravity in meters/s^2
	Array<Destination> destinations;						///< Array of destinations to traverse
	String			destSpace = "world";					///< Space to use for destinations (implies offset) can be "world" or "player"
	int				respawnCount = 0;						///< Number of times to respawn
	AABox			spawnBounds;							///< Spawn position bounding box
	AABox			moveBounds;								///< Movemvent bounding box
	Array<bool>		axisLock = { false, false, false };		///< Array of axis lock values

	String			hitSound = "sound/18397__inferno__smalllas.wav";	///< Sound to play when target is hit (but not destoyed)
	float			hitSoundVol = 1.0f;						///< Hit sound volume
	String          destroyedSound = "sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav";		///< Sound to play when target destroyed
	float           destroyedSoundVol = 10.0f;

	Any modelSpec = PARSE_ANY(ArticulatedModel::Specification{			///< Basic model spec for target
		filename = "model/target/target.obj";
		cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings{
					allowVertexMerging = true;
					forceComputeNormals = false;
					forceComputeTangents = false;
					forceVertexMerging = true;
					maxEdgeLength = inf;
					maxNormalWeldAngleDegrees = 0;
					maxSmoothAngleDegrees = 0;
		    };
		});

	TargetConfig() {}

	/** Load from Any */
	TargetConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id, "An \"id\" field must be provided for every target config!");
			//reader.getIfPresent("elevationLocked", elevLocked);
			reader.getIfPresent("upperHemisphereOnly", upperHemisphereOnly);
			reader.getIfPresent("logTargetTrajectory", logTargetTrajectory);
			reader.getIfPresent("distance", distance);
			reader.getIfPresent("motionChangePeriod", motionChangePeriod);
			reader.getIfPresent("speed", speed);
			reader.getIfPresent("visualSize", size);
			reader.getIfPresent("eccH", eccH);
			reader.getIfPresent("eccV", eccV);
			reader.getIfPresent("jumpEnabled", jumpEnabled);
			reader.getIfPresent("jumpSpeed", jumpSpeed);
			reader.getIfPresent("jumpPeriod", jumpPeriod);
			reader.getIfPresent("accelGravity", accelGravity);
			reader.getIfPresent("modelSpec", modelSpec);
			reader.getIfPresent("destSpace", destSpace);
			reader.getIfPresent("destinations", destinations);
			reader.getIfPresent("respawnCount", respawnCount);
			if (destSpace == "world" && destinations.size() == 0) {
				reader.get("moveBounds", moveBounds, format("A world-space target must either specify destinations or a movement bounding box. See target: \"%s\"", id));
				spawnBounds = moveBounds;
			}
			else {
				if (reader.getIfPresent("moveBounds", moveBounds)) {
					spawnBounds = moveBounds;
				}
			}
			reader.getIfPresent("spawnBounds", spawnBounds);
			if (destSpace == "world" && destinations.size() == 0 && !moveBounds.contains(spawnBounds)) {
				String moveBoundStr = format("AABox{%s, %s}", moveBounds.high().toString(), moveBounds.low().toString());
				String spawnBoundStr = format("AABox{%s, %s}", spawnBounds.high().toString(), spawnBounds.low().toString());
				throw format("The \"moveBounds\" AABox (=%s) must contain the \"spawnBounds\" AABox (=%s)!", moveBoundStr, spawnBoundStr);
			}
			if (reader.getIfPresent("axisLocked", axisLock)) {
				if (axisLock.size() < 3) {
					throw format("Must provide 3 fields (X,Y,Z) for axis lock! Only %d provided! See target: \"%s\"", axisLock.size(), id);
				}
				else if (axisLock.size() > 3) {
					logPrintf("Provided axis lock for target \"%s\" has >3 fields, using the first 3...", id);
				}
				if (axisLock[0] && axisLock[1] && axisLock[2] && speed[0] != 0.0f && speed[1] != 0.0f) {
					throw format("Target \"%s\" locks all axes but has non-zero speed!", id);
				}
			}
			reader.getIfPresent("hitSound", hitSound);
			reader.getIfPresent("hitSoundVol", hitSoundVol);
			reader.getIfPresent("destroyedSound", destroyedSound);
			reader.getIfPresent("destroyedSoundVol", destroyedSoundVol);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in TargetConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["id"] = id;
		a["respawnCount"] = respawnCount;
		a["visualSize"] = size;
		a["modelSpec"] = modelSpec;
		a["logTargetTrajectory"] = logTargetTrajectory;
		if (destinations.size() > 0) {						// Destination-based target
			a["destSpace"] = destSpace;
			a["destinations"] = destinations;
		}
		else {												// Parametric target
			a["upperHemisphereOnly"] = upperHemisphereOnly;
			a["distance"] = distance;
			a["motionChangePeriod"] = motionChangePeriod;
			a["visualSize"] = size;
			a["eccH"] = eccH;
			a["eccV"] = eccV;
			a["jumpEnabled"] = jumpEnabled;
			a["jumpPeriod"] = jumpPeriod;
			a["accelGravity"] = accelGravity;
			a["axisLocked"] = axisLock;
		}
		a["hitSound"] = hitSound;
		a["hitSoundVol"] = hitSoundVol;
		a["explosionSound"] = destroyedSound;
		a["explosionSoundVol"] = destroyedSoundVol;
		return a;
	};

	static TargetConfig load(String filename) {
		return TargetConfig(Any::fromFile(System::findDataFile(filename)));
	}
};

/** Trial count class (optional for alternate TargetConfig/count table lookup) */
class TrialCount {
public:
	Array<String>	ids;			///< Trial ID list
	int				count = 1;		///< Count of trials to be performed
	static int		defaultCount;	///< Default count to use

	TrialCount() {};

	/** Load from Any */
	TrialCount(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("ids", ids, "An \"ids\" field must be provided for each set of trials!");
			if (!reader.getIfPresent("count", count)) {
				count = defaultCount;
			}
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["ids"] = ids;
		a["count"] = count;
		return a;
	}
};

class Question {
public:
	enum Type {
		None,
		MultipleChoice,
		Entry,
		Rating
	};
	Type type = Type::None;
	String prompt = "";
	Array<String> options;
	String title = "Feedback";
	String result = "";

	Question() {};

	Question(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		String typeStr;

		switch (settingsVersion) {
		case 1:
			// Get the question type
			reader.get("type", typeStr, "A \"type\" field must be provided with every question!");
			// Pase the type and get options for multiple choice
			if (!typeStr.compare("MultipleChoice")) {
				type = Type::MultipleChoice;
				reader.get("options", options, "An \"options\" Array must be specified with \"MultipleChoice\" style questions!");
			}
			else if (!typeStr.compare("Entry")) {
				type = Type::Entry;
			}
			else if (!typeStr.compare("Rating")) {
				type = Type::Rating;
				reader.get("options", options, "An \"options\" Array must be specified with \"Rating\" style questions!");
			}
			else {
				throw format("Unrecognized question \"type\" String \"%s\". Valid options are \"MultipleChoice\" or \"Entry\"", typeStr);
			}

			// Get the question prompt (required) and title (optional)
			reader.get("prompt", prompt, "A \"prompt\" field must be provided with every question!");
			reader.getIfPresent("title", title);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in Question.\n", settingsVersion);
			break;
		}
	}

};

class RenderConfig {
public:
	// Rendering parameters
	float           frameRate = 1000.0f;						///< Target (goal) frame rate (in Hz)
	int             frameDelay = 0;								///< Integer frame delay (in frames)
	String          shader = "";								///< Option for a custom shader name
	float           hFoV = 103.0f;							    ///< Field of view (horizontal) for the user

	void load(AnyTableReader reader, int settingsVersion=1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("frameRate", frameRate);
			reader.getIfPresent("frameDelay", frameDelay);
			reader.getIfPresent("shader", shader);
			reader.getIfPresent("horizontalFieldOfView", hFoV);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["frameRate"] = frameRate;
		a["frameDelay"] = frameDelay;
		a["horizontalFieldOfView"] = hFoV;
		a["shader"] = shader;
		return a;
	}

};

class PlayerConfig {
public:
	// View parameters
	float           moveRate = 0.0f;							///< Player move rate (defaults to no motion)
	float           height = 1.5f;								///< Height for the player view (in walk mode)
	float           crouchHeight = 0.8f;						///< Height for the player view (during crouch in walk mode)
	float           jumpVelocity = 7.0f;						///< Jump velocity for the player
	float           jumpInterval = 0.5f;						///< Minimum time between jumps in seconds
	bool            jumpTouch = true;							///< Require the player to be touch a surface to jump?
	Vector3         gravity = Vector3(0.0f, -10.0f, 0.0f);		///< Gravity vector
	Vector2			moveScale = Vector2(1.0f, 1.0f);			///< Player (X/Y) motion scaler
	Vector2			turnScale = Vector2(1.0f, 1.0f);			///< Player (horizontal/vertical) turn rate scaler
	Array<bool>		axisLock = { false, false, false };			///< World-space player motion axis lock
	bool			stillBetweenTrials = false;					///< Disable player motion between trials?
	bool			resetPositionPerTrial = false;				///< Reset the player's position on a per trial basis (to scene default)

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("moveRate", moveRate);
			reader.getIfPresent("moveScale", moveScale);
			reader.getIfPresent("turnScale", turnScale);
			reader.getIfPresent("playerHeight", height);
			reader.getIfPresent("crouchHeight", crouchHeight);
			reader.getIfPresent("jumpVelocity", jumpVelocity);
			reader.getIfPresent("jumpInterval", jumpInterval);
			reader.getIfPresent("jumpTouch", jumpTouch);
			reader.getIfPresent("playerGravity", gravity);
			reader.getIfPresent("playerAxisLock", axisLock);
			reader.getIfPresent("disablePlayerMotionBetweenTrials", stillBetweenTrials);
			reader.getIfPresent("resetPlayerPositionBetweenTrials", resetPositionPerTrial);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["moveRate"] = moveRate;
		a["moveScale"] = moveScale;
		a["playerHeight"] = height;
		a["crouchHeight"] = crouchHeight;
		a["jumpVelocity"] = jumpVelocity;
		a["jumpInterval"] = jumpInterval;
		a["jumpTouch"] = jumpTouch;
		a["playerGravity"] = gravity;
		a["playerAxisLock"] = axisLock;
		a["disablePlayerMotionBetweenTrials"] = stillBetweenTrials;
		a["resetPlayerPositionBetweenTrials"] = resetPositionPerTrial;
		return a;
	}

};

class HudConfig {
public:
	// HUD parameters
	bool            enable = false;							            ///< Master control for all HUD elements
	bool            showBanner = false;						                ///< Show the banner display
	float           bannerVertVisible = 0.41f;				                ///< Vertical banner visibility
	float           bannerLargeFontSize = 30.0f;				            ///< Banner percent complete font size
	float           bannerSmallFontSize = 14.0f;				            ///< Banner detail font size
	String          hudFont = "dominant.fnt";				                ///< Font to use for Heads Up Display

	// Player health bar
	bool            showPlayerHealthBar = false;							///< Display a player health bar?
	Point2          playerHealthBarSize = Point2(200.0f, 20.0f);			///< Player health bar size (in pixels)
	Point2          playerHealthBarPos = Point2(10.0f, 10.0f);				///< Player health bar position (in pixels)
	Point2          playerHealthBarBorderSize = Point2(2.0f, 2.0f);			///< Player health bar border size
	Color4          playerHealthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);		///< Player health bar border color
	Array<Color4>   playerHealthBarColors = {								///< Player health bar start/end colors
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Weapon status
	bool            showAmmo = false;											///< Display remaining ammo
	Point2          ammoPosition = Point2(10.0f, 10.0f);						///< Position of the ammo indicator text
	float           ammoSize = 24.0f;											///< Font size for ammo indicator text
	Color4          ammoColor = Color4(1.0, 1.0, 1.0, 1.0);						///< Color for ammo indicator text
	Color4          ammoOutlineColor = Color4(0.0, 0.0, 0.0, 1.0);				///< Outline color for ammo indicator text
	bool            renderWeaponStatus = true;									///< Display weapon cooldown
	String          cooldownMode = "ring";										///< Currently "ring" and "box" are supported
	String          weaponStatusSide = "left";									///< Only applied in "box" mode, can be "right" or "left"
	float           cooldownInnerRadius = 40.0f;								///< Inner radius for cooldown ring
	float           cooldownThickness = 10.0f;									///< Thickness of cooldown ring
	int             cooldownSubdivisions = 64;									///< Number of polygon divisions in the "ring"
	Color4          cooldownColor = Color4(1.0f, 1.0f, 1.0f, 0.75f);			///< Cooldown ring color when active (transparent when inactive)

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("showHUD", enable);
			reader.getIfPresent("showBanner", showBanner);
			reader.getIfPresent("hudFont", hudFont);
			reader.getIfPresent("showPlayerHealthBar", showPlayerHealthBar);
			reader.getIfPresent("playerHealthBarSize", playerHealthBarSize);
			reader.getIfPresent("playerHealthBarPosition", playerHealthBarPos);
			reader.getIfPresent("playerHealthBarBorderSize", playerHealthBarBorderSize);
			reader.getIfPresent("playerHealthBarBorderColor", playerHealthBarBorderColor);
			reader.getIfPresent("playerHealthBarColors", playerHealthBarColors);
			reader.getIfPresent("showAmmo", showAmmo);
			reader.getIfPresent("ammoPosition", ammoPosition);
			reader.getIfPresent("ammoSize", ammoSize);
			reader.getIfPresent("ammoColor", ammoColor);
			reader.getIfPresent("ammoOutlineColor", ammoOutlineColor);
			reader.getIfPresent("renderWeaponStatus", renderWeaponStatus);
			reader.getIfPresent("weaponStatusSide", weaponStatusSide);
			reader.getIfPresent("cooldownMode", cooldownMode);
			reader.getIfPresent("cooldownInnerRadius", cooldownInnerRadius);
			reader.getIfPresent("cooldownThickness", cooldownThickness);
			reader.getIfPresent("cooldownSubdivisions", cooldownSubdivisions);
			reader.getIfPresent("cooldownColor", cooldownColor);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["showHUD"] = enable;
		a["showBanner"] = showBanner;
		a["hudFont"] = hudFont;
		a["showPlayerHealthBar"] = showPlayerHealthBar;
		a["playerHealthBarSize"] = playerHealthBarSize;
		a["playerHealthBarPosition"] = playerHealthBarPos;
		a["playerHealthBarBorderSize"] = playerHealthBarBorderSize;
		a["playerHealthBarBorderColor"] = playerHealthBarBorderColor;
		a["playerHealthBarColors"] = playerHealthBarColors;
		a["showAmmo"] = showAmmo;
		a["ammoPosition"] = ammoPosition;
		a["ammoSize"] = ammoSize;
		a["ammoColor"] = ammoColor;
		a["ammoOutlineColor"] = ammoOutlineColor;
		a["renderWeaponStatus"] = renderWeaponStatus;
		a["weaponStatusSide"] = weaponStatusSide;
		a["cooldownMode"] = cooldownMode;
		a["cooldownInnerRadius"] = cooldownInnerRadius;
		a["cooldownThickness"] = cooldownThickness;
		a["cooldownSubdivisions"] = cooldownSubdivisions;
		a["cooldownColor"] = cooldownColor;
		return a;
	}
};

class TargetViewConfig {
public:
	// Target color based on health
	Array<Color3>   healthColors = {									    ///< Target start/end color (based on target health)
		Color3(0.0, 1.0, 0.0),
		Color3(1.0, 0.0, 0.0)
	};

	// Target health bars
	bool            showHealthBars = false;									///< Display a target health bar?
	Point2          healthBarSize = Point2(100.0f, 10.0f);					///< Health bar size (in pixels)
	Point3          healthBarOffset = Point3(0.0f, -50.0f, 0.0f);			///< Offset from target to health bar (in pixels)
	Point2          healthBarBorderSize = Point2(2.0f, 2.0f);				///< Thickness of the target health bar border
	Color4          healthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);	///< Health bar border color
	Array<Color4>   healthBarColors = {										///< Target health bar start/end color
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Floating combat text controls
	bool            showCombatText = false;								///< Display floating combat text?
	String          combatTextFont = "dominant.fnt";					///< Font to use for combat text
	float           combatTextSize = 16.0;								///< Font size for floating combat text
	Color4          combatTextColor = Color4(1.0, 0.0, 0.0, 1.0);		///< The main color for floating combat text
	Color4          combatTextOutline = Color4(0.0, 0.0, 0.0, 1.0);		///< Combat text outline color
	Point3          combatTextOffset = Point3(0.0, -10.0, 0.0);			///< Initial offset for combat text
	Point3          combatTextVelocity = Point3(0.0, -100.0, 0.0);		///< Move rate/vector for combat text
	float           combatTextFade = 0.98f;								///< Fade rate for combat text (0 implies don't fade)	
	float           combatTextTimeout = 0.5f;							///< Time for combat text to disappear (in seconds)

	// Reference/dummy target
	float           refTargetSize = 0.01f;								///< Size of the reference target
	Color3          refTargetColor = Color3(1.0, 0.0, 0.0);				///< Default reference target color

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("showTargetHealthBars", showHealthBars);
			reader.getIfPresent("targetHealthBarSize", healthBarSize);
			reader.getIfPresent("targetHealthBarOffset", healthBarOffset);
			reader.getIfPresent("targetHealthBarBorderSize", healthBarBorderSize);
			reader.getIfPresent("targetHealthBarBorderColor", healthBarBorderColor);
			reader.getIfPresent("targetHealthColors", healthColors);
			reader.getIfPresent("targetHealthBarColors", healthBarColors);
			reader.getIfPresent("showFloatingCombatText", showCombatText);
			reader.getIfPresent("floatingCombatTextSize", combatTextSize);
			reader.getIfPresent("floatingCombatTextFont", combatTextFont);
			reader.getIfPresent("floatingCombatTextColor", combatTextColor);
			reader.getIfPresent("floatingCombatTextOutlineColor", combatTextOutline);
			reader.getIfPresent("floatingCombatTextOffset", combatTextOffset);
			reader.getIfPresent("floatingCombatTextVelocity", combatTextVelocity);
			reader.getIfPresent("floatingCombatTextFade", combatTextFade);
			reader.getIfPresent("floatingCombatTextTimeout", combatTextTimeout);
			reader.getIfPresent("referenceTargetSize", refTargetSize);
			reader.getIfPresent("referenceTargetColor", refTargetColor);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["showTargetHealthBars"] = showHealthBars;
		a["targetHealthBarSize"] = healthBarSize;
		a["targetHealthBarOffset"] = healthBarOffset;
		a["targetHealthBarBorderSize"] = healthBarBorderSize;
		a["targetHealthBarBorderColor"] = healthBarBorderColor;
		a["targetHealthColors"] = healthColors;
		a["targetHealthBarColors"] = healthBarColors;
		a["showFloatingCombatText"] = showCombatText;
		a["floatingCombatTextSize"] = combatTextSize;
		a["floatingCombatTextFont"] = combatTextFont;
		a["floatingCombatTextColor"] = combatTextColor;
		a["floatingCombatTextOutlineColor"] = combatTextOutline;
		a["floatingCombatTextOffset"] = combatTextOffset;
		a["floatingCombatTextVelocity"] = combatTextVelocity;
		a["floatingCombatTextFade"] = combatTextFade;
		a["floatingCombatTextTimeout"] = combatTextTimeout;
		a["referenceTargetSize"] = refTargetSize;
		a["referenceTargetColor"] = refTargetColor;
		return a;
	}
};

class ClickToPhotonConfig {
public:
	// Click-to-photon
	bool            enabled = false;                            ///< Render click to photon box
	String          side = "right";                             ///< "right" for right side, otherwise left
	String			mode = "total";								///< Mode used to signal either minimum system latency ("minimum"), or added latency ("total"), or "both"
	Point2          size = Point2(0.05f, 0.035f);				///< Size of the click-to-photon area (ratio of screen space)
	float           vertPos = 0.5f;				                ///< Percentage of the screen down to locate the box
	Array<Color3>   colors = {				                    ///< Colors to apply to click to photon box
		Color3::white() * 0.2f,
		Color3::white() * 0.8f
	};

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("renderClickPhoton", enabled);
			reader.getIfPresent("clickPhotonSide", side);
			reader.getIfPresent("clickPhotonMode", mode);		
			reader.getIfPresent("clickPhotonSize", size);
			reader.getIfPresent("clickPhotonVertPos", vertPos);
			reader.getIfPresent("clickPhotonColors", colors);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["renderClickPhoton"] = enabled;
		a["clickPhotonSide"] = side;
		a["clickPhotonSize"] = size;
		a["clickPhotonVertPos"] = vertPos;
		a["clickPhotonColors"] = colors;
		return a;
	}
};

class AudioConfig {
public:
	// Sounds
	String			sceneHitSound = "sound/18382__inferno__hvylas.wav";								///< Sound to play when hitting the scene
	float			sceneHitSoundVol = 1.0f;

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("sceneHitSound", sceneHitSound);
			reader.getIfPresent("sceneHitSoundVol", sceneHitSoundVol);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["sceneHitSound"] = sceneHitSound;
		a["sceneHitSoundVol"] = sceneHitSoundVol;
		return a;
	}
};

class TimingConfig {
public:
	// Timing parameters
	float           readyDuration = 0.5f;						///< Time in ready state in seconds
	float           taskDuration = 100000.0f;					///< Maximum time spent in any one task
	float           feedbackDuration = 1.0f;					///< Time in feedback state in seconds
	// Trial count
	int             defaultTrialCount = 5;						///< Default trial count

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("feedbackDuration", feedbackDuration);
			reader.getIfPresent("readyDuration", readyDuration);
			reader.getIfPresent("taskDuration", taskDuration);
			reader.getIfPresent("defaultTrialCount", defaultTrialCount);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["feedbackDuration"] = feedbackDuration;
		a["readyDuration"] = readyDuration;
		a["taskDuration"] = taskDuration;
		a["defaultTrialCount"] = defaultTrialCount;
		return a;
	}
};

class LoggerConfig {
public:
	// Enable flags for log
	bool enable					= true;		///< High-level logging enable flag (if false no output is created)							
	bool logTargetTrajectories	= true;		///< Log target trajectories in table?
	bool logFrameInfo			= true;		///< Log frame info in table?
	bool logPlayerActions		= true;		///< Log player actions in table?
	bool logTrialResponse		= true;		///< Log trial response in table?
	bool logUsers				= true;		///< Log user infomration in table?

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("logEnable", enable);
			reader.getIfPresent("logTargetTrajectories", logTargetTrajectories);
			reader.getIfPresent("logFrameInfo", logFrameInfo);
			reader.getIfPresent("logPlayerActions", logPlayerActions);
			reader.getIfPresent("logTrialResponse", logTrialResponse);
			reader.getIfPresent("logUsers", logUsers);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a) const {
		a["logEnable"] = enable;
		a["logTargetTrajectories"] = logTargetTrajectories;
		a["logFrameInfo"] = logFrameInfo;
		a["logPlayerActions"] = logPlayerActions;
		a["logTrialResponse"] = logTrialResponse;
		a["logUsers"] = logUsers;
		return a;
	}
};

class FpsConfig : public ReferenceCountedObject {
public:
	int	            settingsVersion = 1;						///< Settings version
	String          sceneName = "";							    ///< Scene name

	// Sub structures
	RenderConfig		render;									///< Render related config parameters
	PlayerConfig		player;									///< Player related config parameters
	HudConfig			hud;									///< HUD related config parameters
	AudioConfig			audio;									///< Audio related config parameters
	TimingConfig		timing;									///< Timing related config parameters
	TargetViewConfig	targetView;								///< Target drawing config parameters
	ClickToPhotonConfig clickToPhoton;							///< Click to photon config parameters
	LoggerConfig		logger;									///< Logging configuration
	WeaponConfig		weapon;			                        ///< Weapon to be used
	Array<Question> questionArray;								///< Array of questions for this experiment/trial

	// Constructors
	FpsConfig(const Any& any) {
		load(any);
	}

	FpsConfig(const Any& any, const FpsConfig& defaultConfig) {
		*this = defaultConfig;
		load(any);
	}

	FpsConfig() {}

	void load(const Any& any) {
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);
		render.load(reader, settingsVersion);
		player.load(reader, settingsVersion);
		hud.load(reader, settingsVersion);
		targetView.load(reader, settingsVersion);
		clickToPhoton.load(reader, settingsVersion);
		audio.load(reader, settingsVersion);
		timing.load(reader, settingsVersion);
		logger.load(reader, settingsVersion);
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("sceneName", sceneName);
			reader.getIfPresent("weapon", weapon);
			reader.getIfPresent("questions", questionArray);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in FpsConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = settingsVersion;
		a["sceneName"] = sceneName;
		a = render.addToAny(a);
		a = player.addToAny(a);
		a = hud.addToAny(a);
		a = targetView.addToAny(a);
		a = clickToPhoton.addToAny(a);
		a = audio.addToAny(a);
		a = timing.addToAny(a);
		a = logger.addToAny(a);
		a["weapon"] =  weapon;
		return a;
	}
};

/** Configuration for a session worth of trials */
class SessionConfig : public FpsConfig {
public:
	String				id;								///< Session ID
	String				description = "Session";		///< String indicating whether session is training or real
	Array<TrialCount>	trials;							///< Array of trials (and their counts) to be performed
	static FpsConfig	defaultConfig;

	SessionConfig() : FpsConfig(defaultConfig) {}

	static shared_ptr<SessionConfig> create() {
		return createShared<SessionConfig>();
	}

	/** Load from Any */
	SessionConfig(const Any& any) : FpsConfig(any, defaultConfig) {
		TrialCount::defaultCount = timing.defaultTrialCount;
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Unique session info
			reader.get("id", id, "An \"id\" field must be provided for each session!");
			reader.getIfPresent("description", description);
			reader.get("trials", trials, format("Issues in the (required) \"trials\" array for session: \"%s\"", id));
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		// Get the base any config
		Any a = FpsConfig::toAny();

		// Update w/ the session-specific fields
		a["id"] = id;
		a["description"] = description;
		a["trials"] = trials;
		return a;
	}

	/** Get the total number of trials in this session */
	int getTotalTrials(void) {
		int count = 0;
		for (const TrialCount& tc : trials) {
			count += tc.count;
		}
		return count;
	}
};

/** Experiment configuration */
class ExperimentConfig : public FpsConfig {
public:
	String description = "Experiment";					///< Experiment description
	Array<SessionConfig> sessions;						///< Array of sessions
	Array<TargetConfig> targets;						///< Array of trial configs   

	ExperimentConfig() {}
	
	/** Load from Any */
	ExperimentConfig(const Any& any) : FpsConfig(any) {
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Setup the default FPS config based on this
			SessionConfig::defaultConfig = (FpsConfig)(*this);												// Setup the default configuration here
			// Experiment-specific info
			reader.getIfPresent("description", description);
			reader.get("targets", targets, "Issue in the (required) \"targets\" array for the experiment!");	// Targets must be specified for the experiment
			reader.get("sessions", sessions, "Issue in the (required) \"sessions\" array for the experiment config!");
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
	}

	/** Get an array of session IDs */
	void getSessionIds(Array<String>& ids) const {
        ids.fastClear();
		for (const SessionConfig& session : sessions) { ids.append(session.id); }
	}

	/** Get a session config based on its ID */
	shared_ptr<SessionConfig> getSessionConfigById(const String& id) const {
		for (const SessionConfig& session : sessions) {
			if (! session.id.compare(id)) {
                return SessionConfig::createShared<SessionConfig>(session);
            }
		}
		return nullptr;
	}

	/** Get the index of a session in the session array (by ID) */
	int getSessionIndex(const String& id) const {
        debugAssert(sessions.size() >= 0 && sessions.size() < 100000);
        for (int i = 0; i < sessions.size(); ++i) {
			if (! sessions[i].id.compare(id)) {
                return i;
            }
		}
		throw format("Could not find session:\"%s\"", id);
	}
	
	/** Get a pointer to a target config by ID */
	shared_ptr<TargetConfig> getTargetConfigById(const String& id) const {
        for (const TargetConfig& target : targets) { 
			if (!target.id.compare(id)) {
				return TargetConfig::createShared<TargetConfig>(target);
			}
		}
		return nullptr;
	}

	Array<Array<shared_ptr<TargetConfig>>> getTargetsForSession(const String& id) const {
		return getTargetsForSession(getSessionIndex(id));
	}

	Array<Array<shared_ptr<TargetConfig>>> getTargetsForSession(int sessionIndex) const {
		Array<Array<shared_ptr<TargetConfig>>> trials;
		// Iterate through the trials
		for (int i = 0; i < sessions[sessionIndex].trials.size(); i++) {
			Array<shared_ptr<TargetConfig>> targets;
			for (String id : sessions[sessionIndex].trials[i].ids) {
				const shared_ptr<TargetConfig> t = getTargetConfigById(id);
				targets.append(t);
			}
			trials.append(targets);
		}
		return trials;
	}

	Any toAny(const bool forceAll = true) const {
		// Get the base any config
		Any a = FpsConfig::toAny();

		// Write the experiment configuration-specific 
		a["description"] = description;
		a["targets"] = targets;
		a["sessions"] = sessions;
		return a;
	}

	/** Get the experiment config from file */
	static ExperimentConfig load(String filename) {
        // if file not found, build a default
        if (!FileSystem::exists(System::findDataFile(filename, false))) {
            ExperimentConfig ex = ExperimentConfig();
			ex.toAny().save("experimentconfig.Any");
			SessionConfig::defaultConfig = (FpsConfig)ex;
			return ex;
		}
		return Any::fromFile(System::findDataFile(filename));
	}

	/** Print the experiment config to the log */
	void printToLog() {
		logPrintf("-------------------\nExperiment Config\n-------------------\nappendingDescription = %s\nscene name = %s\nFeedback Duration = %f\nReady Duration = %f\nTask Duration = %f\nMax Clicks = %d\n",
			description, sceneName, timing.feedbackDuration, timing.readyDuration, timing.taskDuration, weapon.maxAmmo);
		// Iterate through sessions and print them
		for (int i = 0; i < sessions.size(); i++) {
			SessionConfig sess = sessions[i];
			logPrintf("\t-------------------\n\tSession Config\n\t-------------------\n\tID = %s\n\tFrame Rate = %f\n\tFrame Delay = %d\n",
				sess.id, sess.render.frameRate, sess.render.frameDelay);
			// Now iterate through each run
			for (int j = 0; j < sess.trials.size(); j++) {
				logPrintf("\t\tTrial Run Config: IDs = %s, Count = %d\n",
					sess.trials[j].ids, sess.trials[j].count);
			}
		}
		// Iterate through trials and print them
		for (int i = 0; i < targets.size(); i++) {
			TargetConfig target = targets[i];
			logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tUpper Hemisphere Only = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n\tAxis Lock = [%s, %s, %s]\n",
				target.id, target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.size[0], target.size[1], target.upperHemisphereOnly ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1],
				target.axisLock[0]?"true":"false", target.axisLock[1] ? "true" : "false", target.axisLock[2] ? "true" : "false");
		}
	}
};