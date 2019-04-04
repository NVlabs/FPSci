#include "TargetExperiment.h"
#include "TargetEntity.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <map>

void TargetExperiment::initPsychHelper()
{
	// Define properties of psychophysical methods
	PsychophysicsDesignParameter psychParam;
	psychParam.mMeasuringMethod = PsychophysicsMethod::MethodOfConstantStimuli;
	psychParam.mIsDefault = false;
	psychParam.mStimLevels.push_back(m_config.val["taskDuration"]); // Shorter task is more difficult. However, we are currently doing unlimited time.
	psychParam.mMaxTrialCounts.push_back((int)m_config.val["trialCount"]);

	// Add conditions, one per one initial displacement value.
	// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
	for (int i = 0; i < m_config.val_vec["minSpeeds"].size(); ++i)
	{
		// insert all the values potentially useful for analysis.
		Param p;
		p.add("minEccH", m_config.val["minEccH"]);
		p.add("minEccV", m_config.val["minEccV"]);
		p.add("maxEccH", m_config.val["maxEccH"]);
		p.add("maxEccV", m_config.val["maxEccV"]);
		p.add("targetFrameRate", m_config.val["targetFrameRate"]);
		p.add("targetFrameLag", m_config.val["targetFrameLag"]);
		p.add("visualSize", m_config.val["visualSize"]);
		p.add("motionChangePeriod", m_config.val_vec["motionChangePeriods"][i]);
		p.add("minSpeed", m_config.val_vec["minSpeeds"][i]);
		p.add("maxSpeed", m_config.val_vec["maxSpeeds"][i]);
		// soon we need to add frame delay as well.
		m_psych.addCondition(p, psychParam);
	}

	// call it once all conditions are defined.
	m_psych.chooseNextCondition();
}

void TargetExperiment::onInit() {
	// load the experiment background scene.
	m_app->loadScene(m_app->m_experimentConfig.sceneName);

	// initialize presentation states
	m_app->m_presentationState = PresentationState::initial;
	m_feedbackMessage = "Aim at the target and shoot!";

	// default values
	// TODO: This should all move into configuration file.
	m_config.add("targetFrameRate", m_app->m_experimentConfig.sessions[m_app->m_user.currentSession].frameRate);
	m_config.add("targetFrameLag", (float)m_app->m_experimentConfig.sessions[m_app->m_user.currentSession].frameDelay);
	m_config.add("feedbackDuration", m_app->m_experimentConfig.feedbackDuration);
	m_config.add("readyDuration", m_app->m_experimentConfig.readyDuration);
	m_config.add("taskDuration", m_app->m_experimentConfig.taskDuration);
	m_config.add("trialCount", 200.f);
	m_config.add("visualSize", m_app->m_experimentConfig.targets[0].visualSize);
	m_config.add("minEccH", m_app->m_experimentConfig.targets[0].minEccH);
	m_config.add("maxEccH", m_app->m_experimentConfig.targets[0].maxEccH);
	m_config.add("minEccV", m_app->m_experimentConfig.targets[0].minEccV);
	m_config.add("maxEccV", m_app->m_experimentConfig.targets[0].maxEccV);
	// The following three parameters must have the same number (N) of elements.
	// They are to be joined by their indices to define N task conditions.
	m_config.add("motionChangePeriods", std::vector<float>{100000.0, 100000.0, 0.5});
	m_config.add("minSpeeds", std::vector<float>{0.0, 8.f, 8.f});
	m_config.add("maxSpeeds", std::vector<float>{0.0, 15.f, 15.f});

	// initialize PsychHelper based on the configuration.
	initPsychHelper();

	// create the result file based on experimental configuration.
	createResultFile();
}

void TargetExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
	// seems like nothing necessary?
}

float TargetExperiment::randSign() {
	if (Random::common().uniform() > 0.5) {
		return 1;
	}
	else {
		return -1;
	}
}

