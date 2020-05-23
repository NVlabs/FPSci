/** \file main.cpp */

#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	if (FileSystem::exists("startupconfig.Any")) {
		App::startupConfig = Any::fromFile("startupconfig.Any");
	}
	else {
		// autogenerate if it wasn't there (force all fields into this any file)
		App::startupConfig.toAny(true).save("startupconfig.Any");
	}


	{
		G3DSpecification spec;
		spec.audio = App::startupConfig.audioEnable;
		initGLG3D(spec);
	}

	App::Settings settings(App::startupConfig, argc, argv);

	return App(settings).run();
}
