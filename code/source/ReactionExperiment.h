#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"

class App;

namespace AbstractFPS
{
	class ReactionExperiment : public Experiment
	{
	protected:
		App* m_app;

	public:
		ReactionExperiment(App* app) : Experiment() {
			m_app = app;
		}

		void onInit();

		void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

		void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

		void onUserInput(UserInput * ui);

		void onGraphics2D(RenderDevice * rd, Array<shared_ptr<Surface2D>>& posed2D);
	};
}