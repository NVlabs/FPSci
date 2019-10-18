#pragma once

#include <G3D/G3D.h>
#include "Param.h"

/** Configure how the application should start */
class StartupConfig {
public:
    bool playMode = true;					///< Sets whether the experiment is run in full-screen "playMode" (true for real data)
    String experimentConfigPath = "";		///< Optional path to an experiment config file (if "experimentconfig.Any" will not be this file)
    String userConfigPath = "";				///< Optional path to a user config file (if "userconfig.Any" will not be this file)
    bool audioEnable = true;                ///< Audio on/off

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
        a["playMode"] = playMode;
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
	bool hasLogger = false;			///< Indicates that a hardware logger is present in the system
	String loggerComPort = "";		///< Indicates the COM port that the logger is on when hasLogger = True
	bool hasSync = false;			///< Indicates that a hardware sync will occur via serial card DTR signal
	String syncComPort = "";		///< Indicates the COM port that the sync is on when hasSync = True

	SystemConfig() {};

	/** Construct from Any */
	SystemConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			if(!reader.getIfPresent("HasLogger", hasLogger)){
				throw "System config must specify the \"HasLogger\" flag!";
			}
			reader.getIfPresent("LoggerComPort", loggerComPort);
			if(!reader.getIfPresent("HasSync", hasSync)){
				throw "System config must specify the \"HasSync\" flag!";
			}
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
			if(!reader.getIfPresent("users", users)){
				throw "The \"users\" array must be specified in the user configuration file!";
			}
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
	String id = "anon";								///< User ID
	Array<String> sessionOrder = {};				///< Array containing session ordering
	Array<String> completedSessions = {};			///< Array containing all completed session ids for this user
	static Array<String> defaultSessionOrder;		///< Default session order

	UserSessionStatus() {}

