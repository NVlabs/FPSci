/** \file main.cpp */

#include "FPSciNetworkApp.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

	{
		G3DSpecification spec;
		spec.audio = false;
		initGLG3D(spec);
	}

	GApp::Settings settings(argc, argv);

	return FPSciNetworkApp(settings).run();
}
