#pragma once

#include <G3D/G3D.h>
#include "SingleThresholdMeasurement.h"

/** Configure how the application should start */
class StartupConfig {
public:
    bool playMode = true;					///< Sets whether the experiment is run in full-screen "playMode" (true for real data)
    String experimentConfigPath = "";		///< Optional path to an experiment config file (if "experimentconfig.Any" will not be this file)
    String userConfigPath = "";				///< Optional path to a user config file (if "userconfig.Any" will not be this file)

    StartupConfig() {};

	/** Construct from any here */
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

	/** Allow this to be converted back to any */
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

/** System configuration control and logging */
class SystemConfig {
public:
	// Output/runtime read parameters
	String cpuName;			///< The vendor name of the CPU being used
	int coreCount;			///< Core count for the CPU being used
	String gpuName;			///< The vendor name of the GPU being used
	long memCapacityMB;		///< The capacity of memory (RAM) in MB
	String displayName;		///< The vendor name of the display (not currently working)
	int displayXRes;		///< The horizontal size of the display in pixels
	int displayYRes;		///< The vertical size of the display in pixels
	int displayXSize;		///< The horizontal size of the display in mm
	int displayYSize;		///< The vertical size of the display in mm

	// Input parameters
	bool hasLogger;			///< Indicates that a hardware logger is present in the system
	String loggerComPort;	///< Indicates the COM port that the logger is on when hasLogger = True
	bool hasSync;			///< Indicates that a hardware sync will occur via serial card DTR signal
	String syncComPort;		///< Indicates the COM port that the sync is on when hasSync = True

	SystemConfig() {};

	/** Construct from Any */
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
		// if file not found, copy from the sample config file
		if (!FileSystem::exists("systemconfig.Any")) { 
			FileSystem::copyFile(System::findDataFile("SAMPLEsystemconfig.Any"), "systemconfig.Any");
		}
		return Any::fromFile(System::findDataFile("systemconfig.Any"));
	}

	/** Get the system info using (windows) calls */
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
    String id = "anon";						///< Subject ID (as recorded in output DB)
    double mouseDPI = 800.0;				///< Mouse DPI setting
    double cmp360 = 12.75;					///< Mouse sensitivity, reported as centimeters per 360�
	int currentSession = 0;					///< Currently selected session

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
		return a;
	}
};

/** Class for loading a user table and getting user info */
class UserTable {
public:
	String currentUser = "None";			///< The currently active user
	Array<UserConfig> users = {};			///< A list of valid users

	UserTable() {};

	/** Load from Any */
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
	static Any load(String filename) {
		// load user setting from file
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEuserconfig.Any").c_str(), "userconfig.Any");
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
	String id;										///< User ID
	Array<String> sessionOrder = {};				///< Array containing session ordering
	Array<String> completedSessions = {};			///< Array containing all completed session ids for this user

	UserSessionStatus() {}

	/** Load user status from Any */
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
	Array<UserSessionStatus> userInfo = {};				///< Array of user status

	UserStatusTable() {}

	/** Load from Any */
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

	/** Serialzie to Any */
	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["settingsVersion"] = 1;						// Create a version 1 file
		a["users"] = userInfo;							// Include updated subject table
		return a;
	}

