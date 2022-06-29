#include "StartupConfig.h"
#include "ExperimentConfig.h"
#include "FPSciAnyTableReader.h"

ConfigFiles::ConfigFiles(const Any& any) {
	FPSciAnyTableReader reader(any);

	reader.get("name", name, "Must provide a name for every entry in the experimentList!");

	reader.getIfPresent("experimentConfigFilename", experimentConfigFilename);
	checkValidAnyFilename("experimentConfigFilename", experimentConfigFilename);

	reader.getIfPresent("userConfigFilename", userConfigFilename);
	checkValidAnyFilename("userConfigFilename", userConfigFilename);

	reader.getIfPresent("userStatusFilename", userStatusFilename);
	checkValidAnyFilename("userStatusFilename", userStatusFilename);

	reader.getIfPresent("keymapConfigFilename", keymapConfigFilename);
	checkValidAnyFilename("keymapConfigFilename", keymapConfigFilename);

	reader.getIfPresent("systemConfigFilename", systemConfigFilename);
	checkValidAnyFilename("systemConfigFilename", systemConfigFilename);

	reader.getIfPresent("resultsDirPath", resultsDirPath);
	resultsDirPath = formatDirPath(resultsDirPath);
}

ConfigFiles ConfigFiles::defaults() {
	return ConfigFiles("default", "experimentconfig.Any", "userconfig.Any", "userstatus.Any", "keymap.Any", "systemconfig.Any", "./results");
}

Any ConfigFiles::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	a["name"] = name;
	a["experimentConfigFilename"] = experimentConfigFilename;
	a["userConfigFilename"] = userConfigFilename;
	a["userStatusFilename"] = userStatusFilename;
	a["keymapConfigFilename"] = keymapConfigFilename;
	a["systemConfigFilename"] = systemConfigFilename;
	a["resultsDirPath"] = resultsDirPath;
	return a;
}

void ConfigFiles::populateEmptyFieldsWithDefaults(const ConfigFiles& def) {
	if (experimentConfigFilename.empty()) { experimentConfigFilename = def.experimentConfigFilename; }
	if (userConfigFilename.empty()) { userConfigFilename = def.userConfigFilename; }
	if (userStatusFilename.empty()) { userStatusFilename = def.userStatusFilename; }
	if (keymapConfigFilename.empty()) { keymapConfigFilename = def.keymapConfigFilename; }
	if (systemConfigFilename.empty()) { systemConfigFilename = def.systemConfigFilename; }
	if (resultsDirPath.empty()) { resultsDirPath = def.resultsDirPath; }
}


void ConfigFiles::checkValidAnyFilename(const String& errorName, const String& path) {
	if (path.empty()) return;	// Allow empty paths (will be replaced)
	alwaysAssertM(toLower(path.substr(path.length() - 4)) == ".any", "Config filenames specified in the startup config must end with \".any\"!, check the " + errorName + "!\n");
}

String ConfigFiles::formatDirPath(const String& path) {
	String fpath = path;
	if (!path.empty() && path.substr(path.length() - 1) != "/") { fpath = path + "/"; }
	return fpath;
}


StartupConfig StartupConfig::load(const String& filename) {
	StartupConfig config;
	if (!FileSystem::exists(filename)) {
		config.toAny(true).save(filename, config.jsonAnyOutput);		// If the file doesn't exist create it
	}
	return Any::fromFile(filename);				// Load back from the Any file
}

