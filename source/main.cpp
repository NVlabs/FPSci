/** \file main.cpp */

#include "FPSciApp.h"
#include "NoWindow.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

	FPSciApp::startupConfig = StartupConfig::load("startupconfig.Any");

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