void TargetExperiment::initTargetAnimation() {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_app->m_spawnDistance, 0.0f, 0.0f);
	m_app->m_motionFrame = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);
	m_app->m_motionFrame.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (m_app->m_presentationState == PresentationState::task) {
		m_speed = G3D::Random::common().uniform(m_psych.getParam().val["minSpeed"], m_psych.getParam().val["maxSpeed"]);
		float rot_pitch = randSign() * Random::common().uniform(m_psych.getParam().val["minEccV"], m_psych.getParam().val["maxEccV"]);
		float rot_yaw = randSign() * Random::common().uniform(m_psych.getParam().val["minEccH"], m_psych.getParam().val["maxEccH"]);
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::pitchDegrees(rot_pitch)).approxCoordinateFrame();
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::yawDegrees(rot_yaw)).approxCoordinateFrame();

		// Apply roll rotation by a random amount (random angle in degree from 0 to 360)
		float randomAngleDegree = G3D::Random::common().uniform() * 360;
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
	}
	else {
		m_speed = 0;
	}

	// Full health for the target
	m_app->m_targetHealth = 1.f;

	// Don't reset the view. Wait for the subject to center on the ready target.
	//m_app->resetView();
}


void TargetExperiment::processResponse()
{
	m_taskExecutionTime = m_app->timer.getTime();
	m_response = (m_app->m_targetHealth <= 0) ? 1 : 0; // 1 means success, 0 means failure.
	recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
	m_psych.processResponse(m_response); // process response.
	/*if (m_app->m_expConfig.expMode == "training") {
		m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
	}*/
}

void TargetExperiment::updatePresentationState()
{
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState currentState = m_app->m_presentationState;
	PresentationState newState;
	float stateElapsedTime = m_app->timer.getTime();

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
		if (stateElapsedTime > m_config.val["readyDuration"])
		{
			m_lastMotionChangeAt = 0;
			m_app->m_targetColor = Color3::green().pow(2.0f);
			newState = PresentationState::task;
		}
	}
	else if (currentState == PresentationState::task)
	{
		if ((stateElapsedTime > m_config.val["taskDuration"]) || (m_app->m_targetHealth <= 0))
		{
			m_taskEndTime = System::time();
			processResponse();
			m_app->m_targetColor = Color3::red().pow(2.0f);
			newState = PresentationState::feedback;
		}
	}
	else if (currentState == PresentationState::feedback)
	{
		if ((stateElapsedTime > m_config.val["feedbackDuration"]) && (m_app->m_targetHealth <= 0))
		{
			if (m_psych.isComplete()) {
				m_feedbackMessage = "Experiment complete. Thanks!";
				newState = PresentationState::complete;
			}
			else {
				m_feedbackMessage = "";
				m_psych.chooseNextCondition();
				newState = PresentationState::ready;
			}
		}
	}
	else {
		newState = currentState;
	}

	if (currentState != newState)
	{ // handle state transition.
		m_app->timer.startTimer();
		if (newState == PresentationState::task) {
			m_taskStartTime = System::time();
		}
		m_app->m_presentationState = newState;
		//If we switched to task, call initTargetAnimation to handle new trial
		if ((newState == PresentationState::task) || (newState == PresentationState::feedback)) {
			initTargetAnimation();
		}
	}
}

void TargetExperiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
{
	// 1. Update presentation state and send task performance to psychophysics library.
	updatePresentationState();

	// 2. Check if motionChange is required (happens only during 'task' state with a designated level of chance).
	if (m_app->m_presentationState == PresentationState::task)
	{
		if (m_app->timer.getTime() > m_lastMotionChangeAt + m_psych.getParam().val["motionChangePeriod"])
		{
			// If yes, rotate target coordinate frame by random (0~360) angle in roll direction
			m_lastMotionChangeAt = m_app->timer.getTime();
			float randomAngleDegree = G3D::Random::common().uniform() * 360;
			m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
		}
	}

	// 3. update target location (happens only during 'task' and 'feedback' states).
	if (m_app->m_presentationState == PresentationState::task)
	{
		float rotationAngleDegree = (float)rdt * m_speed;

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

	// 4. Clear m_TargetArray. Append an object with m_targetLocation if necessary ('task' and 'feedback' states).
	Point3 t_pos = m_app->m_motionFrame.pointToWorldSpace(Point3(0, 0, -m_app->m_targetDistance));


	if (m_app->m_targetHealth > 0.f) {
		// Don't spawn a new target every frame
		if (m_app->m_targetArray.size() == 0) {
			m_app->spawnTarget(t_pos, m_psych.getParam().val["visualSize"], false, m_app->m_targetColor);
		}
		else {
			// TODO: don't hardcode assumption of a single target
			m_app->m_targetArray[0]->setFrame(t_pos);
		}
	}

	// record target trajectories, view direction trajectories, and mouse motion.
	if (m_app->m_presentationState == PresentationState::task)
	{
		accumulateTrajectories();
	}
}

void TargetExperiment::onUserInput(UserInput* ui)
{
	// insert response and uncomment below. 
	if (ui->keyPressed(GKey::LEFT_MOUSE)) {
		m_app->fire();
		if (m_app->m_targetHealth == 0) {
			// target eliminated, must be 'hit'.
			if (m_app->m_presentationState == PresentationState::task)
			{
				accumulatePlayerAction(PlayerAction::HIT);
			}
		}
		else {
			// target still present, must be 'miss'.
			if (m_app->m_presentationState == PresentationState::task)
			{
				accumulatePlayerAction(PlayerAction::MISS);
			}
		}
	}
}

void TargetExperiment::onGraphics2D(RenderDevice* rd)
{
	const float scale = rd->viewport().width() / 1920.0f;
	rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

	// Reticle
	Draw::rect2D((m_app->m_reticleTexture->rect2DBounds() * scale - m_app->m_reticleTexture->vector2Bounds() * scale / 2.0f) / 8.0f + rd->viewport().wh() / 2.0f, rd, Color3::green(), m_app->m_reticleTexture);

	// TODO: Feels like the following variables should be members of TargetExperiment:
	// m_renderHud, m_hudTexture, m_reticleTexture, ...
	if (m_app->m_renderHud && !m_app->m_emergencyTurbo) {
		const Point2 hudCenter(rd->viewport().width() / 2.0f, m_app->m_hudTexture->height() * scale * 0.48f);
		Draw::rect2D((m_app->m_hudTexture->rect2DBounds() * scale - m_app->m_hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), m_app->m_hudTexture);
		m_app->m_hudFont->draw2D(rd, "1:36", hudCenter - Vector2(80, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		m_app->m_hudFont->draw2D(rd, "86%", hudCenter + Vector2(7, -1), scale * 30, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		m_app->m_hudFont->draw2D(rd, "2080", hudCenter + Vector2(125, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}

	if (!m_feedbackMessage.empty()) {
		m_app->m_outputFont->draw2D(rd, m_feedbackMessage.c_str(),
			(Point2((float)m_app->window()->width() / 2 - 40, (float)m_app->window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
	}
}

void TargetExperiment::createResultFile()
{
	// generate folder result_data if it does not exist.
	if (!FileSystem::isDirectory(String("result_data"))) {
		FileSystem::createDirectory(String("result_data"));
	}

	// create a unique file name
	String timeStr(genUniqueTimestamp());
	/*if (m_app->m_expConfig.expMode == "training") {
		mResultFileName = ("result_data/" + m_app->m_expConfig.expMode + "_" + m_app->m_expConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".db").c_str();
	}
	else {
		mResultFileName = ("result_data/" + m_app->m_expConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".db").c_str();
	}*/
	mResultFileName = ("result_data/" + m_app->m_experimentConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".db").c_str();

	// create the file
	if (sqlite3_open(mResultFileName.c_str(), &m_db)) {
		// TODO: report error if failed.
	}

	// create tables inside the db file.
	// 1. Experiment description (time and subject ID)
	// create sqlite table
	std::vector<std::vector<std::string>> expColumns = {
		// format: column name, data type, sqlite modifier(s)
			{ "time", "text", "NOT NULL" },
			{ "subjectID", "text", "NOT NULL" },
	};
	createTableInDB(m_db, "Experiments", expColumns); // no need of Primary Key for this table.

	// populate table
	std::vector<std::string> expValues = {
		addQuotes(timeStr.c_str()),
		addQuotes(m_app->m_user.subjectID.c_str())
	};
	insertRowIntoDB(m_db, "Experiments", expValues);

	// 2. Conditions
	// create sqlite table
	std::vector<std::vector<std::string>> conditionColumns = {
			{ "id", "integer", "PRIMARY KEY"}, // this makes id a key value, requiring it to be unique for each row.
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "min_ecc_h", "real" },
			{ "min_ecc_V", "real" },
			{ "max_ecc_h", "real" },
			{ "max_ecc_V", "real" },
			{ "min_speed", "real" },
			{ "max_speed", "real" },
			{ "motion_change_period", "real" },
	};
	createTableInDB(m_db, "Conditions", conditionColumns); // Primary Key needed for this table.

	// populate table
	for (int i = 0; i < m_psych.mMeasurements.size(); ++i)
	{
		std::vector<std::string> conditionValues = {
			std::to_string(i), // this index is uniquely and statically assigned to each SingleThresholdMeasurement.
			std::to_string(m_psych.mMeasurements[i].getParam().val["targetFrameRate"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["targetFrameLag"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["minEccH"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["minEccV"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["maxEccH"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["maxEccV"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["minSpeed"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["maxSpeed"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["motionChangePeriod"]),
		};
		insertRowIntoDB(m_db, "Conditions", conditionValues);
	}

	// 3. Trials, only need to create the table.
	std::vector<std::vector<std::string>> trialColumns = {
			{ "condition_ID", "integer" },
			{ "start_time", "real" },
			{ "end_time", "real" },
			{ "task_execution_time", "real" },
	};
	createTableInDB(m_db, "Trials", trialColumns);

	// 4. Target_Trajectory, only need to create the table.
	std::vector<std::vector<std::string>> targetTrajectoryColumns = {
			{ "time", "real" },
			{ "position_x", "real" },
			{ "position_y", "real" },
			{ "position_z", "real" },
	};
	createTableInDB(m_db, "Target_Trajectory", targetTrajectoryColumns);

	// 5. Player_Action, only need to create the table.
	std::vector<std::vector<std::string>> viewTrajectoryColumns = {
			{ "time", "real" },
			{ "event", "text" },
			{ "position_az", "real" },
			{ "position_el", "real" },
	};
	createTableInDB(m_db, "Player_Action", viewTrajectoryColumns);
}

void TargetExperiment::recordTrialResponse()
{
	// Trials table. Record trial start time, end time, and task completion time.
	std::vector<std::string> trialValues = {
		std::to_string(m_psych.mCurrentConditionIndex),
		std::to_string(m_taskStartTime),
		std::to_string(m_taskEndTime),
		std::to_string(m_taskExecutionTime),
	};
	insertRowIntoDB(m_db, "Trials", trialValues);

	// Target_Trajectory table. Write down the recorded target trajectories.
	insertRowsIntoDB(m_db, "Target_Trajectory", m_targetTrajectory);
	m_targetTrajectory.clear();

	// Player_Action table. Write down the recorded player actions.
	insertRowsIntoDB(m_db, "Player_Action", m_playerActions);
	m_playerActions.clear();
}

void TargetExperiment::accumulateTrajectories()
{
	// recording target trajectories
	Point3 targetPosition = m_app->getTargetPosition();
	std::vector<std::string> targetTrajectoryValues = {
		std::to_string(System::time()),
		std::to_string(targetPosition.x),
		std::to_string(targetPosition.y),
		std::to_string(targetPosition.z),
	};
	m_targetTrajectory.push_back(targetTrajectoryValues);

	// recording view direction trajectories
	accumulatePlayerAction(PlayerAction::AIM);
}

void TargetExperiment::accumulatePlayerAction(PlayerAction hm)
{
	// specify action type
	String s;
	if (hm == PlayerAction::HIT) {
		s = "hit";
	}
	else if (hm == PlayerAction::MISS) {
		s = "miss";
	}
	else if (hm == PlayerAction::AIM) {
		s = "aim";
	}

	// recording target trajectories
	Point2 dir = m_app->getViewDirection();
	std::vector<std::string> playerActionValues = {
		std::to_string(System::time()),
		addQuotes(s.c_str()),
		std::to_string(dir.x),
		std::to_string(dir.y),
	};
	m_playerActions.push_back(playerActionValues);
}

void TargetExperiment::closeResultFile()
{
	sqlite3_close(m_db);
}
