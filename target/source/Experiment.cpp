/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "Experiment.h"
#include "App.h"

void PsychHelper::addCondition(Array<Param> newConditionParams, PsychophysicsDesignParameter newPsychParam)
{
	SingleThresholdMeasurement m(newConditionParams, newPsychParam);
	mMeasurements.push_back(m);
}

void PsychHelper::chooseNextCondition()
{
	// Choose any staircase whose progress ratio is minimum
	float minimumProgressRatio = 1;
	// Find minimum progress ratio
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (mMeasurements[i].getProgressRatio() < minimumProgressRatio)
		{
			minimumProgressRatio = mMeasurements[i].getProgressRatio();
		}
	}
	// Make a vector with all the measurement cells with minimum progress ratio
	std::vector<int32_t> validIndex;
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (mMeasurements[i].getProgressRatio() == minimumProgressRatio)
		{
			validIndex.push_back(i);
		}
	}
	// Now choose any one from validIndex
	mCurrentConditionIndex = validIndex[rand() % (int32_t)validIndex.size()];
	std::cout << "Next chosen staircase is: " << mCurrentConditionIndex << '\n';
}

Array<Param> PsychHelper::getParams()
{
	return mMeasurements[mCurrentConditionIndex].TargetParameters;
}

float PsychHelper::getStimLevel()
{
	return mMeasurements[mCurrentConditionIndex].getCurrentLevel();
}

void PsychHelper::processResponse(int32_t response)
{
	// Process the response.
	mMeasurements[mCurrentConditionIndex].processResponse(response);
	mTrialCount++;
}

bool PsychHelper::isComplete() // did the experiment end?
{
	bool allMeasurementComplete = true;
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (!mMeasurements[i].isComplete()) // if any one staircase is incomplete, set allSCCompleted to false and break.
		{
			allMeasurementComplete = false;
			break;
		}
	}
	return allMeasurementComplete;
}

bool Experiment::initPsychHelper()
{
	// Add conditions, one per one initial displacement value.
	// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
	// Iterate over the sessions here and add a config for each
	m_session = m_config.getSessionConfigById(m_app->getDropDownSessId());
	if (m_session == nullptr) return false;
	Array<Array<Param>> params = m_config.getExpConditions(m_session->id);
	for (Array<Param> targets : params) {
		// Define properties of psychophysical methods
		PsychophysicsDesignParameter psychParam;
		psychParam.mMeasuringMethod = PsychophysicsMethod::MethodOfConstantStimuli;
		// Can we remove this?
		psychParam.mIsDefault = false;
		// We need something in mStimLevels to run psychphysics...
		psychParam.mStimLevels.push_back(m_config.taskDuration);						// Shorter task is more difficult. However, we are currently doing unlimited time.
		psychParam.mMaxTrialCounts.push_back((int)targets[0].val["trialCount"]);		// Get the trial count from the parameters
		for (int i = 0; i < targets.size(); i++) {										// Add the session to each target
			const char* sess = m_session->id.c_str();
			targets[i].add("name", format("%s_%d", sess, i).c_str());
			targets[i].add("session", sess);
		}
		m_psych.addCondition(targets, psychParam);
	}

	// Update the logger w/ these conditions (IS THIS THE RIGHT PLACE TO DO THIS???)
	m_app->logger->addTargets(m_psych.mMeasurements);

	// call it once all conditions are defined.
	m_psych.chooseNextCondition();
	return true;
}

void Experiment::onInit() {
	// Initialize presentation states
	presentationState = PresentationState::initial;
	m_feedbackMessage = "Aim at the target and shoot!";

	m_config = m_app->experimentConfig;									// Setup config from app
	m_hasSession = initPsychHelper();
	if (!m_hasSession) {												// Initialize PsychHelper based on the configuration.
		presentationState = PresentationState::feedback;
	}
}

void Experiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
	// seems like nothing necessary?
}

float Experiment::randSign() {
	if (Random::common().uniform() > 0.5) {
		return 1;
	}
	else {
		return -1;
	}
}

