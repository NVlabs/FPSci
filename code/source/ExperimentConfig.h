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

// Morgan's sample
//Array<Session> sessionArray;
//r.getIfPresent("sessions", sessionArray);

class UserConfig {
public:
    String subjectID;
    double mouseDPI;
    double cmp360;
    UserConfig() : subjectID("anon"), mouseDPI(2400.0), cmp360(12.75) {}

    UserConfig(const Any& any) {
        int settingsVersion; // used to allow different version numbers to be loaded differently
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("subjectID", subjectID);
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
};


class TrialConfig {
public:
	String id;						// Trial ID to indentify affiliated trial runs
	float motionChangePeriod;		// 
	float minSpeed;					// Minimum (world space) speed
	float maxSpeed;					// Maximum (world space) speed
	//shared_ptr<TargetEntity> target;			// Target entity to contain points (if loaded here)

	TrialConfig(): motionChangePeriod(1.0), minSpeed(0), maxSpeed(5.5) {}

	TrialConfig(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("id", id);
			reader.getIfPresent("minSpeed", minSpeed);
			reader.getIfPresent("maxSpeed", maxSpeed);
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
	unsigned int trainingCount;		// Number of training trials to complete
	unsigned int realCount;			// Number of real trials to complete

	TrialRuns() : trainingCount(0), realCount(0) {}

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
	bool playMode;					// Developer only feature for debugging/testing
	String	taskType;				// "Reaction" or "Target"
	String	appendingDescription;	// Short text field for description
	String  sceneName;				// For target experiment
	Array<SessionConfig> sessions;		// Array of sessions
	Array<TrialConfig> trials;			// Array of trial configs
	String expMode = "training";

	ExperimentConfig() : playMode(true), taskType("reaction"), appendingDescription("ver1"), sceneName("eSports Simple Hallway") {}
	
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
			reader.getIfPresent("sessions", sessions);
			reader.getIfPresent("trials", trials);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
		// fine to have extra entries not read
		//reader.verifyDone();
	}
};

