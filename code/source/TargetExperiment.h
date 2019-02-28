#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"

class App;

namespace AbstractFPS
{
	class TargetExperiment : public Experiment
	{
	public:
		//static shared_ptr<TargetExperiment> create() {
		//	return createShared<TargetExperiment>();
		//}

		TargetExperiment(App* app) : Experiment() {
			m_app = app;
		}

		void initTargetAnimation();

		void updatePresentationState();

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

		String m_feedbackMessage;
		float m_lastMotionChangeAt;
		App *m_app;
	};
}
