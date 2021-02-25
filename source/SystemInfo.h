#pragma once
#include <G3D/G3D.h>

/** Information about the system being used
The current implementation is heavily Windows-specific */
class SystemInfo {
public:
	// Output/runtime read parameters
	String	hostName;			///< System host (PC) name
	String  userName;			///< System username
	String	cpuName;			///< The vendor name of the CPU being used
	int		coreCount;			///< Core count for the CPU being used
	String	gpuName;			///< The vendor name of the GPU being used
	long	memCapacityMB;		///< The capacity of memory (RAM) in MB
	String	displayName;		///< The vendor name of the display (not currently working)
	int		displayXRes;		///< The horizontal size of the display in pixels
	int		displayYRes;		///< The vertical size of the display in pixels
	int		displayXSize;		///< The horizontal size of the display in mm
	int		displayYSize;		///< The vertical size of the display in mm

	static SystemInfo get(void);						// Get the system info using (windows) calls

	Any toAny(const bool forceAll = true) const;
	void printToLog();
};