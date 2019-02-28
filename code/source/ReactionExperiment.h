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

		/** Variables specific to this experiment */
		Color3 m_stimColor = Color3::white();

		/** Functions specific to this experiment */
		void updatePresentationState(RealTime framePeriod);

		/** To be replaced with call back functions */
		void onInit();

		void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

		void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

		void onUserInput(UserInput * ui);

		void onGraphics2D(RenderDevice * rd, Array<shared_ptr<Surface2D>>& posed2D);

		void createResultFile();

		void recordTrialResponse();

		void closeResultFile();

		void initPsychHelper();

		float m_taskExecutionTime;
		int m_response;

		bool m_reacted = false;
		String m_feedbackMessage;
	};
}