void Experiment::initTargetAnimation() {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	float visualSize = G3D::Random().common().uniform(m_psych.getParams()[0].val["minVisualSize"], m_psych.getParams()[0].val["maxVisualSize"]);

	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_userSpawnDistance, 0.0f, 0.0f);
	CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);
	f.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (presentationState == PresentationState::task) {
		for (Param target : m_psych.getParams()) {
			float rot_pitch = randSign() * Random::common().uniform(target.val["minEccV"], target.val["maxEccV"]);
			float rot_yaw = randSign() * Random::common().uniform(target.val["minEccH"], target.val["maxEccH"]);
			float visualSize = G3D::Random().common().uniform(target.val["minVisualSize"], target.val["maxVisualSize"]);

			f = (f.toMatrix4() * Matrix4::pitchDegrees(rot_pitch)).approxCoordinateFrame();
			f = (f.toMatrix4() * Matrix4::yawDegrees(rot_yaw)).approxCoordinateFrame();

			if (String(target.str["jumpEnabled"].c_str()) == "true") {
				m_app->spawnJumpingTarget(
					f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
					visualSize,
					m_config.targetHealthColors[0],
					{ target.val["minSpeed"], target.val["maxSpeed"] },
					{ target.val["minMotionChangePeriod"], target.val["maxMotionChangePeriod"] },
					{ target.val["minJumpPeriod"], target.val["maxJumpPeriod"] },
					{ target.val["minDistance"], target.val["maxDistance"] },
					{ target.val["minJumpSpeed"], target.val["maxJumpSpeed"] },
					{ target.val["minGravity"], target.val["maxGravity"] },
					initialSpawnPos,
					m_targetDistance,
					String(target.str["id"]),
					String(target.str["name"])
				);
			}
			else {
				m_app->spawnFlyingTarget(
					f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
					visualSize,
					m_config.targetHealthColors[0],
					{ target.val["minSpeed"], target.val["maxSpeed"] },
					{ target.val["minMotionChangePeriod"], target.val["maxMotionChangePeriod"] },
					initialSpawnPos,
					String(target.str["id"]),
					String(target.str["name"])
				);
			}
		}
	}
	else {
		// Make sure we reset the target color here (avoid color bugs)
		m_app->spawnFlyingTarget(
			f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
			visualSize,
			m_config.dummyTargetColor.pow(2.0f),
			{ 0.0f, 0.0f },
			{ 1000.0f, 1000.f },
			initialSpawnPos,
			"dummy"
		);
	}

	// Reset number of destroyed targets
	m_app->destroyedTargets = 0;
	// reset click counter
	m_clickCount = 0;
}

void Experiment::processResponse()
{
	m_taskExecutionTime = m_timer.getTime();
	m_response = m_app->destroyedTargets; // Number of destroyed targets
	recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
	m_psych.processResponse(m_response); // process response.
	String sess = String(m_psych.mMeasurements[m_psych.mCurrentConditionIndex].TargetParameters[0].str["session"]);
	if (m_response == m_psych.mMeasurements[m_psych.mCurrentConditionIndex].TargetParameters.size()) {
		m_totalRemainingTime += (double(m_config.taskDuration) - m_taskExecutionTime);
		if (m_config.getSessionConfigById(sess)->expMode == "training") {
			m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
		}
	}
	else {
		if (m_config.getSessionConfigById(sess)->expMode == "training") {
			m_feedbackMessage = "Failure!";
		}
	}
}

