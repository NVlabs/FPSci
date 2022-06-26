/** \file main.cpp */

#include "FPSciNetworkApp.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

	FPSciNetworkApp::startupConfig = StartupConfig::load("startupconifg.Any");

	{
		G3DSpecification spec;
		spec.audio = false;
		initGLG3D(spec);
	}

	FPSciNetworkApp::Settings settings(FPSciNetworkApp::startupConfig, argc, argv);
	settings.window.visible = false;
	settings.window.fullScreen = false;

	return FPSciNetworkApp(settings).run();
}
