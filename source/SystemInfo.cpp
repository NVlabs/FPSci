#include "SystemInfo.h"

SystemInfo SystemInfo::get(void) {
	SystemInfo info;

	info.hostName = getenv("COMPUTERNAME");		// Get the host (computer) name
	info.userName = getenv("USERNAME");			// Get the current logged in username

	// Get CPU name string
	int cpuInfo[4] = { -1 };
	unsigned nExIds, i = 0;
	char cpuBrandString[0x40];
	__cpuid(cpuInfo, 0x80000000);
	nExIds = cpuInfo[0];
	for (unsigned int i = 0x80000000; i <= nExIds; i++) {
		__cpuid(cpuInfo, i);
		// Interpret CPU brand string
		switch (i) {
		case 0x80000002:
			memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
			break;
		case 0x80000003:
			memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
			break;
		case 0x80000004:
			memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
			break;
		default:
			// Removed these are they are unnecessary prints...
			//logPrintf("Couldn't get system info...\n");
			break;
		}
	}
	info.cpuName = cpuBrandString;

	// Get CPU core count
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	info.coreCount = sysInfo.dwNumberOfProcessors;

	// Get memory size
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	info.memCapacityMB = (long)(statex.ullTotalPhys / (1024 * 1024));

	// Get GPU name string
	String gpuVendor = String((char*)glGetString(GL_VENDOR)).append(" ");
	String gpuRenderer = String((char*)glGetString(GL_RENDERER));
	info.gpuName = gpuVendor.append(gpuRenderer);

	// Get display information (monitor name)
	// This seems to break on many systems/provide less than descriptive names!!!
	/*DISPLAY_DEVICE dd;
	int deviceIndex = 0;
	int monitorIndex = 0;
	EnumDisplayDevices(0, deviceIndex, &dd, 0);
	std::string deviceName = dd.DeviceName;
	EnumDisplayDevices(deviceName.c_str(), monitorIndex, &dd, 0);
	displayName = String(dd.DeviceString);*/
	info.displayName = String("TODO");

	// Get screen resolution
	info.displayXRes = GetSystemMetrics(SM_CXSCREEN);
	info.displayYRes = GetSystemMetrics(SM_CYSCREEN);

	// Get display size
	HWND const hwnd = 0;
	HDC const hdc = GetDC(hwnd);
	assert(hdc);
	info.displayXSize = GetDeviceCaps(hdc, HORZSIZE);
	info.displayYSize = GetDeviceCaps(hdc, VERTSIZE);

	return info;
}

Any SystemInfo::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	a["hostname"] = hostName;
	a["username"] = userName;
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

void SystemInfo::printToLog() {
	// Print system info to log
	logPrintf("\n-------------------\nSystem Info:\n-------------------\n\tHostname: %s\n\tUsername: %s\n\tProcessor: %s\n\tCore Count: %d\n\tMemory: %dMB\n\tGPU: %s\n\tDisplay: %s\n\tDisplay Resolution: %d x %d (px)\n\tDisplay Size: %d x %d (mm)\n\n",
		hostName, userName, cpuName, coreCount, memCapacityMB, gpuName, displayName, displayXRes, displayYRes, displayXSize, displayYSize);
}