void Experiment::updatePresentationState()
{
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState currentState = presentationState;
	PresentationState newState;
	int remainingTargets = m_app->targetArray.size();
	float stateElapsedTime = m_timer.getTime();

	newState = currentState;

	if (currentState == PresentationState::initial)
	{
		if (!m_app->m_buttonUp)
		{
			//m_feedbackMessage = "";
			newState = PresentationState::feedback;
		}
	}
	else if (currentState == PresentationState::ready)
	{
		if (stateElapsedTime > m_config.readyDuration)
		{
			//m_lastMotionChangeAt = 0;
			newState = PresentationState::task;
		}
	}
	else if (currentState == PresentationState::task)
	{
		if ((stateElapsedTime > m_config.taskDuration) || (remainingTargets <= 0) || (m_clickCount == m_config.weapon.maxAmmo))
		{
			m_taskEndTime = Logger::genUniqueTimestamp();
			processResponse();
			m_app->clearTargets(); // clear all remaining targets
			newState = PresentationState::feedback;
		}
	}
	else if (currentState == PresentationState::feedback)
	{
		if ((stateElapsedTime > m_config.feedbackDuration) && (remainingTargets <= 0))
		{
			if (m_psych.isComplete()) {
				m_app->mergeCurrentLogToCurrentDB();
				m_app->markSessComplete(String(m_psych.getParams()[0].str["session"]));			// Add this session to user's completed sessions
				m_app->updateSessionDropDown();

				int score = int(m_totalRemainingTime);
				m_feedbackMessage = format("Session complete! You scored %d!", score); // Update the feedback message
				newState = PresentationState::scoreboard;
			}
			else {
				m_feedbackMessage = "";
				m_psych.chooseNextCondition();
				newState = PresentationState::ready;
			}
		}
	}
	else if (currentState == PresentationState::scoreboard) {
		if (stateElapsedTime > m_scoreboardDuration) {
			newState = PresentationState::complete;
			m_app->openUserSettingsWindow();
			if (m_hasSession) {
				m_app->userSaveButtonPress();												// Press the save button for the user...
				Array<String> remaining = m_app->updateSessionDropDown();
				if (remaining.size() == 0) {
					m_feedbackMessage = "All Sessions Complete!"; // Update the feedback message
				}
				else {
					m_feedbackMessage = "Session Complete!"; // Update the feedback message
					moveOn = true;														// Check for session complete (signal start of next session)
				}
			}
			else
				m_feedbackMessage = "All Sessions Complete!";							// Update the feedback message
		}
	}

	else {
		newState = currentState;
	}

	if (currentState != newState)
	{ // handle state transition.
		m_timer.startTimer();
		if (newState == PresentationState::task) {
			m_taskStartTime = Logger::genUniqueTimestamp();
		}
		presentationState = newState;
		//If we switched to task, call initTargetAnimation to handle new trial
		if ((newState == PresentationState::task) || (newState == PresentationState::feedback)) {
			initTargetAnimation();
		}
	}
}

void Experiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
{
	// 1. Update presentation state and send task performance to psychophysics library.
	updatePresentationState();

	// 2. Record target trajectories, view direction trajectories, and mouse motion.
	if (presentationState == PresentationState::task)
	{
		accumulateTrajectories();
		accumulateFrameInfo(rdt, sdt, idt);
	}
}

void Experiment::onUserInput(UserInput* ui)
{
	// nothing here, handled in App::onUserInput
}

void Experiment::onGraphics2D(RenderDevice* rd)
{
	const float scale = rd->viewport().width() / 1920.0f;
	rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

	// Reticle
	Draw::rect2D(
		(m_app->reticleTexture->rect2DBounds() * scale - m_app->reticleTexture->vector2Bounds() * scale / 2.0f) / 2.0f + rd->viewport().wh() / 2.0f,
		rd, Color3::green(), m_app->reticleTexture
	);

	// TODO: Feels like the following variables should be members of Experiment (or should this be moved into the App?)
	// m_renderHud, m_hudTexture, m_reticleTexture, ...
	if (m_config.showHUD && m_config.showBanner && !m_app->emergencyTurbo) {
		const Point2 hudCenter(rd->viewport().width() / 2.0f, m_app->hudTexture->height() * scale * 0.48f);
		Draw::rect2D((m_app->hudTexture->rect2DBounds() * scale - m_app->hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), m_app->hudTexture);
		
		// Create strings for time remaining, progress in sessions, and score
		float remainingTime = m_config.taskDuration - m_timer.getTime();
		float printTime = remainingTime > 0 ? remainingTime : 0.0f;
		String time_string = format("%0.2f", printTime);
		String prog_string = "";
		if (m_session != nullptr){
			prog_string = format("%d", (int)(100.0f*(float)m_psych.mTrialCount / (float)m_session->getTotalTrials())) + "%";
		}
		String score_string = format("%d", (int)(10 * m_totalRemainingTime));
		
		m_app->hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		m_app->hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale * 30, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		m_app->hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}

	if (!m_feedbackMessage.empty()) {
		m_app->outputFont->draw2D(rd, m_feedbackMessage.c_str(),
			(Point2((float)m_app->window()->width() / 2 - 40, (float)m_app->window()->height() / 2 + 40) * scale).floor(), floor(20.0f * scale), Color3::yellow());
	}
}

