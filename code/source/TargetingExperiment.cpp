#include "TargetingExperiment.h"

namespace Psychophysics
{

	void TargetingExperiment::updateRenderParamsForCurrentTrial()
	{
		// get new render params from StimVariable
		renderParams.initialDisplacement.x = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "initialDisplacementDirection"));
		renderParams.initialDisplacement.y = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "initialDisplacementDistance"));
		renderParams.visualSize = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "visualSize"));
		renderParams.taskDuration = StimVariableVec[currStimVariableNum]->currStimVal;
		renderParams.speed = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "speed"));
		renderParams.motionChangePeriod = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "motionChangePeriod"));
		renderParams.readyDuration = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "readyDuration"));
		renderParams.feedbackDuration = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "feedbackDuration"));
		renderParams.weaponType = queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "weaponType");
		renderParams.weaponStrength = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "weaponStrength"));
		renderParams.frameRate = std::stof(queryStimDB(db, StimVariableVec[currStimVariableNum]->StimVariableID, "frameRate"));
	}

	void TargetingExperiment::init(std::string subjectID, std::string expVersion, int sessionNum, std::string dbLoc, bool trainingModeIn)
	{
		// turn on training mode
		trainingMode = trainingModeIn;

		// initialize presentation state
		m_app->m_presentationState = PresentationState::initial;
		m_feedbackMessage = "Aim at the target and shoot!";

		/////// Create Database ///////
		// create or open existing database at save location
		if (sqlite3_open(dbLoc.c_str(), &db) != SQLITE_OK) {
			fprintf(stderr, "Error: could not open database\n");
		}

		// apply changes depending on experiment version
		if (expVersion == "SimpleMotion")
		{
			m_conditionParams.speed = 6.0f;
		}
		else if (expVersion == "ComplexMotion")
		{
			m_conditionParams.speed = 6.0f;
			m_conditionParams.motionChangePeriod = 0.4f;
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
		int numStimVariables = (int)m_conditionParams.initialDisplacements.size() * (int)m_conditionParams.visualSizes.size();

		// create the stimulus params table
		// additional parameters can be added here if you have multiple staircases
		// for different conditions (such as fovea size, rendering method, etc)
		std::vector<std::vector<std::string>> stimParamsColumns = {
			// format: column name, data type, sqlite modifier(s)
			{ "expID", "integer", "NOT NULL" },
			{ "name", "text", "NOT NULL" },
			{ "maxTrials", "integer" },
			{ "stimLevels", "text" },
			{ "initialDisplacementDirection", "real" },
			{ "initialDisplacementDistance", "real" },
			{ "visualSize", "real" },
			{ "speed", "real" },
			{ "motionChangePeriod", "real" },
			{ "readyDuration", "real" },
			{ "feedbackDuration", "real" },
			{ "weaponType", "text" },
			{ "weaponStrength", "real" },
			{ "frameRate", "real" },
			{ "numFrameDelay", "real" },
		};
		createTableInDB(db, "stimParams", stimParamsColumns);


		int stimID;
		std::vector<std::string> stimParamsValues;

		// go through each StimVariable
		std::vector<G3D::Vector2> initialDisplacementCombos = genCombos(m_conditionParams.initialDisplacements, (int)m_conditionParams.visualSizes.size(), true);
		std::vector<float> visualSizeCombos = genCombos(m_conditionParams.visualSizes, (int)m_conditionParams.initialDisplacements.size(), false);

		for (int si = 0; si < numStimVariables; si++)
		{

			float initialDisplacementDirection = initialDisplacementCombos[si].x;
			float initialDisplacementDistance = initialDisplacementCombos[si].y;
			float visualSize = visualSizeCombos[si];

			// populate the stimulus params table
			std::vector<std::string> stimParamsValues = {
				std::to_string(expID),
				addQuotes(vec2str(std::vector<std::string>{ "initialDisplacementDirection: ", std::to_string(initialDisplacementDirection),
															", initialDisplacementDistance: ", std::to_string(initialDisplacementDistance),
															", visualSize: ", std::to_string(visualSize) }, " ")),
				std::to_string(m_conditionParams.trialCount),
				addQuotes(vec2str(m_conditionParams.taskDurationLevels, ",")),
				std::to_string(initialDisplacementDirection),
				std::to_string(initialDisplacementDistance),
				std::to_string(visualSize),
				std::to_string(m_conditionParams.speed),
				std::to_string(m_conditionParams.motionChangePeriod),
				std::to_string(m_conditionParams.readyDuration),
				std::to_string(m_conditionParams.feedbackDuration),
				addQuotes(m_conditionParams.weaponType),
				std::to_string(m_conditionParams.weaponStrength),
				std::to_string(m_conditionParams.frameRate),
				std::to_string(m_conditionParams.numFrameDelay),
			};
			assert(stimParamsValues.size() == stimParamsColumns.size(), "Incorrect number of arguments for insert to stimParams table.\n");
			stimID = insertIntoDB(db, "stimParams", stimParamsValues);

			// initialize the staircase with its id numbers
			StimVariableVec.emplace_back(std::make_shared<MCS_Stim>());
			std::dynamic_pointer_cast<MCS_Stim>(StimVariableVec[si])->init(expID, stimID, m_conditionParams.taskDurationLevels, m_conditionParams.trialCount);
		}

		// once all stim managers have been added, pick one to start
		currStimVariableNum = randInt(0, (int)StimVariableVec.size() - 1);

		updateRenderParamsForCurrentTrial();
	}


	void TargetingExperiment::updateHelper(const std::string& keyInput, const FSM::State& pastState)
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

	void TargetingExperiment::printDebugInfo()
	{
		std::cout << "STATE : " << fsm->currState << std::endl;
		std::cout << "MCS NUM : " << currStimVariableNum << std::endl;
		std::cout << "INITIAL DISPLACEMENT : " << renderParams.initialDisplacement.x << renderParams.initialDisplacement.y << std::endl;
		std::cout << "VISUAL SIZE : " << renderParams.visualSize << std::endl;
		std::cout << "STIMVAL : " << StimVariableVec[currStimVariableNum]->currStimVal << std::endl;
	}

	std::string TargetingExperiment::getDebugStr()
	{
		if (!fsm) {
			return "";
		}
		else {
			std::string debugInfo;
			debugInfo += "STATE : " + std::to_string(fsm->currState) + ' ';
			debugInfo += "MCS NUM : " + std::to_string(currStimVariableNum) + ' ';
			debugInfo += "INITIAL DISPLACEMENT : " + std::to_string(renderParams.initialDisplacement.x)
				+ ' ' + std::to_string(renderParams.initialDisplacement.y) + ' ';
			debugInfo += "VISUAL SIZE : " + std::to_string(renderParams.visualSize) + ' ';
			debugInfo += "STIMVAL : " + std::to_string(StimVariableVec[currStimVariableNum]->currStimVal) + ' ';
			return debugInfo;
		}
	}

	void TargetingExperiment::initTargetAnimation() {
		// initialize target location based on the initial displacement values
		// Not reference: we don't want it to change after the first call.
		static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_app->m_spawnDistance, 0.0f, 0.0f);
		m_app->m_motionFrame = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);
		m_app->m_motionFrame.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction

        // In task state, spawn a test target. Otherwise spawn a target at straight ahead.
        if (m_app->m_presentationState == PresentationState::task) {
            m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(renderParams.initialDisplacement.x)).approxCoordinateFrame();
            m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::yawDegrees(-renderParams.initialDisplacement.y)).approxCoordinateFrame();

            // Apply roll rotation by a random amount (random angle in degree from 0 to 360)
            float randomAngleDegree = G3D::Random::common().uniform() * 360;
            m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
        }

		// Full health for the target
		m_app->m_targetHealth = 1.f;

        // Don't reset the view. Wait for the subject to center on the ready target.
		//m_app->resetView();
	}

	void TargetingExperiment::updatePresentationState()
	{
		// This updates presentation state and also deals with data collection when each trial ends.
		PresentationState currentState = m_app->m_presentationState;
		PresentationState newState;
		double stateElapsedTime = (double)getTime();

        newState = currentState;

		if (currentState == PresentationState::initial)
		{
			if (!m_app->m_buttonUp)
			{
				m_feedbackMessage = "";
				newState = PresentationState::feedback;
			}
		}
		else if (currentState == PresentationState::ready)
		{
			if (stateElapsedTime > renderParams.readyDuration)
			{
				m_lastMotionChangeAt = 0;
                newState = PresentationState::task;
			}
		}
		else if (currentState == PresentationState::task)
		{
			if ((stateElapsedTime > renderParams.taskDuration) || (m_app->m_targetHealth <= 0))
			{
				// Communicate with psychophysics library at this point
				if (m_app->m_targetHealth <= 0)
				{
					m_app->informTrialSuccess();
				}
				else
				{
					m_app->informTrialFailure();
				}
				if (trainingMode) {
					m_feedbackMessage = std::to_string(int(stateElapsedTime * 1000)) + " ms!";
				}
				newState = PresentationState::feedback;
			}
		}
		else if (currentState == PresentationState::feedback)
		{
			if ((stateElapsedTime > renderParams.feedbackDuration) && (m_app->m_targetHealth <= 0))
			{
				m_feedbackMessage = "";
				if (isExperimentDone()) {
					newState = PresentationState::complete;
				}
				else {
					newState = PresentationState::ready;
				}
			}
		}
		else {
			newState = currentState;
		}

		if (currentState != newState)
		{ // handle state transition.
			startTimer();
			m_app->m_presentationState = newState;
            //If we switched to task, call initTargetAnimation to handle new trial
            if ((newState == PresentationState::task) || (newState == PresentationState::feedback)) {
                initTargetAnimation();
            }
		}
	}

	void TargetingExperiment::updateAnimation(RealTime framePeriod)
	{
		// 1. Update presentation state and send task performance to psychophysics library.
		updatePresentationState();

		// 2. Check if motionChange is required (happens only during 'task' state with a designated level of chance).
		if (m_app->m_presentationState == PresentationState::task)
		{
			if (getTime() > m_lastMotionChangeAt + renderParams.motionChangePeriod)
			{
				// If yes, rotate target coordinate frame by random (0~360) angle in roll direction
				m_lastMotionChangeAt = getTime();
				float randomAngleDegree = G3D::Random::common().uniform() * 360;
				m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
			}
		}

		// 3. update target location (happens only during 'task' and 'feedback' states).
		if (m_app->m_presentationState == PresentationState::task)
		{
			float rotationAngleDegree = (float)framePeriod * renderParams.speed;

			// Attempts to bound the target within visible space.
			//float currentYaw;
			//float ignore;
			//m_motionFrame.getXYZYPRDegrees(ignore, ignore, ignore, currentYaw, ignore, ignore);
			//static int reverse = 1;
			//const float change = abs(currentYaw - rotationAngleDegree);
			//// If we are headed in the wrong direction, reverse the yaw.
			//if (change > m_yawBound && change > abs(currentYaw)) {
			//    reverse *= -1;
			//}
			//m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::yawDegrees(-rotationAngleDegree * reverse)).approxCoordinateFrame();

			m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::yawDegrees(-rotationAngleDegree)).approxCoordinateFrame();

		}

		// 4. Update tunnel and target colors
		if (m_app->m_presentationState == PresentationState::ready)
		{
			if (m_app->m_targetHealth > 0)
            {
                m_app->m_targetColor = Color3::red().pow(2.0f);
            }
            else
            {
                m_app->m_targetColor = Color3::green().pow(2.0f);
                // If the target is dead, empty the projectiles
                m_app->m_projectileArray.fastClear();
            }
		}
		else if (m_app->m_presentationState == PresentationState::task)
		{
			m_app->m_targetColor = m_app->m_targetHealth * Color3::cyan().pow(2.0f) + (1.0f - m_app->m_targetHealth) * Color3::brown().pow(2.0f);
		}
		else if (m_app->m_presentationState == PresentationState::feedback)
		{
			if (m_app->m_targetHealth > 0)
			{
				m_app->m_targetColor = Color3::green().pow(2.0f);
			}
			else
			{
				m_app->m_targetColor = Color3::green().pow(2.0f);
				// If the target is dead, empty the projectiles
				m_app->m_projectileArray.fastClear();
			}
		}
		else if (m_app->m_presentationState == PresentationState::complete) {
			m_app->drawMessage("Experiment Completed. Thanks!");
		}

		// 5. Clear m_TargetArray. Append an object with m_targetLocation if necessary ('task' and 'feedback' states).
		Point3 t_pos = m_app->m_motionFrame.pointToWorldSpace(Point3(0, 0, -m_app->m_targetDistance));


        if (m_app->m_targetHealth > 0.f) {
            // Don't spawn a new target every frame
            if (m_app->m_targetArray.size() == 0) {
                m_app->spawnTarget(t_pos, renderParams.visualSize);
            }
            else {
                // TODO: don't hardcode assumption of a single target
                m_app->m_targetArray[0]->setFrame(t_pos);
            }
        }
	}

	void TargetingExperiment::setFrameRate(float fr) {
		m_conditionParams.frameRate = fr;
	}
}

