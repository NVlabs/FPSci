#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"

class App;

namespace AbstractFPS
{
	class ReactionExperiment : public Experiment
	{
	public:
		static shared_ptr<ReactionExperiment> create() {
			return createShared<ReactionExperiment>();
		}

		void onInit();

		void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

		void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

		void onUserInput(UserInput * ui);

		void onGraphics2D(RenderDevice * rd, Array<shared_ptr<Surface2D>>& posed2D);
	};
}