/** \file main.cpp */

#include "FPSciNetworkApp.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

	{
		G3DSpecification spec;
		spec.audio = false;
		spec.logFilename = "serverlog.txt";
		initGLG3D(spec);
	}

	FPSciNetworkApp::startupConfig = StartupConfig::load("serverstartupconfig.Any");

	FPSciNetworkApp::Settings settings(FPSciNetworkApp::startupConfig, argc, argv);
	//settings.window.visible = false;
	//settings.window.fullScreen = false;

	return FPSciNetworkApp(settings).run();
}
