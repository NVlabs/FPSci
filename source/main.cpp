/** \file main.cpp */

#include "FPSciApp.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	if (FileSystem::exists("startupconfig.Any")) {
		FPSciApp::startupConfig = Any::fromFile("startupconfig.Any");
	}
	else {
		// autogenerate if it wasn't there (force all fields into this any file)
		FPSciApp::startupConfig.toAny(true).save("startupconfig.Any");
	}


	{
		G3DSpecification spec;
		spec.audio = FPSciApp::startupConfig.audioEnable;
		
		// lower audio latency to 256 / 48000 * 1000 * (3 - 1.5) = 8 ms
		// Based on Average latency (ms) = bufferlength / 48kHz * 1000ms/s * (numbuffers - 1.5)
		// See more here: https://github.com/NVlabs/abstract-fps/pull/223
		spec.audioBufferLength = 256;
		spec.audioNumBuffers = 3;

		initGLG3D(spec);
	}

	FPSciApp::Settings settings(FPSciApp::startupConfig, argc, argv);

	return FPSciApp(settings).run();
}
