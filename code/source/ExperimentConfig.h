#pragma once

#include <G3D/G3D.h>
#include "SingleThresholdMeasurement.h"

// This is a write-only structure to log information affiliated with a system
class SystemConfig {
public:
	String cpuName;
	int coreCount;
	String gpuName;
	long memCapacityMB;
	String displayName;
	int displayXRes;
	int displayYRes;
	int displayXSize;
	int displayYSize;

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
		return a;
	}
};

class UserConfig {
public:
    String subjectID = "anon";			// Subject ID (as recorded in output DB)
    double mouseDPI = 800.0;			// Mouse DPI setting
    double cmp360 = 12.75;				// Mouse sensitivity, reported as centimeters per 360�
	int currentSession = 0;				// Currently selected session
	Array<String> completedSessions;	// List of completed sessions for this user
    UserConfig() {}

    UserConfig(const Any& any) {
        int settingsVersion; // used to allow different version numbers to be loaded differently
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("subjectID", subjectID);
            reader.getIfPresent("mouseDPI", mouseDPI);
            reader.getIfPresent("cmp360", cmp360);
			reader.getIfPresent("completedSessions", completedSessions);
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
		a["settingsVersion"] = 1;							// Create a version 1 file
		a["subjectID"] = subjectID;							// Include subject ID
		a["mouseDPI"] = mouseDPI;							// Include mouse DPI
		a["cmp360"] = cmp360;								// Include cm/360
		a["completedSessions"] = completedSessions;			// Include completed sessions list
		return a;
	}

	// Simple rotine to get the user configuration from file
	static Any getUserConfig(void) {
		// load user setting from file
		if (!FileSystem::exists("userconfig.Any")) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEuserconfig.Any").c_str(), "userconfig.Any");
		}
		return Any::fromFile(System::findDataFile("userconfig.Any"));
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
			debugPrintf("Settings version '%d' not recognized in TrialConfig.\n", settingsVersion);
			break;
		}
	}
};

class TargetConfig {
public:
	String id;												// Trial ID to indentify affiliated trial runs
	bool elevLocked = false;								// Elevation locking
	float distance = 30.0f;									// Distance to the target
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
			reader.getIfPresent("accelGravity", accelGravity);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in TrialConfig.\n", settingsVersion);
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
	String expMode = "training";				// String indicating whether session is training or real
	String	selectionOrder = "random";			// "Random", "Round Robbin", "In Order"
	Array<String> trials;
	Array<int> trialCounts;

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
			reader.get("trialCounts", trialCounts);
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
	bool playMode = true;							// Developer only feature for debugging/testing
	String	taskType = "reaction";					// "Reaction" or "Target"
	String	appendingDescription = "ver0";			// Short text field for description
	String  sceneName = "eSports Simple Hallway";	// For target experiment
	float feedbackDuration = 1.0f;
	float readyDuration = 0.5f;
	float taskDuration = 100000.0f;
	int maxClicks = 10000;							// Maximum number of clicks to allow in a trial
	Array<SessionConfig> sessions;					// Array of sessions
	String sessionOrder = "random";					// Order in which to run sessions?
	Array<TargetConfig> targets;					// Array of trial configs
	Array<ReactionConfig> reactions;				// Array of reaction configs
	bool renderDecals = true;						// If bullet decals are on
	bool renderMuzzleFlash = false;					// Muzzle flash

	ExperimentConfig() {}
	
	ExperimentConfig(const Any& any) {
		int settingsVersion = 1; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("playMode", playMode);
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
			reader.getIfPresent("renderDecals", renderDecals);
			reader.getIfPresent("renderMuzzleFlash", renderMuzzleFlash);
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

	// Get a list of session IDs from the session array
	Array<String> getSessionIdArray(void) {
		Array<String> ids;
		for (int i = 0; i < sessions.size(); i++) {
			ids.append(sessions[i].id);
		}
		return ids;
	}

	SessionConfig* getSessionConfigById(String id) {
		for (int i = 0; i < sessions.size(); i++) {
			if (!sessions[i].id.compare(id)) return &sessions[i];
		}
		return NULL;
	}

	int getSessionIndex(String id) {
		for (int i = 0; i < targets.size(); i++) {
			if (!sessions[i].id.compare(id)) return i;
		}
        // wasn't found, probably will segfault
        return -1;
	}
	
	// Get a pointer to a target config by ID
	TargetConfig* getTargetConfigById(String id) {
		for (int i = 0; i < targets.size(); i++) {
			if (!targets[i].id.compare(id)) return &targets[i];
		}
		return NULL;
	}

	// Get a pointer to a reaction config by ID
	ReactionConfig* getReactionConfigById(String id) {
		for (int i = 0; i < reactions.size(); i++) {
			if (!reactions[i].id.compare(id)) return &reactions[i];
		}
		return NULL;
	}

	Array<Param> getTargetExpConditions(String id) {
		int idx = getSessionIndex(id);
		return getTargetExpConditions(idx);
	}

	// This is a kludge to quickly create experiment conditions w/ appropriate parameters
	Array<Param> getTargetExpConditions(int sessionIndex) {
		Array<Param> params;
		for (int j = 0; j < sessions[sessionIndex].trials.size(); j++) {
			String id = sessions[sessionIndex].trials[j];
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
			p.add("trialCount", (float)sessions[sessionIndex].trialCounts[j]);
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
			for (float intensity : getReactionConfigById(sessions[sessionIndex].trials[j])->intensities) {
				Param p;
				p.add("intensity", intensity);
				p.add("targetFrameRate", sessions[sessionIndex].frameRate);
				p.add("targetFrameLag", (float)sessions[sessionIndex].frameDelay);
				p.add("trialCount", (float)sessions[sessionIndex].trialCounts[j]);
				params.append(p);
			}
		}
		return params;
	}

	// Get the experiment config from file
	static ExperimentConfig getExperimentConfig(void) {
		if (!FileSystem::exists("experimentconfig.Any")) { // if file not found, copy from the sample config file.
			FileSystem::copyFile(System::findDataFile("SAMPLEexperimentconfig.Any"), "experimentconfig.Any");
		}
		return Any::fromFile(System::findDataFile("experimentconfig.Any"));
	}
};