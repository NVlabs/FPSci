#pragma once
#include <G3D/G3D.h>

class ConfigFiles {
public:
	String name;								///< Name for the experiment
	String experimentConfigFilename;			///< Experiment configuration filename/path
	String userConfigFilename;					///< User configuration filename/path
	String userStatusFilename;					///< User status filename/path
	String keymapConfigFilename;				///< Keymap configuration filename/path
	String systemConfigFilename;				///< System configuration filename/path
	String resultsDirPath;						///< Results directory path

	ConfigFiles() {};
	ConfigFiles(String name, String expConfig, String userConfig, String userStatus, String keymapConfig, String systemConfig, String resultsDir) :
		name(name), experimentConfigFilename(expConfig), userConfigFilename(userConfig), userStatusFilename(userStatus),
		keymapConfigFilename(keymapConfig), systemConfigFilename(systemConfig), resultsDirPath(resultsDir) {};
	ConfigFiles(const Any& any);

	static ConfigFiles defaults();							// Default config filenames
	static void checkValidAnyFilename(const String& errorName, const String& path);		// Assert that the filename `path` ends in .any and report `errorName` if it doesn't
	static String formatDirPath(const String& path);		//	Returns the provided path with trailing slashes added if missing


	Any toAny(const bool forceAll = false) const;
	void populateEmptyFieldsWithDefaults(const ConfigFiles& def);
};

/** Configure how the application should start */
class StartupConfig {
public:
	bool	developerMode = false;								///< Sets whether the app is run in "developer mode" (i.e. w/ extra menus)
	bool	waypointEditorMode = false;							///< Sets whether the app is run w/ the waypoint editor available
	bool	fullscreen = true;									///< Whether the app runs in windowed mode
	Vector2 windowSize = { 1920, 980 };							///< Window size (when not run in fullscreen)

	ConfigFiles defaultExperiment = ConfigFiles::defaults();	///< Setup default list
	Array<ConfigFiles> experimentList;							///< List of configs (for various experiments)

	bool	audioEnable = true;									///< Audio on/off

	StartupConfig() {};											///< Default constructor
	StartupConfig(const Any& any);								///< Any constructor

	static StartupConfig load(const String& filename);			///< Load the startup config from file (create if needed)
	Any toAny(const bool forceAll = true) const;				///< Conver to any
};