void Experiment::recordTrialResponse()
{
	String sess = String(m_psych.mMeasurements[m_psych.mCurrentConditionIndex].TargetParameters[0].str["session"]);

	// Trials table. Record trial start time, end time, and task completion time.
	Array<String> trialValues = {
		String(std::to_string(m_psych.mCurrentConditionIndex)),
		"'" + sess + "'",
		"'" + m_config.getSessionConfigById(sess)->expMode + "'",
		"'" + m_taskStartTime + "'",
		"'" + m_taskEndTime + "'",
		String(std::to_string(m_taskExecutionTime)),
		String(std::to_string(m_response))
	};
	m_app->logger->recordTrialResponse(trialValues);

	// Target_Trajectory table. Write down the recorded target trajectories.
	m_app->logger->recordTargetTrajectory(m_targetTrajectory);
	m_targetTrajectory.clear();

	// Player_Action table. Write down the recorded player actions.
	m_app->logger->recordPlayerActions(m_playerActions);
	m_playerActions.clear();

	// Frame_Info table. Write down all frame info.
	m_app->logger->recordFrameInfo(m_frameInfo);
	m_frameInfo.clear();
}

void Experiment::accumulateTrajectories()
{
	for (shared_ptr<TargetEntity> target : m_app->targetArray) {
		// recording target trajectories
		Point3 targetAbsolutePosition = target->frame().translation;
		Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_userSpawnDistance, 0.0f, 0.0f);
		Point3 targetPosition = targetAbsolutePosition - initialSpawnPos;

		//// below for 2D direction calculation (azimuth and elevation)
		//Point3 t = targetPosition.direction();
		//float az = atan2(-t.z, -t.x) * 180 / pif();
		//float el = atan2(t.y, sqrtf(t.x * t.x + t.z * t.z)) * 180 / pif();

		Array<String> targetTrajectoryValues = {
			"'" + Logger::genUniqueTimestamp() + "'",
			"'" + target->name() + "'",
			String(std::to_string(targetPosition.x)),
			String(std::to_string(targetPosition.y)),
			String(std::to_string(targetPosition.z)),
		};
		m_targetTrajectory.push_back(targetTrajectoryValues);
	}
	// recording view direction trajectories
	accumulatePlayerAction("aim");
}

void Experiment::accumulatePlayerAction(String action, String targetName)
{
	BEGIN_PROFILER_EVENT("accumulatePlayerAction");
	// recording target trajectories
	Point2 dir = m_app->getViewDirection();
	Array<String> playerActionValues = {
		"'" + Logger::genUniqueTimestamp() + "'",
		"'" + action + "'",
		String(std::to_string(dir.x)),
		String(std::to_string(dir.y)),
		"'" + targetName + "'",
	};
	m_playerActions.push_back(playerActionValues);
	END_PROFILER_EVENT();
}

void Experiment::accumulateFrameInfo(RealTime t, float sdt, float idt) {
	Array<String> frameValues = {
		"'" + Logger::genUniqueTimestamp() + "'",
		String(std::to_string(idt)),
		String(std::to_string(sdt))
	};
	m_frameInfo.push_back(frameValues);
}

bool Experiment::responseReady() {
	double timeNow = System::time();
	if ((timeNow - m_lastFireAt) > (m_config.weapon.firePeriod)) {
		m_lastFireAt = timeNow;
		return true;
	}
	else {
		return false;
	}
}

double Experiment::weaponCooldownPercent() {
	if (m_config.weapon.firePeriod == 0.0f) return 1.0;
	return min((System::time() - m_lastFireAt) / m_config.weapon.firePeriod, 1.0);
}
