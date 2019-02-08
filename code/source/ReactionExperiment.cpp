#include "ReactionExperiment.h"

namespace Psychophysics
{

	void ReactionExperiment::updateRenderParamsForCurrentTrial()
	{
		// get new render params from StimVariable
		renderParams.minimumForeperiod = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "minimumForeperiod"));
		renderParams.meanWaitDuration = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "meanWaitDuration"));
		renderParams.feedbackDuration = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "feedbackDuration"));
		renderParams.intensity = StimVariableVec[currStimVariableNum]->currStimVal;
		renderParams.frameRate = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "frameRate"));
	}

	void ReactionExperiment::init(std::string subjectID, std::string expVersion, int sessionNum, std::string dbLoc, bool trainingModeIn)
	{
		// turn on training mode
		trainingMode = trainingModeIn;

		// initialize presentation state
		m_app->m_presentationState = PresentationState::initial;
		m_feedbackMessage = "Reaction test. Click when green!";

		/////// Create Database ///////
		// create or open existing database at save location
		if (sqlite3_open(dbLoc.c_str(), &db) != SQLITE_OK) {
			fprintf(stderr, "Error: could not open database\n");
		}


		/////// FSM ///////
		bool waitForKeypress = true;
		bool blockDesign = false;
		// The following does not matter in this experiment because we are not reacting to key presses.
		// Variable naming needs to be more general in this app.
		KeyID keyTimer = KeyID::SPACEBAR;
		KeyID keyYes = KeyID::ONE; // ONE for success
		KeyID keyNo = KeyID::TWO; // TWO for failure
		int stimDur = 0; // ms

		fsm = std::make_shared<YesNo>();
		// Cast fsm to pointer to derived class to get access to init method.
		std::dynamic_pointer_cast<YesNo>(fsm)->init(keyYes, keyNo, waitForKeypress, blockDesign);


		/////// Experiment Table ///////
		// These are the params being recorded per experiment. Additional custom params can be added, 
		// but the schema (list of params) must be the same for each database file. Currently 
		// it's my intention that the expVersion string can accommodate most versioning information

		// create sqlite table
		std::vector<std::vector<std::string>> expParamsColumns = {
			// format: column name, data type, sqlite modifier(s)
				{ "expVersion", "text", "NOT NULL" },
				{ "dateTime", "text", "NOT NULL" },
				{ "subjectID", "text", "NOT NULL" },
				{ "sessionNum", "integer" },
				{ "trainingMode", "integer" }, // database-specific below here
				{ "keyTimer", "text" },
				{ "keyYes", "text" },
				{ "KeyNo", "text" } };
		createTableInDB(db, "expParams", expParamsColumns);

		// populate table and get experiment id number
		std::vector<std::string> expParamsValues = {
			addQuotes(expVersion),
			addQuotes(return_current_time_and_date()),
			addQuotes(subjectID),
			std::to_string(sessionNum),
			std::to_string(trainingMode), // database-specific below here
			addQuotes(getKeyString(keyTimer)),
			addQuotes(getKeyString(keyYes)),
			addQuotes(getKeyString(keyNo))
		};
		assert(expParamsValues.size() == expParamsColumns.size(), "Incorrect number of arguments for insert to expParams table.\n");
		int expID = insertIntoDB(db, "expParams", expParamsValues);


		/////// Trial Data Table ///////
		createTrialDataTable(db); // see utils.h for recorded fields


		/////// Stim Managers ///////
		int numStimVariables = 1; // currently there is only one condition (flat color) with multiple intensity levels. We may increase number of scene types (conditions)

		// create the stimulus params table
		// additional parameters can be added here if you have multiple staircases
		// for different conditions (such as fovea size, rendering method, etc)
		std::vector<std::vector<std::string>> stimParamsColumns = {
			// format: column name, data type, sqlite modifier(s)
			{ "expID", "integer", "NOT NULL" },
			{ "name", "text", "NOT NULL" },
			{ "maxTrials", "integer" },
			{ "stimLevels", "text" }, // intensity
			{ "minimumForeperiod", "real" },
			{ "meanWaitDuration", "real" },
			{ "feedbackDuration", "real" },
			{ "frameRate", "real" },
		};
		createTableInDB(db, "stimParams", stimParamsColumns);

		int stimID;
		std::vector<std::string> stimParamsValues;

		for (int si = 0; si < numStimVariables; si++)
		{
			// populate the stimulus params table
			std::vector<std::string> stimParamsValues = {
				std::to_string(expID),
				addQuotes(vec2str(std::vector<std::string>{ "minimumForeperiod: ", std::to_string(m_conditionParams.minimumForeperiod),
															", meanWaitDuration: ", std::to_string(m_conditionParams.meanWaitDuration),
															", feedbackDuration: ", std::to_string(m_conditionParams.feedbackDuration) }, " ")),
				std::to_string(m_conditionParams.trialCount),
				addQuotes(vec2str(m_conditionParams.intensities, ",")),
				std::to_string(m_conditionParams.minimumForeperiod),
				std::to_string(m_conditionParams.meanWaitDuration),
				std::to_string(m_conditionParams.feedbackDuration),
				std::to_string(m_conditionParams.frameRate),
			};
			assert(stimParamsValues.size() == stimParamsColumns.size(), "Incorrect number of arguments for insert to stimParams table.\n");
			stimID = insertIntoDB(db, "stimParams", stimParamsValues);

			// initialize the staircase with its id numbers
			StimVariableVec.emplace_back(std::make_shared<MCS_Stim>());
			std::dynamic_pointer_cast<MCS_Stim>(StimVariableVec[si])->init(expID, stimID, m_conditionParams.intensities, m_conditionParams.trialCount);
		}

		// once all stim managers have been added, pick one to start
		currStimVariableNum = randInt(0, (int)StimVariableVec.size() - 1);

		updateRenderParamsForCurrentTrial();
	}


	void ReactionExperiment::updateHelper(const std::string& keyInput, const FSM::State& pastState)
	{
		// go through the list of randomly shuffled staircases until you find one incomplete
		//std::vector<int> stRandIx = randShuffle((int)StimVariableVec.size() - 1);
		std::vector<int> stRandIx = randShuffle((int)StimVariableVec.size());
		for (int ri : stRandIx)
		{
			if (!StimVariableVec[ri]->allComplete)
			{
				currStimVariableNum = ri;
				break;
			}
		}

		// get new render params from StimVariable
		updateRenderParamsForCurrentTrial();

		if (fsm->currStimState == FSM::ISI)
		{
			renderParams.sceneType = "isi";
		}
		else
		{
			renderParams.sceneType = "ingame";
		}
	}

	void ReactionExperiment::printDebugInfo()
	{
		std::cout << "STATE : " << fsm->currState << std::endl;
		std::cout << "MCS NUM : " << currStimVariableNum << std::endl;
		std::cout << "READY DURATION : " << renderParams.minimumForeperiod << std::endl;
		std::cout << "MEAN WAIT DURATION : " << renderParams.meanWaitDuration << std::endl;
		std::cout << "STIMVAL (INTENSITY) : " << StimVariableVec[currStimVariableNum]->currStimVal << std::endl;
	}

	std::string ReactionExperiment::getDebugStr()
	{
		if (!fsm) {
			return "";
		}
		else {
			std::string debugInfo;
			debugInfo += "STATE : " + std::to_string(fsm->currState) + ' ';
			debugInfo += "MCS NUM : " + std::to_string(currStimVariableNum) + ' ';
			debugInfo += "READY DURATION : " + std::to_string(renderParams.minimumForeperiod) + ' ';
			debugInfo += "MEAN WAIT DURATION : " + std::to_string(renderParams.meanWaitDuration) + ' ';
			debugInfo += "STIMVAL (INTENSITY) : " + std::to_string(StimVariableVec[currStimVariableNum]->currStimVal) + ' ';
			return debugInfo;
		}
	}

	void ReactionExperiment::updatePresentationState(RealTime framePeriod)
	{
		// This updates presentation state and also deals with data collection when each trial ends.
		PresentationState currentState = m_app->m_presentationState;
		PresentationState newState;
		double stateElapsedTime = (double)getTime();

		if (currentState == PresentationState::initial)
		{
			if (!m_app->m_buttonUp)
			{
				m_feedbackMessage = "";
				newState = PresentationState::ready;
			}
		}
		else if (currentState == PresentationState::ready)
		{
			// start task if waited longer than minimum foreperiod AND the probabilistic condition is met (Nickerson & Burhnham 1969, Response times with nonaging foreperiods).
			float taskStartChancePerFrame = (1.0f / (float)renderParams.meanWaitDuration) * (float)framePeriod;
			if ((stateElapsedTime > renderParams.minimumForeperiod) && (G3D::Random::common().uniform() < taskStartChancePerFrame))
			{
				newState = PresentationState::task;
			}
			else if (m_reacted) // stimulus not shown yet, but responded already -> an immediate trial failure.
			{
				startTimer(); // starting timer so that we get unrealistically small number for failed trials.
				m_app->informTrialFailure();
				m_feedbackMessage = "Failure: Responded too quickly.";
				newState = PresentationState::feedback;
			}
			else { // keep waiting.
				newState = currentState;
			}
		}
		else if (currentState == PresentationState::task)
		{
			if (m_reacted)
			{
				m_app->informTrialSuccess();
				m_feedbackMessage = std::to_string(int(stateElapsedTime * 1000)) + " msec";
				newState = PresentationState::feedback;
			}
			else newState = currentState;
		}
		else if (currentState == PresentationState::feedback)
		{
			if (stateElapsedTime > renderParams.feedbackDuration)
			{
				m_reacted = false;
				if (isExperimentDone()) {
					m_feedbackMessage = "Experiment complete. Thanks!";
					newState = PresentationState::complete;
				}
				else {
					m_feedbackMessage = "";
					newState = PresentationState::ready;
				}
			}
			else newState = currentState;
		}
		else {
			newState = currentState;
		}

		if (currentState != newState)
		{ // handle state transition.
			startTimer();
			m_app->m_presentationState = newState;
		}
	}

	void ReactionExperiment::updateAnimation(RealTime framePeriod)
	{
		// 1. Update presentation state and send task performance to psychophysics library.
		updatePresentationState(framePeriod);

		// 2. Assign the background color for 2D graphics
		if (m_app->m_presentationState == PresentationState::ready) {
			m_stimColor = Color3::red() * (float)renderParams.intensity;
		}
		else if (m_app->m_presentationState == PresentationState::task) {
			m_stimColor = Color3::green() * (float)renderParams.intensity;
		}
		else if (m_app->m_presentationState == PresentationState::feedback) {
			m_stimColor = Color3::white() * 0.3f;
		}
		else if (m_app->m_presentationState == PresentationState::complete) {
			m_stimColor = Color3::white() * 0.2f;
		}
	}

	void ReactionExperiment::setFrameRate(float fr) {
		m_conditionParams.frameRate = fr;
	}
}