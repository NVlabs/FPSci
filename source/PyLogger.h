#pragma once

#include <G3D/G3D.h>

class PythonLogger : ReferenceCountedObject {
protected:
	bool							m_loggerRunning = false;			///< Flag to indicate whether a python logger (for HW logger) is running (i.e. needs to be closed)
	HANDLE							m_loggerHandle = 0;					///< Process handle for the python logger instance (for HW logger) if running
	String							m_logName;							///< The log name used by the python logger instance (for HW logger) if running
	String							m_mode;								///< This stores the logging mode ("minimum" latency or "total" latency for now)
	String							m_com;
	bool							m_hasSync = false;
	String							m_syncComPort = "";

	String GetLastErrorString() {
		DWORD error = GetLastError();
		if (error) {
			LPVOID lpMsgBuf;
			DWORD bufLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
			if (bufLen) {
				LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
				std::string result(lpMsgStr, lpMsgStr + bufLen);
				LocalFree(lpMsgBuf);
				return String(result);
			}
		}
		return String();
	}
public:
	PythonLogger(String com, bool hasSync = false, String syncComPort = "") {
		m_com = com;
		m_hasSync = hasSync;
		m_syncComPort = syncComPort;
	}

	static shared_ptr<PythonLogger> create(String com, bool hasSync, String syncComPort = "") {
		return createShared<PythonLogger>(com, hasSync, syncComPort);
	}

	void run(String logName, String mode = "minimum") {
		m_logName = logName;
		m_mode = mode;

		// Variables for creating process/getting handle
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Come up w/ command string
		String cmd = "python ../scripts/\"event logger\"/software/event_logger.py " + m_com + " \"" + logName + "\"";
		if (m_hasSync) cmd += " " + m_syncComPort;

		logPrintf("Running python command: '%s'\n", cmd.c_str());

		LPSTR command = LPSTR(cmd.c_str());
		if (!CreateProcess(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			logPrintf("Failed to start logger: %s\n", GetLastErrorString());
		}
		// Update logger management variables
		m_loggerRunning = true;
		m_loggerHandle = pi.hProcess;
	}

	bool pythonMergeLogs(String basename, bool block=false) {
		String dbFile = basename + ".db";
		String eventFile = basename + "_event.csv";
		
		// If we can't find either the db output file or the csv input return false
		if (!FileSystem::exists(dbFile, false) || !FileSystem::exists(eventFile, false)) {
			logPrintf("Could not find db file: '%s' or csv file: '%s'\n", dbFile, eventFile);
			return false;
		}

		// Variables for creating process/getting handle
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		String cmd = "python ../scripts/\"event logger\"/software/event_log_insert.py " + eventFile + " " + dbFile + " " + m_mode;
		logPrintf("Running python merge script: '%s'\n", cmd.c_str());

		LPSTR command = LPSTR(cmd.c_str());
		if (!CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			logPrintf("Failed to merge results: %s\n", GetLastErrorString());
		}
		else if (block) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		logPrintf("Merge complete.\n");
		return true;
	}

	void killPythonLogger() {
		if (m_loggerRunning) TerminateProcess(m_loggerHandle, 0);
		m_loggerRunning = false;
	}

	void mergeLogToDb(bool block=false) {
		if (m_loggerRunning) {
			killPythonLogger();
			alwaysAssertM(pythonMergeLogs(m_logName, block), "Failed to merge logs! See log.txt for details...");
		}
	}

};