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
	String	appendingDescription;

	ExperimentConfig() : targetFrameRate(360.0f), expMode("training"), taskType("reaction"), appendingDescription("ver1") {}

	ExperimentConfig(const Any& any) {
		int settingsVersion; // used to allow different version numbers to be loaded differently
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("targetFrameRate", targetFrameRate);
			reader.getIfPresent("expMode", expMode);
			reader.getIfPresent("taskType", taskType);
			reader.getIfPresent("appendingDescription", appendingDescription);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}
		// fine to have extra entries not read
		//reader.verifyDone();
	}
};


// until we figure out the design for multiple experiments, the following is not used
class ExperimentSettings {
public:
    String weaponType;
    float targetFrameRate;
    int numFrameDelay;
    String expVersion;
};

class ExperimentSettingsList {
public:
    String subjectID;
    double mouseDPI;
    double cmp360;
    Array<ExperimentSettings> settingsList;
    int settingsVersion; // used to allow different version numbers to be loaded differently

    ExperimentSettingsList() : subjectID("anon"), mouseDPI(2400.0), cmp360(12.75) {}

    ExperimentSettingsList(const Any& any) {
        AnyTableReader reader(any);
        reader.getIfPresent("settingsVersion", settingsVersion);

        switch (settingsVersion) {
        case 1:
            reader.getIfPresent("subjectID", subjectID);
            reader.getIfPresent("mouseDPI", mouseDPI);
            reader.getIfPresent("cmp360", cmp360);

            // Read trials array
            settingsList.resize(any["experiments"].size());
            for (int i = 0; i < settingsList.size(); ++i) {
                AnyTableReader trialReader(any["experiments"][i]);
                trialReader.getIfPresent("weaponType", settingsList[i].weaponType);
                trialReader.getIfPresent("targetFrameRate", settingsList[i].targetFrameRate);
                trialReader.getIfPresent("numFrameDelay", settingsList[i].numFrameDelay);
                trialReader.getIfPresent("expVersion", settingsList[i].expVersion);
            }
            break;
        default:
            debugPrintf("Settings version '%d' not recognized.\n", settingsVersion);
            break;
        }
        // fine to have extra entries not read
        //reader.verifyDone();
    }
};
