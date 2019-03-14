#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"
#include "sqlHelpers.h"

class App;

enum PlayerAction { AIM, HIT, MISS };

class TargetExperiment : public Experiment
{
protected:
	float m_lastMotionChangeAt;
	sqlite3* m_db = nullptr;

	TargetExperiment(App* app) : Experiment(app) {
	}

public:
	static shared_ptr<TargetExperiment> create(App* app) {
		return createShared<TargetExperiment>(app);
	}

	/**creates a new target with randomized motion path and gives it to the app */
	void createNewTarget();
	void initTargetAnimation();

	void updatePresentationState();

	void onInit();

	void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

	void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

	void onUserInput(UserInput * ui);

	void onGraphics2D(RenderDevice * rd);

	void createResultFile();

	void processResponse();

	void recordTrialResponse();

	void recordTrajectories();

	void recordPlayerAction(PlayerAction hm);

	void closeResultFile();

	void initPsychHelper();
};
