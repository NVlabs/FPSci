/** \file main.cpp */

#include "FPSciServerApp.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

	{
		G3DSpecification spec;
		spec.audio = false;
		spec.logFilename = "serverlog.txt";
		initGLG3D(spec);
	}

	FPSciServerApp::startupConfig = StartupConfig::load("serverstartupconfig.Any");

	FPSciServerApp::Settings settings(FPSciServerApp::startupConfig, argc, argv);
	//settings.window.visible = false;
	//settings.window.fullScreen = false;

	return FPSciServerApp(settings).run();
}
