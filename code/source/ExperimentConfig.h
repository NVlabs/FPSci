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
    double cmp360 = 12.75;				// Mouse sensitivity, reported as centimeters per 360°
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


class TargetConfig {
public:
	String id;						// Trial ID to indentify affiliated trial runs
	float motionChangePeriod = 1.0f;// 
	float minSpeed = 0.0f;			// Minimum (world space) speed
	float maxSpeed = 5.5f;			// Maximum (world space) speed
	float minEccH = 5.0f;			// Minimnum horizontal eccentricity
	float maxEccH = 15.0f;			// Maximum horizontal eccentricity
	float minEccV = 0.0f;			// Minimum vertical eccentricity
	float maxEccV = 2.0f;			// Maximum vertical eccentricity
	float visualSize = 0.02f;		// Visual size of the target (in degrees)

	//shared_ptr<TargetEntity> target;			// Target entity to contain points (if loaded here)

	TargetConfig() {}

	TargetConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.getIfPresent("minSpeed", minSpeed);
			reader.getIfPresent("maxSpeed", maxSpeed);
			reader.getIfPresent("visualSize", visualSize);
			reader.getIfPresent("minEccH", minEccH);
			reader.getIfPresent("maxEccH", maxEccH);
			reader.getIfPresent("minEccV", minEccV);
			reader.getIfPresent("maxEccV", maxEccV);
			//reader.getIfPresent("target", target);
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
	String id;						// Trial ID (look up against trial configs)
	unsigned int trainingCount = 0;	// Number of training trials to complete
	unsigned int realCount = 0;		// Number of real trials to complete

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
	float	frameRate;				// Target (goal) frame rate (in Hz)
	unsigned int frameDelay;		// Integer frame delay (in frames)
	unsigned int trialCount;		// Number of trials that have been run in this session?
	String	selectionOrder;			// "Random", "Round Robbin", "In Order"
	Array<TrialRuns> trialRuns;		// Table of trial runs

	SessionConfig() : frameRate(240.0f), frameDelay(0), selectionOrder("Random") {}

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
	Array<SessionConfig> sessions;					// Array of sessions
	Array<TargetConfig> targets;					// Array of trial configs
	String sessionOrder = "Random";
	bool decalsEnable = true;						// If bullet decals are on
	bool muzzleFlashEnable = false;					// Muzzle flash

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
			reader.get("sessions", sessions);
			reader.get("targets", targets);
			reader.getIfPresent("feedbackDuration", feedbackDuration);
			reader.getIfPresent("readyDuration", readyDuration);
			reader.getIfPresent("taskDuration", taskDuration);
			reader.getIfPresent("sessionOrder", sessionOrder);
			reader.getIfPresent("decalsEnable", decalsEnable);
			reader.getIfPresent("muzzleFlashEnable", muzzleFlashEnable);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
		// fine to have extra entries not read
		//reader.verifyDone();
	}
};

