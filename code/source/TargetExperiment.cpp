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
	psychParam.mStimLevels.push_back(m_app->m_experimentConfig.taskDuration); // Shorter task is more difficult. However, we are currently doing unlimited time.
	psychParam.mMaxTrialCounts.push_back(m_app->m_experimentConfig.trialCount);

	// Initialize PsychHelper.
	m_psych.clear(); // TODO: check whether necessary.

	// Add conditions, one per one initial displacement value.
	// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
	for (auto d : m_app->m_experimentConfig.initialDisplacements)
	{
		// insert all the values potentially useful for analysis.
		Param p;
		p.add("initialDisplacementYaw", d.y);
		p.add("initialDisplacementRoll", d.x);
		p.add("targetFrameRate", m_app->m_experimentConfig.targetFrameRate);
		p.add("targetFrameLag", (float)m_app->m_experimentConfig.targetFrameLag);
		p.add("visualSize", m_app->m_experimentConfig.visualSize);
		p.add("motionChangePeriod", m_app->m_experimentConfig.motionChangePeriod);
		p.add("speed", m_app->m_experimentConfig.speed);
		// soon we need to add frame delay as well.
		m_psych.addCondition(p, psychParam);
	}
}

void TargetExperiment::onInit() {
	// load the experiment background scene.
	m_app->loadScene(m_app->m_experimentConfig.sceneName);

	// initialize presentation states
	m_app->m_presentationState = PresentationState::initial;
	m_feedbackMessage = "Aim at the target and shoot!";

	// default values
	// TODO: This should all move into configuration file.
	m_app->m_experimentConfig.feedbackDuration = 1.0;
	m_app->m_experimentConfig.motionChangePeriod = 100000.0;
	m_app->m_experimentConfig.readyDuration = 0.5;
	m_app->m_experimentConfig.taskDuration = 100000.0;
	m_app->m_experimentConfig.speed = 0.0;
	m_app->m_experimentConfig.trialCount = 10;
	m_app->m_experimentConfig.visualSize = 0.02f;
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, -15.0f));
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, -10.0f));
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, -5.0f));
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, 5.0f));
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, 10.0f));
	m_app->m_experimentConfig.initialDisplacements.append(Point2(0.0f, 15.0f));

	// apply changes depending on experiment version
	if (m_app->m_experimentConfig.expVersion == "SimpleMotion")
	{
		m_app->m_experimentConfig.speed = 6.0f;
	}
	else if (m_app->m_experimentConfig.expVersion == "ComplexMotion")
	{
		m_app->m_experimentConfig.speed = 6.0f;
		m_app->m_experimentConfig.motionChangePeriod = 0.4f;
	}

	if (m_app->m_experimentConfig.expMode == "training") { // shorter experiment if in training
		// NOTE: maybe not a good idea with dedicated subject.
		m_app->m_experimentConfig.trialCount = m_app->m_experimentConfig.trialCount;
	}

	// initialize PsychHelper based on the configuration.
	initPsychHelper();

	// create the result file based on experimental configuration.
	createResultFile();
}

void TargetExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
	// seems like nothing necessary?
}

void TargetExperiment::initTargetAnimation() {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_app->m_spawnDistance, 0.0f, 0.0f);
	m_app->m_motionFrame = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);
	m_app->m_motionFrame.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (m_app->m_presentationState == PresentationState::task) {
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(m_psych.getParam().val["initialDisplacementRoll"])).approxCoordinateFrame();
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::yawDegrees(m_psych.getParam().val["initialDisplacementYaw"])).approxCoordinateFrame();

		// Apply roll rotation by a random amount (random angle in degree from 0 to 360)
		float randomAngleDegree = G3D::Random::common().uniform() * 360;
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
	}

	// Full health for the target
	m_app->m_targetHealth = 1.f;

	// Don't reset the view. Wait for the subject to center on the ready target.
	//m_app->resetView();
}


void TargetExperiment::createNewTarget() {
	// determine starting position
	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_app->m_spawnDistance, 0.0f, 0.0f);
	m_app->m_motionFrame = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);
	m_app->m_motionFrame.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction
	Array<Point3> destinationArray;

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (m_app->m_presentationState == PresentationState::task) {
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(m_psych.getParam().val["initialDisplacementRoll"])).approxCoordinateFrame();
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::yawDegrees(m_psych.getParam().val["initialDisplacementYaw"])).approxCoordinateFrame();

		// Apply roll rotation by a random amount (random angle in degree from 0 to 360)
		float randomAngleDegree = G3D::Random::common().uniform() * 360;
		m_app->m_motionFrame = (m_app->m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();

		// create list of motion positions TODO
	}

	// spawn target
	const shared_ptr<TargetEntity>& target = m_app->spawnTarget(initialSpawnPos, 0.2f, false, Color3::purple());
	const Point3 center = m_app->activeCamera()->frame().translation;
	target->setDestinations(destinationArray, center);
}

