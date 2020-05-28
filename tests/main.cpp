
#include "FPSciTests.h"

#include <gtest/gtest.h>
#include <FPSciApp.h>
#include <crtdbg.h>

int AbortReportHook(int reportType, char* message, int* returnValue)
{
	const char* typeStr;
	switch (reportType) {
	case _CRT_WARN: typeStr = "Warning"; break;
	case _CRT_ERROR: typeStr = "Error"; break;
	case _CRT_ASSERT: typeStr = "Assertion"; break;
	default: typeStr = "<invalid report type>"; break;
	}
	printf("Abort (%s): %s\n", typeStr, message);
	fflush(stdout);
	*returnValue = 1;
	return true; // no popup!
}
// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char** argv)
{
	// Stop visual studio creating an abort popup and stalling the CI runner
	_CrtSetReportHook(AbortReportHook);

	// Hack to disable error popups during automated testing because there is no dynamic version of G3D_DEBUG_NOGUI
	G3D::_internal::_consolePrintHook = nullptr;
	G3D::_internal::_debugHook = nullptr;
	G3D::_internal::_failureHook = nullptr;

	testing::InitGoogleTest(&argc, const_cast<char**>(argv));

	// Do the same thing main.cpp does in FPSci to load its settings for the default config smoke test
	if (FileSystem::exists("startupconfig.Any")) {
		FPSciApp::startupConfig = Any::fromFile("startupconfig.Any");
		{
			G3DSpecification spec;
			spec.audio = FPSciApp::startupConfig.audioEnable;
			initGLG3D(spec);
		}
		g_defaultSettings = std::make_unique<FPSciApp::Settings>(FPSciApp::startupConfig, 1, argv);
	}

	// Create clean settings for the main test environment
	g_settings = std::make_unique<G3D::GApp::Settings>(1, argv);

	int result = RUN_ALL_TESTS();

	g_settings.reset();
	g_defaultSettings.reset();
	return result;
}
