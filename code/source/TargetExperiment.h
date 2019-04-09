#pragma once
#include <G3D/G3D.h>
#include "Experiment.h"
#include "App.h"
#include "sqlHelpers.h"

class App;

class TargetExperiment : public Experiment
{
protected:
	double m_lastMotionChangeAt;
	sqlite3* m_db = nullptr;
	std::vector<std::vector<std::string>> m_playerActions;
	std::vector<std::vector<std::string>> m_targetTrajectory;
	float m_speed = 0;
	int m_clickCount = 0;

	TargetExperiment(App* app) : Experiment(app) {
		// secure vector capacity large enough so as to avoid memory allocation time.
		m_playerActions.reserve(5000);
		m_targetTrajectory.reserve(5000);
	}

public:
	static shared_ptr<TargetExperiment> create(App* app) {
		return createShared<TargetExperiment>(app);
	}

	/**creates a new target with randomized motion path and gives it to the app */
	void initTargetAnimation();

	float randSign();

	void updatePresentationState();

	void onInit();

	void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);

	void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

	void onUserInput(UserInput * ui);

	void onGraphics2D(RenderDevice * rd);

	void createResultFile();

	void processResponse();

	void recordTrialResponse();

	void accumulateTrajectories();

	/** queues action with given name to insert into database when trial completes 
	@param action - one of "aim" "hit" and "miss" */
	void accumulatePlayerAction(String action);

	void closeResultFile();

	void initPsychHelper();
};
