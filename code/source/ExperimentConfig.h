#pragma once

#include <G3D/G3D.h>

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

class ExperimentConfig {
public:
	float	targetFrameRate; // hz
	int     targetFrameLag; // integer
	String	expMode; // training or real
	String	taskType; // reaction or target
	String  expVersion;
	String	appendingDescription;
	int     trialCount;
	float   taskDuration;
	Array<Point2> initialDisplacements;

	////////////// TargetExperiment ////////////////
	String  sceneName;
	// TODO: expVersion is duplicate of motionType. Decide what to do with expVersion and motionType.
	//String  motionType; // Static, SimpleMotion, ComplexMotion
	float   readyDuration; // sec
	float   feedbackDuration;
	float   speed;
	float   motionChangePeriod;
	float   visualSize;
	////////////// TargetExperiment ////////////////

	////////////// ReactionExperiment ////////////////
	float meanWaitDuration;
	float minimumForeperiod;
	Array<float> intensities;
	////////////// ReactionExperiment ////////////////

	ExperimentConfig() : targetFrameRate(360.0f), expMode("training"), taskType("reaction"), expVersion("static"), appendingDescription("ver1"), sceneName("eSports Simple Hallway") {}


	ExperimentConfig(const Any& any) {
		int settingsVersion; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("targetFrameRate", targetFrameRate);
			reader.getIfPresent("targetFrameLag", targetFrameLag);
			reader.getIfPresent("expMode", expMode);
			reader.getIfPresent("taskType", taskType);
			reader.getIfPresent("expVersion", expVersion);
			reader.getIfPresent("appendingDescription", appendingDescription);
			reader.getIfPresent("sceneName", sceneName);            
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
		// fine to have extra entries not read
		//reader.verifyDone();
	}
};