	/** Get the user status table from file */
	static UserStatusTable load(void) {
		if (!FileSystem::exists("userstatus.Any")) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEuserstatus.Any"), "userstatus.Any");
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
		for (auto sess : status->sessionOrder) {
			if (!status->completedSessions.contains(sess)) return sess;
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
	String id = "default";												///< Id by which to refer to this weapon
	int maxAmmo = 10000;												///< Max ammo (clicks) allowed per trial (set large for laser mode)
	float firePeriod = 0.5;												///< Minimum fire period (set to 0 for laser mode)
	bool autoFire = false;												///< Fire repeatedly when mouse is held? (set true for laser mode)
	float damagePerSecond = 2.0f;										///< Damage per second delivered (compute shot damage as damagePerSecond/firePeriod)
	String fireSound = "sound/42108__marcuslee__Laser_Wrath_6.wav"; 	///< Sound to play on fire
	bool renderModel = false;											///< Render a model for the weapon?
	Vector3 muzzleOffset = Vector3(0, 0, 0);							///< Offset to the muzzle of the weapon model
	Any modelSpec = PARSE_ANY(ArticulatedModel::Specification{			///< Basic model spec
		filename = "model/sniper/sniper.obj";
		preprocess = {
			transformGeometry(all(), Matrix4::yawDegrees(90));
			transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
		};
		scale = 0.25;
		});
	bool renderMuzzleFlash = false;										///< Render a muzzle flash when the weapon fires?
	bool renderDecals = true;											///< Render decals when the shots miss?
	bool renderBullets = false;											///< Render bullets leaving the weapon
	float bulletSpeed = 1.0f;											///< Speed to draw at for rendered rounds (in m/s)
	//String missDecal = "bullet-decal-256x256.png";					///< The decal to place where the shot misses
	float fireSpread = 0;												///< The spread of the fire
	float damageRollOffAim = 0;											///< Damage roll off w/ aim
	float damageRollOffDistance = 0;									///< Damage roll of w/ distance
	//String reticleImage;												///< Reticle image to show for this weapon

	WeaponConfig() {}

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

			reader.getIfPresent("modelSpec", modelSpec);
			reader.getIfPresent("muzzleOffset", muzzleOffset);
			//model = ArticulatedModel::create(modelSpec, "viewModel");
			
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
};

/** Class for representing a given target configuration */
class TargetConfig {
public:
	String id;												///< Trial ID to indentify affiliated trial runs
	bool elevLocked = false;								///< Elevation locking
	Array<float> distance = { 30.0f, 40.0f };				///< Distance to the target
	Array<float> motionChangePeriod = { 1.0f, 1.0f };		///< Range of motion change period in seconds
	Array<float> speed = { 0.0f, 5.5f };					///< Range of angular velocities for target
	Array<float> eccH = { 5.0f, 15.0f };					///< Range of initial horizontal eccentricity
	Array<float> eccV = { 0.0f, 2.0f };						///< Range of initial vertical eccentricity
	Array<float> visualSize = { 0.02f, 0.02f };				///< Visual size of the target (in degrees)
	bool jumpEnabled = false;								///< Flag indicating whether the target jumps
	Array<float> jumpPeriod = { 2.0f, 2.0f };				///< Range of time period between jumps in seconds
	Array<float> jumpSpeed = { 2.0f, 5.5f };				///< Range of jump speeds in meters/s
	Array<float> accelGravity = { 9.8f, 9.8f };				///< Range of acceleration due to gravity in meters/s^2

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
		scale = 0.25;
	});

	//Array<Vector3> path;		// Unused, to dictate a motion path...
	//String explosionSound;	// TODO: Add target explosion sound string here and use it for m_explosionSound

	TargetConfig() {}

	/** Load from Any */
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
			reader.getIfPresent("modelSpec", modelSpec);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in TargetConfig.\n", settingsVersion);
			break;
		}
	}
};

/** Trial count class (optional for alternate TargetConfig/count table lookup) */
class TrialCount {
public:
	Array<String> ids;						///< Trial ID list
	unsigned int count = 0;			///< Count of trials to be performed

	TrialCount() {};

	/** Load from Any */
	TrialCount(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("ids", ids);
			reader.get("count", count);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}
};

/** Configuration for a session worth of trials */
class SessionConfig {
public:
	String id;									///< Session ID
	float	frameRate = 0.0f;					///< Target (goal) frame rate (in Hz)
	unsigned int frameDelay = 0;				///< Integer frame delay (in frames)
	String  expMode = "training";				///< String indicating whether session is training or real
	Array<TrialCount> trials;					///< Array of trials (and their counts) to be performed

	SessionConfig() : frameRate(240.0f), frameDelay(0){}

	/** Load from Any */
	SessionConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("id", id);
			reader.getIfPresent("frameRate", frameRate);
			reader.getIfPresent("frameDelay", frameDelay);
			reader.getIfPresent("expMode", expMode);
			reader.get("trials", trials);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	/** Get the total number of trials in this session */
	int getTotalTrials(void) {
		int count = 0;
		for (TrialCount tc : trials) {
			count += tc.count;
		}
		return count;
	}
};

/** Experiment configuration */
class ExperimentConfig {
public:
	// Task parameters
	String	appendingDescription = "ver0";			///< Short text field for description
	String  sceneName = "eSports Simple Hallway";	///< Scene to use for the experiment
	float readyDuration = 0.5f;						///< Time in ready state in seconds
	float taskDuration = 100000.0f;					///< Maximum time spent in any one task
	float feedbackDuration = 1.0f;					///< Time in feedback state in seconds
	float fieldOfView = 103.0f;						///< Field of view (horizontal) for the user
	
	bool showHUD = false;							///< Show the Heads Up Display
	String hudFont = "dominant.fnt";				///< Font to use for Heads Up Display

