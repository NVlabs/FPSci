#pragma once

#include <G3D/G3D.h>

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
	String	expMode; // training or real
	String	taskType; // reaction or targeting
	String  expVersion;
	String	appendingDescription;
    String  sceneName;

	ExperimentConfig() : targetFrameRate(360.0f), expMode("training"), taskType("reaction"), expVersion("static"), appendingDescription("ver1"), sceneName("eSports Simple Hallway") {}

	ExperimentConfig(const Any& any) {
		int settingsVersion; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("targetFrameRate", targetFrameRate);
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