	/** Load user status from Any */
	UserSessionStatus(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			if(!reader.getIfPresent("id", id)){
				throw "All user status fields must include the user ID!";
			}
			sessionOrder = defaultSessionOrder;
			// Check for empty session list here (no default) require a per user session order in this case
			if (sessionOrder.size() == 0 && !reader.getIfPresent("sessions", sessionOrder)) {
				throw format("Must provide \"sessions\" array for User ID:\"%s\" in user status!", id);
			}
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
	bool sequence = false;								///< Flag for whether to sequence these experiments (allow duplicates)
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
			reader.getIfPresent("sequence", sequence);
			reader.getIfPresent("sessions", defaultSessionOrder);
			UserSessionStatus::defaultSessionOrder = defaultSessionOrder;				// Set the default order here
			if(!reader.getIfPresent("users", userInfo)){
				throw "The \"users\" array must bree present in the user status file!";
			}
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
		a["sequence"] = sequence;
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
		if (sequence) {
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
	float fireSoundVol = 0.5f;											///< Volume for fire sound
	bool renderModel = false;											///< Render a model for the weapon?
	Vector3 muzzleOffset = Vector3(0, 0, 0);							///< Offset to the muzzle of the weapon model
	ArticulatedModel::Specification modelSpec;							///< Model to use for the weapon (must be specified when renderModel=true)
	bool renderMuzzleFlash = false;										///< Render a muzzle flash when the weapon fires?
	bool renderDecals = true;											///< Render decals when the shots miss?
	bool renderBullets = false;											///< Render bullets leaving the weapon
	float bulletSpeed = 1.0f;											///< Speed to draw at for rendered rounds (in m/s)
	//String missDecal = "bullet-decal-256x256.png";					///< The decal to place where the shot misses
	float fireSpread = 0;												///< The spread of the fire
	float damageRollOffAim = 0;											///< Damage roll off w/ aim
	float damageRollOffDistance = 0;									///< Damage roll of w/ distance
	//String reticleImage;												///< Reticle image to show for this weapon

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
				if(!reader.getIfPresent("modelSpec", modelSpec)){
					throw "If \"renderModel\" is set to true within a weapon config then a \"modelSpec\" must be provided!";
				}
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
class TargetConfig {
public:
	String id;												///< Trial ID to indentify affiliated trial runs
	//bool elevLocked = false;								///< Elevation locking
	bool upperHemisphereOnly = false;                       ///< Limit flying motion to upper hemisphere only
	Array<float> distance = { 30.0f, 40.0f };				///< Distance to the target
	Array<float> motionChangePeriod = { 1.0f, 1.0f };		///< Range of motion change period in seconds
	Array<float> speed = { 0.0f, 5.5f };					///< Range of angular velocities for target
	Array<float> eccH = { 5.0f, 15.0f };					///< Range of initial horizontal eccentricity
	Array<float> eccV = { 0.0f, 2.0f };						///< Range of initial vertical eccentricity
	Array<float> visualSize = { 0.2f, 0.2f };				///< Visual size of the target (in degrees)
	bool jumpEnabled = false;								///< Flag indicating whether the target jumps
	Array<float> jumpPeriod = { 2.0f, 2.0f };				///< Range of time period between jumps in seconds
	Array<float> jumpSpeed = { 2.0f, 5.5f };				///< Range of jump speeds in meters/s
	Array<float> accelGravity = { 9.8f, 9.8f };				///< Range of acceleration due to gravity in meters/s^2
	Array<Destination> destinations;						///< Array of destinations to traverse
	String destSpace = "world";								///< Space to use for destinations (implies offset) can be "world" or "player"
	float respawnCount = 0.0f;								///< Number of times to respawn
	AABox bbox = AABox();

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

	//String explosionSound;	// TODO: Add target explosion sound string here and use it for m_explosionSound

	TargetConfig() {}

	/** Load from Any */
	TargetConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			if(!reader.getIfPresent("id", id)){
				throw "An \"id\" field must be provided for every target config!";
			}
			//reader.getIfPresent("elevationLocked", elevLocked);
			reader.getIfPresent("upperHemisphereOnly", upperHemisphereOnly);
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
			reader.getIfPresent("destSpace", destSpace);
			reader.getIfPresent("destinations", destinations);
			reader.getIfPresent("respawnCount", respawnCount);
			if (destSpace == "world" && destinations.size() == 0) {
				if(!reader.getIfPresent("bounds", bbox)){
					throw format("A world-space target must either specify destinations or a bounding box. See target: \"%s\"", id);
				}
			}
			else {
				reader.getIfPresent("bounds", bbox);
			}
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
		a["visualSize"] = visualSize;
		a["modelSpec"] = modelSpec;
		if (destinations.size() > 0) {
			a["destSpace"] = destSpace;
			a["destinations"] = destinations;
		}
		else {
			a["upperHemisphereOnly"] = upperHemisphereOnly;
			a["distance"] = distance;
			a["motionChangePeriod"] = motionChangePeriod;
			a["visualSize"] = visualSize;
			a["eccH"] = eccH;
			a["eccV"] = eccV;
			a["jumpEnabled"] = jumpEnabled;
			a["jumpPeriod"] = jumpPeriod;
			a["accelGravity"] = accelGravity;
		}
		return a;
	};

	static TargetConfig load(String filename) {
		return TargetConfig(Any::fromFile(System::findDataFile(filename)));
	}
};

/** Trial count class (optional for alternate TargetConfig/count table lookup) */
class TrialCount {
public:
	Array<String> ids;			///< Trial ID list
	int count = 1;				///< Count of trials to be performed
	static int defaultCount;	///< Default count to use

	TrialCount() {};

	/** Load from Any */
	TrialCount(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);


		switch (settingsVersion) {
		case 1:
			if(!reader.getIfPresent("ids", ids)){
				throw "An \"ids\" field must be provided for each set of trials!";
			}
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
			if (!reader.getIfPresent("type", typeStr)) {
				throw "A \"type\" field must be provided with every question!";
			}
			// Pase the type and get options for multiple choice
			if (!typeStr.compare("MultipleChoice")) {
				type = Type::MultipleChoice;
				if (!reader.getIfPresent("options", options)) {
					throw "An \"options\" Array must be specified with \"MultipleChoice\" style questions!";
				}
			}
			else if (!typeStr.compare("Entry")) {
				type = Type::Entry;
			}
			else if (!typeStr.compare("Rating")) {
				type = Type::Rating;
				if (!reader.getIfPresent("options", options)) {
					throw "An \"options\" Array must be specified with \"Rating\" style questions!";
				}
			}
			else {
				throw format("Unrecognized question \"type\" String \"%s\". Valid options are \"MultipleChoice\" or \"Entry\"", typeStr);
			}

			// Get the question prompt
			if(!reader.getIfPresent("prompt", prompt)){
				throw "A \prompt\" field must be provided with every question!";
			}
			reader.getIfPresent("title", title);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in Question.\n", settingsVersion);
			break;
		}
	}

};

class FpsConfig : public ReferenceCountedObject {
public:
	int	settingsVersion = 1;						///< Settings version
	String sceneName = "";							///< Scene name