	float moveRate = 0.0f;							///< Player move rate (defaults to no motion)
	bool walkMode = false;							///< Whether the player "walks" (true) or "flies" (false)
	float playerHeight = 0.6f;						///< Height for the player view (in walk mode)
	float crouchHeight = 0.3f;						///< Height for the player view (during crouch in walk mode)
	float jumpVelocity = 40.0f;						///< Jump velocity for the player
	Vector3 playerGravity = Vector3(0.0f,-5.0f, 0.0f);	///< Gravity vector

	WeaponConfig weapon;							///< Weapon to be used
	
	Array<SessionConfig> sessions;					///< Array of sessions
	Array<TargetConfig> targets;					///< Array of trial configs
    
	bool renderWeaponStatus = true;                 ///< Display weapon cooldown
    String weaponStatusSide = "left";               ///< "right" for right side, otherwise left
    
	bool renderClickPhoton = true;                  ///< Render click to photon box
    String clickPhotonSide = "right";               ///< "right" for right side, otherwise left
	
	String shader = "";								///< Option for a custom shader name

	// Target health bars
	Color3 dummyTargetColor = Color3(1.0, 0.0, 0.0);///< Default "dummy" target color
	Array<Color3> targetHealthColors = {			///< Target start/end color (based on target health)
		Color3(0.0, 1.0, 0.0), 
		Color3(1.0, 0.0, 0.0)
	};

	String explosionSound = "sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav";		///< Sound to play when target destroyed

