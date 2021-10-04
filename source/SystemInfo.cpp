#include "SystemInfo.h"
#include <setupapi.h>
#include <devguid.h>

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

	// Get USB info
	Array<String> VIDs; Array<String> PIDs;
	int usbCount = getUSBInfo(VIDs, PIDs);

	Any::AnyTable lookup;
	bool haveVIDPIDLookup = loadVIDPIDTable("vid_pid.json", lookup);
	for (int i = 0; i < usbCount; i++) {
		// Add each VID/PID pair to the info structure
		String devString = format("VID = %s, PID = %s", VIDs[i], PIDs[i]);
		if (haveVIDPIDLookup) {
			String vendor = "Unknown vendor";
			String product = "Unknown product";
			if (lookup.containsKey(VIDs[i])) {
				vendor = lookup[VIDs[i]]["vendor"];
				if (lookup[VIDs[i]]["pids"].containsKey(PIDs[i])) {
					product = lookup[VIDs[i]]["pids"][PIDs[i]];
				}
			}
			devString += format(" (%s %s)", vendor.c_str(), product.c_str());
		}
		info.usbDevices.append(devString);
	}

	return info;
}

int SystemInfo::getUSBInfo(Array<String>& VIDs, Array<String>& PIDs) {
	HDEVINFO deviceInfoSet;
	GUID* guidDev = (GUID*)&GUID_DEVCLASS_USB;
	deviceInfoSet = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
	int memberIndex = 0;
	int validCount = 0;

	while (true) {
		// Initialize storage
		SP_DEVINFO_DATA deviceInfoData;
		ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		// Get device info (one at a time)
		if (!SetupDiEnumDeviceInfo(deviceInfoSet, memberIndex, &deviceInfoData)) {
			if (GetLastError() == ERROR_NO_MORE_ITEMS) {
				break;      // Exit the while loop once all devices have been found
			}
		}
		memberIndex++;

		// Storage for VID/PID string
		TCHAR buffer[4000];
		DWORD nSize = 0;
		SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, buffer, sizeof(buffer), &nSize);
		buffer[nSize] = '\0';       // Add string null terminator

		// Convert to G3D::String
		String vidpidStr = String(buffer);

		// Validate the VID/PID string to make sure it is a USB device w/ VID and PID present
		if (!vidpidStr.rfind("USB", 0) == 0) continue;                  // Skip non-USB devices
		if (vidpidStr.rfind("VID") == std::string::npos) continue;      // Skip USB devices without VID present

		// Extract VID and PID substrings
		String vid = vidpidStr.substr(8, 4);
		String pid = vidpidStr.substr(17, 4);
		VIDs.append(vid);
		PIDs.append(pid);
		validCount++;
	}
	if (deviceInfoSet) {
		SetupDiDestroyDeviceInfoList(deviceInfoSet);
	}
	return validCount;
}

bool SystemInfo::loadVIDPIDTable(const String& filename, Any::AnyTable& table) {
	if (!FileSystem::exists(System::findDataFile(filename, false))) {
		return false;
	}
	Any raw;
	raw.load(System::findDataFile(filename));
	table = raw.table();
	return true;
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
	a["USBDevices"] = usbDevices;
	return a;
}

void SystemInfo::printToLog() {
	// Print system info to log
	logPrintf("\n-------------------\nSystem Info:\n-------------------\n\tHostname: %s\n\tUsername: %s\n\tProcessor: %s\n\tCore Count: %d\n\tMemory: %dMB\n\tGPU: %s\n\tDisplay: %s\n\tDisplay Resolution: %d x %d (px)\n\tDisplay Size: %d x %d (mm)\n",
		hostName, userName, cpuName, coreCount, memCapacityMB, gpuName, displayName, displayXRes, displayYRes, displayXSize, displayYSize);
	// Print USB devices to the log
	logPrintf("\tUSB Devices:\n");
	for (String devString : usbDevices) {
		logPrintf("\t\t%s\n", devString);
	}
	logPrintf("\n");
}
