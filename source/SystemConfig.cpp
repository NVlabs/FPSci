#include "SystemConfig.h"

SystemConfig::SystemConfig(const Any& any) {
	AnyTableReader reader(any);
	int settingsVersion = 1;
	reader.getIfPresent("settingsVersion", settingsVersion);
	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("hasLatencyLogger", hasLogger);
		if (hasLogger) {
			reader.get("loggerComPort", loggerComPort, "Logger COM port must be provided if \"hasLogger\" = true!");
		}
		else {
			reader.getIfPresent("loggerComPort", loggerComPort);
		}

		reader.getIfPresent("hasLatencyLoggerSync", hasSync);
		if (hasSync) {
			reader.get("loggerSyncComPort", syncComPort, "Logger sync COM port must be provided if \"hasLoggerSync\" = true!");
		}
		else {
			reader.getIfPresent("loggerSyncComPort", syncComPort);
		}
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in SystemConfig.\n", settingsVersion);
		break;
	}
}

SystemConfig SystemConfig::load(String filename) {
	if (filename.empty()) { filename = "systemconfig.Any"; }
	// Create default UserConfig file
	if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, generate a default user config table
		SystemConfig defConfig = SystemConfig();
		defConfig.toAny().save(filename, true);						// Save the .any file
		return defConfig;
	}
	return Any::fromFile(System::findDataFile(filename));
}

Any SystemConfig::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	SystemConfig def;
	if (forceAll || def.hasLogger != hasLogger)			a["hasLatencyLogger"] = hasLogger;
	if (forceAll || def.loggerComPort != loggerComPort)	a["loggerComPort"] = loggerComPort;
	if (forceAll || def.hasSync != hasSync)				a["hasLatencyLoggerSync"] = hasSync;
	if (forceAll || def.syncComPort != syncComPort)		a["loggerSyncComPort"] = syncComPort;
	return a;
}

void SystemConfig::printToLog() {
	const String loggerComStr = hasLogger ? loggerComPort : "None";
	const String syncComStr = hasSync ? syncComPort : "None";
	logPrintf("-------------------\nLDAT-R Config:\n-------------------\n\tLogger Present: %s\n\tLogger COM Port: %s\n\tSync Card Present: %s\n\tSync COM Port: %s\n\n",
		hasLogger ? "True" : "False",
		loggerComStr.c_str(),
		hasSync ? "True" : "False",
		syncComStr.c_str()
	);
}