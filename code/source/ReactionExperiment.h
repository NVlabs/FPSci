#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"
#include "sqlHelpers.h"

class App;

class ReactionExperiment : public Experiment
{
protected:
	bool m_reacted = false;
	String m_trialStartTime, m_trialEndTime;
	sqlite3* m_db = nullptr;

	ReactionExperiment(App* app) : Experiment(app) {
	}

public:
	static shared_ptr<ReactionExperiment> create(App* app) {
		return createShared<ReactionExperiment>(app);
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

	void onGraphics2D(RenderDevice * rd);

	void processResponse();

	void recordTrialResponse();

	bool initPsychHelper();
};
