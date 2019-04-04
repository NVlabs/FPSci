#pragma once

#include <G3D/G3D.h>
//#include "TargetEntity.h"

class Session {
public:
	float targetFrameRate = 0;
	int trials = 0;

	Session() {}
	Session(const Any& any) {
		AnyTableReader r(any);
		r.getIfPresent("targetFrameRate", targetFrameRate);
		r.getIfPresent("trials", trials);
	}
};

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

// Morgan's sample
//Array<Session> sessionArray;
//r.getIfPresent("sessions", sessionArray);

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
			reader.getIfPresent("completedSesssions", completedSessions);
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
};


class ReactionConfig {
public:
	float minimumForeperiod = 1.5f;							// Minimum time to wait before a reaction time transition
	Array<float> intensities;								// List of intensities to test

	ReactionConfig() : minimumForeperiod(1.5f) {}

	ReactionConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("minimumForeperiod", minimumForeperiod);
			reader.getIfPresent("intensities", intensities);
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
	bool elevLocked;										// Elevation locking
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

class TrialRuns {
public:
	String id;								// Trial ID (look up against trial configs)
	unsigned int trainingCount = 0;			// Number of training trials to complete
	unsigned int realCount = 0;				// Number of real trials to complete

	TrialRuns() {}

	TrialRuns(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.getIfPresent("trainingCount", trainingCount);
			reader.getIfPresent("realCount", realCount);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in TrialRuns.\n", settingsVersion);
			break;
		}
		//reader.verifyDone();
	}
};

class SessionConfig {
public:
	String id;
	float	frameRate = 240.0f;					// Target (goal) frame rate (in Hz)
	unsigned int frameDelay = 0;				// Integer frame delay (in frames)
	String	selectionOrder = "random";			// "Random", "Round Robbin", "In Order"
	Array<TrialRuns> trialRuns;					// Table of trial runs

	SessionConfig() : frameRate(240.0f), frameDelay(0), selectionOrder("random") {}

	SessionConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("id", id);
			reader.getIfPresent("FrameRate", frameRate);
			reader.getIfPresent("FrameDelay", frameDelay);
			reader.getIfPresent("SelectionOrder", selectionOrder);
			reader.get("trials", trialRuns);
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
			reader.getIfPresent("sessionOrder", sessionOrder);
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
};