	bool showTargetHealthBars = false;				///< Display a target health bar?
	Point2 targetHealthBarSize = Point2(100.0f, 10.0f);						///< Health bar size (in pixels)
	Point3 targetHealthBarOffset = Point3(0.0f, -50.0f, 0.0f);				///< Offset from target to health bar (in pixels)
	Point2 targetHealthBarBorderSize = Point2(2.0f, 2.0f);					///< Thickness of the target health bar border
	Color4 targetHealthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);		///< Health bar border color
	Array<Color4> targetHealthBarColors = {			///< Target health bar start/end color
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Floating combat text controls
	bool showCombatText = false;					///< Display floating combat text?
	String combatTextFont = "dominant.fnt";			///< Font to use for combat text
	float combatTextSize = 16.0;					///< Font size for floating combat text
	Color4 combatTextColor = Color4(1.0, 0.0, 0.0, 1.0);		///< The main color for floating combat text
	Color4 combatTextOutline = Color4(0.0, 0.0, 0.0, 1.0);		///< Combat text outline color
	Point3 combatTextOffset = Point3(0.0, -10.0, 0.0);			///< Initial offset for combat text
	Point3 combatTextVelocity = Point3(0.0, -100.0, 0.0);	///< Move rate/vector for combat text
	float combatTextFade = 0.98f;					///< Fade rate for combat text (0 implies don't fade)	
	float combatTextTimeout = 0.5f;					///< Time for combat text to disappear (in seconds)

	ExperimentConfig() {}
	
	/** Load from Any */
	ExperimentConfig(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("appendingDescription", appendingDescription);
			reader.getIfPresent("sceneName", sceneName);
			reader.get("sessions", sessions);
			reader.get("targets", targets);
			reader.getIfPresent("feedbackDuration", feedbackDuration);
			reader.getIfPresent("readyDuration", readyDuration);
			reader.getIfPresent("taskDuration", taskDuration);
			reader.getIfPresent("fieldOfView", fieldOfView);
			reader.getIfPresent("showHUD", showHUD);
			reader.getIfPresent("hudFont", hudFont);
			reader.getIfPresent("moveRate", moveRate);
			reader.getIfPresent("walkMode", walkMode);
			reader.getIfPresent("playerHeight", playerHeight);
			reader.getIfPresent("crouchHeight", crouchHeight);
			reader.getIfPresent("jumpVelocity", jumpVelocity);
			reader.getIfPresent("playerGravity", playerGravity);
			reader.getIfPresent("weapon", weapon);
            reader.getIfPresent("renderWeaponStatus", renderWeaponStatus);
            reader.getIfPresent("weaponStatusSide", weaponStatusSide);
            reader.getIfPresent("renderClickPhoton", renderClickPhoton);
            reader.getIfPresent("clickPhotonSide", clickPhotonSide);
			reader.getIfPresent("shader", shader);
			reader.getIfPresent("explosionSound", explosionSound);
			reader.getIfPresent("showTargetHealthBars", showTargetHealthBars);
			reader.getIfPresent("targetHealthBarSize", targetHealthBarSize);
			reader.getIfPresent("targetHealthBarOffset", targetHealthBarOffset);
			reader.getIfPresent("targetHealthBarBorderSize", targetHealthBarBorderSize);
			reader.getIfPresent("targetHealthBarBorderColor", targetHealthBarBorderColor);
			reader.getIfPresent("targetHealthColors", targetHealthColors);
			reader.getIfPresent("dummyTargetColor", dummyTargetColor);
			reader.getIfPresent("targetHealthBarColors", targetHealthBarColors);
			reader.getIfPresent("showFloatingCombatText", showCombatText);
			reader.getIfPresent("floatingCombatTextSize", combatTextSize);
			reader.getIfPresent("floatingCombatTextFont", combatTextFont);
			reader.getIfPresent("floatingCombatTextColor", combatTextColor);
			reader.getIfPresent("floatingCombatTextOutlineColor", combatTextOutline);
			reader.getIfPresent("floatingCombatTextOffset", combatTextOffset);
			reader.getIfPresent("floatingCombatTextVelocity", combatTextVelocity);
			reader.getIfPresent("floatingCombatTextFade", combatTextFade);
			reader.getIfPresent("floatingCombatTextTimeout", combatTextTimeout);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
	}

	/** Get an array of session IDs */
	Array<String> getSessIds() {
		Array<String> ids;
		for (auto sess : sessions) {
			ids.append(sess.id);
		}
		return ids;
	}

	/** Get a session config based on its ID */
	shared_ptr<SessionConfig> getSessionConfigById(String id) {
		for (int i = 0; i < sessions.size(); i++) {
			if (!sessions[i].id.compare(id)) return std::make_shared<SessionConfig>(sessions[i]);
		}
		return nullptr;
	}

	/** Get the index of a session in the session array (by ID) */
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
			if (!targets[i].id.compare(id)) {
				return std::make_shared<TargetConfig>(targets[i]);
			}
		}
		return nullptr;
	}

	/** Get experiment conditions for a given session (by ID) */
	Array<Array<Param>> getExpConditions(String id) {
		int idx = getSessionIndex(id);
		return getExpConditions(idx);
	}

	/** This is a kludge to quickly create param-based experiment conditions w/ appropriate parameters */
	Array<Array<Param>> getExpConditions(int sessionIndex) {
		Array<Array<Param>> sessParams;
		for (int j = 0; j < sessions[sessionIndex].trials.size(); j++) {
			// Append each trial worth of targets
			Array<Param> targets;
			for (String id : sessions[sessionIndex].trials[j].ids) {
				// Append training target
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
				p.add("maxMotionChangePeriod", getTargetConfigById(id)->motionChangePeriod[1]);
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
				p.add("id", id.c_str());
				if (getTargetConfigById(id)->jumpEnabled) {
					p.add("jumpEnabled", "true");
				}
				else {
					p.add("jumpEnabled", "false");
				}
				targets.append(p);
			}
			sessParams.append(targets);
		}
		return sessParams;
	}

	/** Get the experiment config from file */
	static ExperimentConfig load(String filename) {
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEexperimentconfig.Any"), "experimentconfig.Any");
		}
		return Any::fromFile(System::findDataFile(filename));
	}

	/** Print the experiment config to the log */
	void printToLog() {
		logPrintf("-------------------\nExperiment Config\n-------------------\nappendingDescription = %s\nscene name = %s\nFeedback Duration = %f\nReady Duration = %f\nTask Duration = %f\nMax Clicks = %d\n",
			appendingDescription, sceneName, feedbackDuration, readyDuration, taskDuration, weapon.maxAmmo);
		// Iterate through sessions and print them
		for (int i = 0; i < sessions.size(); i++) {
			SessionConfig sess = sessions[i];
			logPrintf("\t-------------------\n\tSession Config\n\t-------------------\n\tID = %s\n\tFrame Rate = %f\n\tFrame Delay = %d\n",
				sess.id, sess.frameRate, sess.frameDelay);
			// Now iterate through each run
			for (int j = 0; j < sess.trials.size(); j++) {
				logPrintf("\t\tTrial Run Config: IDs = %s, Count = %d\n",
					sess.trials[j].ids, sess.trials[j].count);
			}
		}
		// Iterate through trials and print them
		for (int i = 0; i < targets.size(); i++) {
			TargetConfig target = targets[i];
			logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tElevation Locked = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n",
				target.id, target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.visualSize[0], target.visualSize[1], target.elevLocked ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1]);
		}
	}
};