void TargetExperiment::processResponse()
{
	m_taskExecutionTime = m_app->timer.getTime();
	m_response = (m_app->m_targetHealth <= 0) ? 1 : 0; // 1 means success, 0 means failure.
	recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
	m_psych.processResponse(m_response); // process response.
	if (m_app->m_experimentConfig.expMode == "training") {
		m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
	}
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
		if (stateElapsedTime > m_app->m_experimentConfig.readyDuration)
		{
			m_lastMotionChangeAt = 0;
			m_app->m_targetColor = Color3::green().pow(2.0f);
			newState = PresentationState::task;
		}
	}
	else if (currentState == PresentationState::task)
	{
		if ((stateElapsedTime > m_app->m_experimentConfig.taskDuration) || (m_app->m_targetHealth <= 0))
		{
			m_taskEndTime = System::time();
			processResponse();
			m_app->m_targetColor = Color3::red().pow(2.0f);
			newState = PresentationState::feedback;
		}
	}
	else if (currentState == PresentationState::feedback)
	{
		if ((stateElapsedTime > m_app->m_experimentConfig.feedbackDuration) && (m_app->m_targetHealth <= 0))
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
			// TODO spawn new target here
			//createNewTarget();
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
		if (m_app->timer.getTime() > m_lastMotionChangeAt + m_app->m_experimentConfig.motionChangePeriod)
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
		float rotationAngleDegree = (float)rdt * m_app->m_experimentConfig.speed;

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
			m_app->spawnTarget(t_pos, m_app->m_experimentConfig.visualSize, false, m_app->m_targetColor);
		}
		else {
			// TODO: don't hardcode assumption of a single target
			m_app->m_targetArray[0]->setFrame(t_pos);
		}
	}

	// record target trajectories, view direction trajectories, and mouse motion.
	if (m_app->m_presentationState == PresentationState::task)
	{
		recordTrajectories();
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
				recordPlayerAction(PlayerAction::HIT);
			}
		}
		else {
			// target still present, must be 'miss'.
			if (m_app->m_presentationState == PresentationState::task)
			{
				recordPlayerAction(PlayerAction::MISS);
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
	if (m_app->m_experimentConfig.expMode == "training") {
		mResultFileName = ("result_data/" + m_app->m_experimentConfig.expMode + "_" + m_app->m_experimentConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".db").c_str();
	}
	else {
		mResultFileName = ("result_data/" + m_app->m_experimentConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".db").c_str();
	}

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
	insertIntoDB(m_db, "Experiments", expValues);

	// 2. Conditions
	// create sqlite table
	std::vector<std::vector<std::string>> conditionColumns = {
			{ "id", "integer", "PRIMARY KEY"}, // this makes id a key value, requiring it to be unique for each row.
			{ "refresh_rate", "real" },
			{ "added_frame_lag", "real" },
			{ "displacement_r", "real" },
			{ "displacement_theta", "real" },
			{ "speed", "real" },
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
			std::to_string(m_psych.mMeasurements[i].getParam().val["initialDisplacementYaw"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["initialDisplacementRoll"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["speed"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["motionChangePeriod"]),
		};
		insertIntoDB(m_db, "Conditions", conditionValues);
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
			{ "position_az", "real" },
			{ "position_el", "real" },
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
	std::vector<std::string> trialValues = {
		std::to_string(m_psych.mCurrentConditionIndex),
		std::to_string(m_taskStartTime),
		std::to_string(m_taskEndTime),
		std::to_string(m_taskExecutionTime),
	};
	insertIntoDB(m_db, "Trials", trialValues);
}

void TargetExperiment::recordTrajectories()
{
	// recording target trajectories
	Point2 dir = m_app->getTargetDirection();
	std::vector<std::string> targetTrajectoryValues = {
		std::to_string(System::time()),
		std::to_string(dir.x),
		std::to_string(dir.y),
	};
	insertIntoDB(m_db, "Target_Trajectory", targetTrajectoryValues);

	// recording view direction trajectories
	recordPlayerAction(PlayerAction::AIM);
}

void TargetExperiment::recordPlayerAction(PlayerAction hm)
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

	// recording view direction trajectories
	insertIntoDB(m_db, "Player_Action", playerActionValues);

}

void TargetExperiment::closeResultFile()
{
	sqlite3_close(m_db);
}
