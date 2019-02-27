#pragma once
#include <G3D/G3D.h>
#include "PsychHelper.h"
#include "App.h"

class App;

namespace AbstractFPS
{
	class ReactionExperiment : public ReferenceCountedObject
	{
	protected:
		App* m_app;

	public:
		static shared_ptr<ReactionExperiment> create() {
			return createShared<ReactionExperiment>();
		}

		ReactionExperiment(App* app) : ReferenceCountedObject() {
			m_app = app;
		}

		void onInit();

		void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

		void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

		void onUserInput(UserInput * ui);

		void onGraphics2D(RenderDevice * rd, Array<shared_ptr<Surface2D>>& posed2D);
	};
}