StartupConfig::StartupConfig(const Any& any) {
	bool foundDefault = false;
	int settingsVersion = 1;
	FPSciAnyTableReader reader(any);
	reader.getIfPresent("settingsVersion", settingsVersion);
	Array<String> sampleFilenames;
	ConfigFiles sampleExperiment;

	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("developerMode", developerMode);
		reader.getIfPresent("waypointEditorMode", waypointEditorMode);
		reader.getIfPresent("fullscreen", fullscreen);
		reader.getIfPresent("windowSize", windowSize);
		reader.getIfPresent("jsonAnyOutput", jsonAnyOutput);
		
		foundDefault = reader.getIfPresent("defaultExperiment", defaultExperiment);
		if (!foundDefault) {
			logPrintf("Warning: no `defaultExperiment` found in `startupConfig.Any`, you may be using an old experiment config.\n");
			String experimentConfigFilename, userConfigFilename, userStatusFilename, keymapConfigFilename, systemConfigFilename, resultsDirPath;
			reader.getIfPresent("experimentConfigFilename", experimentConfigFilename);
			reader.getIfPresent("userConfigFilename", userConfigFilename);
			reader.getIfPresent("userStatusFilename", userStatusFilename);
			reader.getIfPresent("keymapConfigFilename", keymapConfigFilename);
			reader.getIfPresent("systemConfigFilename", systemConfigFilename);
			reader.getIfPresent("resultsDirPath", resultsDirPath);
			logPrintf("One possible fix is to use the following in `startupConfig.Any`:\n");
			logPrintf("    defaultExperiment = {\n");
			logPrintf("        name = \"default\";\n");
			logPrintf("        experimentConfigFilename = \"%s\";\n", experimentConfigFilename);
			logPrintf("        userConfigFilename = \"%s\";\n", userConfigFilename);
			logPrintf("        userStatusFilename = \"%s\";\n", userStatusFilename);
			logPrintf("        keymapConfigFilename = \"%s\";\n", keymapConfigFilename);
			logPrintf("        systemConfigFilename = \"%s\";\n", systemConfigFilename);
			logPrintf("        resultsDirPath = \"%s\";\n", resultsDirPath);
			logPrintf("    };\n");
		}

		reader.getIfPresent("experimentList", experimentList);
		for (ConfigFiles& files : experimentList) { files.populateEmptyFieldsWithDefaults(defaultExperiment); }
		if (experimentList.length() == 0) { experimentList.append(defaultExperiment); }

		logPrintf("Searching 'samples' directory for sample experiments.\n");
		FileSystem::getFiles("samples/*.Experiment.Any", sampleFilenames);
		sampleExperiment.populateEmptyFieldsWithDefaults(defaultExperiment);
		sampleExperiment.userConfigFilename = format("samples/sample.User.Any");
		for (String& sampleFile : sampleFilenames) {
			String name = sampleFile.substr(0, sampleFile.find_first_of('.'));
			sampleExperiment.experimentConfigFilename = format("samples/%s", sampleFile);
			sampleExperiment.name = format("[Sample]%s", name);
			sampleExperiment.userStatusFilename = format("samples/%s.Status.Any", name);
			if (!FileSystem::exists(sampleExperiment.userStatusFilename)) {
				logPrintf("Skipping Sample '%s' because user status '%s' doesn't exist\n", name, sampleExperiment.userStatusFilename);
				continue;
			}
			experimentList.append(sampleExperiment);
			logPrintf("Sample '%s' added to experiment list\n", sampleExperiment.name);
		}
		logPrintf("\n");

		reader.getIfPresent("audioEnable", audioEnable);
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in StartupConfig.\n", settingsVersion);
		break;
	}
}

Any StartupConfig::toAny(const bool forceAll) const {
	StartupConfig def;		// Create a dummy default config for value testing
	Any a(Any::TABLE);
	if (forceAll || def.developerMode != developerMode)								a["developerMode"] = developerMode;
	if (forceAll || def.waypointEditorMode != waypointEditorMode)					a["waypointEditorMode"] = waypointEditorMode;
	if (forceAll || def.fullscreen != fullscreen)									a["fullscreen"] = fullscreen;
	if (forceAll || def.audioEnable != audioEnable)									a["audioEnable"] = audioEnable;
	if (forceAll || def.jsonAnyOutput != jsonAnyOutput)									a["jsonAnyOutput"] = jsonAnyOutput;
	a["defaultExperiment"] = defaultExperiment;
	a["experimentList"] = experimentList;

	return a;
}

bool StartupConfig::validateExperiments() const {
	// Validate experiment configs
	bool valid = true;
	for (auto& configs : experimentList) {
		ExperimentConfig experimentConfig = ExperimentConfig::load(configs.experimentConfigFilename, jsonAnyOutput);
		logPrintf("Validating experiment '%s'\n", configs.name);
		if (!experimentConfig.validate(false)) {
			logPrintf("  Error: experiment '%s' is not valid! (See '%s')\n", configs.name, configs.experimentConfigFilename);
			valid = false;
		}
	}
	return valid;
}