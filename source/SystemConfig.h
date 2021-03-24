#pragma once
#include <G3D/G3D.h>

/** System-specific configuration */
class SystemConfig {
public:
	// Input parameters
	bool	hasLogger = false;		///< Indicates that a hardware logger is present in the system
	String	loggerComPort = "";		///< Indicates the COM port that the logger is on when hasLogger = True
	bool	hasSync = false;		///< Indicates that a hardware sync will occur via serial card DTR signal
	String	syncComPort = "";		///< Indicates the COM port that the sync is on when hasSync = True

	SystemConfig() {};
	SystemConfig(const Any& any);

	static SystemConfig load(String filename = "systemconfig.Any");
	Any toAny(const bool forceAll = true) const;
	void printToLog();					// Print the latency logger config to log.txt

};