	// Rendering parameters
	float frameRate = 0.0f;							///< Target (goal) frame rate (in Hz)
	int frameDelay = 0;								///< Integer frame delay (in frames)
	String shader = "";								///< Option for a custom shader name

	// Timing parameters
	float readyDuration = 0.5f;						///< Time in ready state in seconds
	float taskDuration = 100000.0f;					///< Maximum time spent in any one task
	float feedbackDuration = 1.0f;					///< Time in feedback state in seconds

	// Trial count
	int defaultTrialCount = 5;						///< Default trial count

	// View parameters
	float hFoV = 103.0f;							///< Field of view (horizontal) for the user
	float moveRate = 0.0f;							///< Player move rate (defaults to no motion)
	float playerHeight = 1.5f;						///< Height for the player view (in walk mode)
	float crouchHeight = 0.8f;						///< Height for the player view (during crouch in walk mode)
	float jumpVelocity = 7.0f;						///< Jump velocity for the player
	float jumpInterval = 0.5f;						///< Minimum time between jumps in seconds
	bool jumpTouch = true;							///< Require the player to be touch a surface to jump?
	Vector3 playerGravity = Vector3(0.0f, -10.0f, 0.0f);		///< Gravity vector

	WeaponConfig weapon = WeaponConfig();			///< Weapon to be used

	// HUD parameters
	bool showHUD = false;							///< Master control for all HUD elements
	bool showBanner = false;						///< Show the banner display
	float bannerVertVisible = 0.41f;				///< Vertical banner visibility
	float bannerLargeFontSize = 30.0f;				///< Banner percent complete font size
	float bannerSmallFontSize = 14.0f;				///< Banner detail font size
	String hudFont = "dominant.fnt";				///< Font to use for Heads Up Display

	// Player health bar
	bool showPlayerHealthBar = false;										///< Display a player health bar?
	Point2 playerHealthBarSize = Point2(200.0f, 20.0f);						///< Player health bar size (in pixels)
	Point2 playerHealthBarPos = Point2(74.0f, 74.0f);						///< Player health bar position (in pixels)
	Point2 playerHealthBarBorderSize = Point2(2.0f, 2.0f);					///< Player health bar border size
	Color4 playerHealthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);		///< Player health bar border color
	Array<Color4> playerHealthBarColors = {									///< Player health bar start/end colors
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Click-to-photon
	bool renderClickPhoton = false;                 ///< Render click to photon box
	String clickPhotonSide = "right";               ///< "right" for right side, otherwise left
	Point2 clickPhotonSize = Point2(0.05f, 0.035f);	///< Size of the click-to-photon area (ratio of screen space)
	float clickPhotonVertPos = 0.5f;				///< Percentage of the screen down to locate the box
	Array<Color3> clickPhotonColors = {				///< Colors to apply to click to photon box
		Color3::white() * 0.2f,
		Color3::white() * 0.8f
	};

	// Weapon status
	bool showAmmo = false;													///< Display remaining ammo
	Point2 ammoPosition = Point2(64.0f, 64.0f);								///< Position of the ammo indicator text
	float ammoSize = 24.0f;													///< Font size for ammo indicator text
	Color4 ammoColor = Color4(1.0, 1.0, 1.0, 1.0);							///< Color for ammo indicator text
	Color4 ammoOutlineColor = Color4(0.0, 0.0, 0.0, 1.0);					///< Outline color for ammo indicator text
	bool renderWeaponStatus = true;											///< Display weapon cooldown
	String cooldownMode = "ring";											///< Currently "ring" and "box" are supported
	String weaponStatusSide = "left";										///< Only applied in "box" mode, can be "right" or "left"
	float cooldownInnerRadius = 40.0f;										///< Inner radius for cooldown ring
	float cooldownThickness = 10.0f;										///< Thickness of cooldown ring
	int cooldownSubdivisions = 64;											///< Number of polygon divisions in the "ring"
	Color4 cooldownColor = Color4(1.0f, 1.0f, 1.0f, 0.75f);					///< Cooldown ring color when active (transparent when inactive)

	// Target health bars
	Array<Color3> targetHealthColors = {									///< Target start/end color (based on target health)
		Color3(0.0, 1.0, 0.0),
		Color3(1.0, 0.0, 0.0)
	};
	bool showTargetHealthBars = false;										///< Display a target health bar?
	Point2 targetHealthBarSize = Point2(100.0f, 10.0f);						///< Health bar size (in pixels)
	Point3 targetHealthBarOffset = Point3(0.0f, -50.0f, 0.0f);				///< Offset from target to health bar (in pixels)
	Point2 targetHealthBarBorderSize = Point2(2.0f, 2.0f);					///< Thickness of the target health bar border
	Color4 targetHealthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);		///< Health bar border color
	Array<Color4> targetHealthBarColors = {									///< Target health bar start/end color
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Sounds
	String explosionSound = "sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav";		///< Sound to play when target destroyed
	float explosionSoundVol = 10.0f;

	// Floating combat text controls
	bool showCombatText = false;								///< Display floating combat text?
	String combatTextFont = "dominant.fnt";						///< Font to use for combat text
	float combatTextSize = 16.0;								///< Font size for floating combat text
	Color4 combatTextColor = Color4(1.0, 0.0, 0.0, 1.0);		///< The main color for floating combat text
	Color4 combatTextOutline = Color4(0.0, 0.0, 0.0, 1.0);		///< Combat text outline color
	Point3 combatTextOffset = Point3(0.0, -10.0, 0.0);			///< Initial offset for combat text
	Point3 combatTextVelocity = Point3(0.0, -100.0, 0.0);		///< Move rate/vector for combat text
	float combatTextFade = 0.98f;								///< Fade rate for combat text (0 implies don't fade)	
	float combatTextTimeout = 0.5f;								///< Time for combat text to disappear (in seconds)

	// Reference target
	float refTargetSize = 0.01f;								///< Size of the reference target
	Color3 refTargetColor = Color3(1.0, 0.0, 0.0);				///< Default reference target color

	// Questions
	Array<Question> questions;

	// Constructors
	FpsConfig(const Any& any) {
		load(any);
	}

	FpsConfig(const Any& any, FpsConfig defaultConfig) {
		*this = defaultConfig;
		load(any);
	}

	FpsConfig() {}

	void load(const Any& any) {
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("sceneName", sceneName);
			reader.getIfPresent("frameRate", frameRate);
			reader.getIfPresent("frameDelay", frameDelay);
			reader.getIfPresent("feedbackDuration", feedbackDuration);
			reader.getIfPresent("readyDuration", readyDuration);
			reader.getIfPresent("taskDuration", taskDuration);
			reader.getIfPresent("defaultTrialCount", defaultTrialCount);
			reader.getIfPresent("horizontalFieldOfView", hFoV);
			reader.getIfPresent("moveRate", moveRate);
			reader.getIfPresent("playerHeight", playerHeight);
			reader.getIfPresent("crouchHeight", crouchHeight);
			reader.getIfPresent("jumpVelocity", jumpVelocity);
			reader.getIfPresent("jumpInterval", jumpInterval);
			reader.getIfPresent("jumpTouch", jumpTouch);
			reader.getIfPresent("playerGravity", playerGravity);
			reader.getIfPresent("weapon", weapon);
			reader.getIfPresent("renderClickPhoton", renderClickPhoton);
			reader.getIfPresent("clickPhotonSide", clickPhotonSide);
			reader.getIfPresent("clickPhotonSize", clickPhotonSize);
			reader.getIfPresent("clickPhotonVertPos", clickPhotonVertPos);
			reader.getIfPresent("clickPhotonColors", clickPhotonColors);
			reader.getIfPresent("shader", shader);
			reader.getIfPresent("showHUD", showHUD);
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
			reader.getIfPresent("explosionSound", explosionSound);
			reader.getIfPresent("explosionSoundVol", explosionSoundVol);
			reader.getIfPresent("showTargetHealthBars", showTargetHealthBars);
			reader.getIfPresent("targetHealthBarSize", targetHealthBarSize);
			reader.getIfPresent("targetHealthBarOffset", targetHealthBarOffset);
			reader.getIfPresent("targetHealthBarBorderSize", targetHealthBarBorderSize);
			reader.getIfPresent("targetHealthBarBorderColor", targetHealthBarBorderColor);
			reader.getIfPresent("targetHealthColors", targetHealthColors);
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
			reader.getIfPresent("referenceTargetSize", refTargetSize);
			reader.getIfPresent("referenceTargetColor", refTargetColor);
			reader.getIfPresent("questions", questions);
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
		a["frameRate"] = frameRate;
		a["frameDelay"] = frameDelay;
		a["feedbackDuration"] = feedbackDuration;
		a["readyDuration"] = readyDuration;
		a["taskDuration"] = taskDuration;
		a["defaultTrialCount"] = defaultTrialCount;
		a["horizontalFieldOfView"] = hFoV;
		a["moveRate"] = moveRate;
		a["playerHeight"] = playerHeight;
		a["crouchHeight"] = crouchHeight;
		a["jumpVelocity"] = jumpVelocity;
		a["jumpInterval"] = jumpInterval;
		a["jumpTouch"] = jumpTouch;
		a["playerGravity"] = playerGravity;
		a["weapon"] =  weapon;
		a["renderClickPhoton"] = renderClickPhoton;
		a["clickPhotonSide"] = clickPhotonSide;
		a["clickPhotonSize"] = clickPhotonSize;
		a["clickPhotonVertPos"] = clickPhotonVertPos;
		a["clickPhotonColors"] = clickPhotonColors;
		a["shader"] = shader;
		a["showHUD"] = showHUD;
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
		a["explosionSound"] = explosionSound;
		a["explosionSoundVol"] = explosionSoundVol;
		a["showTargetHealthBars"] = showTargetHealthBars;
		a["targetHealthBarSize"] = targetHealthBarSize;
		a["targetHealthBarOffset"] = targetHealthBarOffset;
		a["targetHealthBarBorderSize"] = targetHealthBarBorderSize;
		a["targetHealthBarBorderColor"] = targetHealthBarBorderColor;
		a["targetHealthColors"] = targetHealthColors;
		a["targetHealthBarColors"] = targetHealthBarColors;
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

/** Configuration for a session worth of trials */
class SessionConfig : public FpsConfig {
public:
	String id;									///< Session ID
	String  sessDescription = "Session";		///< String indicating whether session is training or real
	Array<TrialCount> trials;					///< Array of trials (and their counts) to be performed
	static FpsConfig defaultConfig;

	SessionConfig() : FpsConfig(defaultConfig) {};

	static shared_ptr<SessionConfig> create() {
		return createShared<SessionConfig>();
	}

	/** Load from Any */
	SessionConfig(const Any& any) : FpsConfig(any, defaultConfig) {
		TrialCount::defaultCount = defaultTrialCount;
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Unique session info
			reader.getIfPresent("id", id);
			reader.getIfPresent("description", sessDescription);
			if (!reader.getIfPresent("trials", trials)){
				throw format("A \"trials\" array must be specified with each session! See session: \"%s\"", id);
			}
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		// Get the base any config
		Any a = ((FpsConfig*)this)->toAny();
		// Update w/ the session-specific fields
		a["id"] = id;
		a["description"] = sessDescription;
		a["trials"] = trials;
		return a;
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
class ExperimentConfig : public FpsConfig {
public:
	String expDescription = "Experiment";				///< Experiment description
	Array<SessionConfig> sessions;						///< Array of sessions
	Array<TargetConfig> targets;						///< Array of trial configs   

	ExperimentConfig() {}
	
	/** Load from Any */
	ExperimentConfig(const Any& any) : FpsConfig(any) {
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Experiment-specific info
			reader.getIfPresent("description", expDescription);
			if(!reader.getIfPresent("targets", targets)){												// Get the targets (required)

				throw "At least one target must be specified for the experiment!";
			}
			SessionConfig::defaultConfig = (FpsConfig)(*this);											// Setup the default configuration here
			if(!reader.getIfPresent("sessions", sessions)){												// Get the sessions (required)
				throw "The \"sessions\" array must be provided as part of the experiment config!";
			}
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
				shared_ptr<TargetConfig> target = getTargetConfigById(id);
				p.add("minEccH", target->eccH[0]);
				p.add("minEccV", target->eccV[0]);
				p.add("maxEccH", target->eccH[1]);
				p.add("maxEccV", target->eccV[1]);
				p.add("targetFrameRate", sessions[sessionIndex].frameRate);
				p.add("targetFrameLag", (float)sessions[sessionIndex].frameDelay);
				p.add("minVisualSize", target->visualSize[0]);
				p.add("maxVisualSize", target->visualSize[1]);
				p.add("minMotionChangePeriod", target->motionChangePeriod[0]);
				p.add("maxMotionChangePeriod", target->motionChangePeriod[1]);
				p.add("upperHemisphereOnly", (float)target->upperHemisphereOnly);
				p.add("minSpeed", target->speed[0]);
				p.add("maxSpeed", target->speed[1]);
				p.add("minDistance", target->distance[0]);
				p.add("maxDistance", target->distance[1]);
				p.add("minJumpPeriod", target->jumpPeriod[0]);
				p.add("maxJumpPeriod", target->jumpPeriod[1]);
				p.add("minJumpSpeed", target->jumpSpeed[0]);
				p.add("maxJumpSpeed", target->jumpSpeed[1]);
				p.add("minGravity", target->accelGravity[0]);
				p.add("maxGravity", target->accelGravity[1]);
				p.add("trial_idx", (float)j);
				p.add("trialCount", (float)sessions[sessionIndex].trials[j].count);
				p.add("id", id.c_str());
				p.add("sessionID", sessions[sessionIndex].id.c_str());
				p.add("destCount", (float)target->destinations.size());
				p.add("destSpace", target->destSpace.c_str());
				p.add("respawns", (float)target->respawnCount);
				p.destinations = target->destinations;
				p.bounds = target->bbox;
				String modelName = target->modelSpec["filename"];
				p.add("model", modelName.c_str());
				if (target->jumpEnabled) {
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

	Any toAny(const bool forceAll = true) const {
		// Get the base any config
		Any a = ((FpsConfig*)this)->toAny();
		// Write the experiment configuration-specific 
		a["description"] = expDescription;
		a["targets"] = targets;
		a["sessions"] = sessions;
		return a;
	}

	/** Get the experiment config from file */
	static ExperimentConfig load(String filename) {
		if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, build a default
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
			expDescription, sceneName, feedbackDuration, readyDuration, taskDuration, weapon.maxAmmo);
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
			logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tUpper Hemisphere Only = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n",
				target.id, target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.visualSize[0], target.visualSize[1], target.upperHemisphereOnly ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1]);
		}
	}
};