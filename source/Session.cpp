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
#include "Session.h"
#include "App.h"

void Session::addTrial(Array<Param> params) {
	m_remaining.append(params[0].val["trialCount"]);
	m_trialParams.append(params);
}

void Session::nextCondition() {
	Array<int> unrunTrialIdxs;
	for (int i = 0; i < m_remaining.size(); i++) {
		if (m_remaining[i] > 0) {
			unrunTrialIdxs.append(i);
		}
	}
	m_currTrialIdx = unrunTrialIdxs[rand() % unrunTrialIdxs.size()];
}

bool Session::isComplete() {
	bool allTrialsComplete = true;
	for (int remaining : m_remaining) {
		allTrialsComplete &= (remaining == 0);
	}
	return allTrialsComplete;
}

bool Session::setupTrialParams(Array<Array<Param>> params)
{
	for (Array<Param> targets : params) {
		for (int i = 0; i < targets.size(); i++) {										// Add the session to each target
			std::string sess = targets[i].str["sessionID"];
			targets[i].add("name", format("%s_%d_%s_%d", sess, (int)targets[i].val["trial_idx"], targets[i].str["id"], i).c_str());
		}
		addTrial(targets);
	}

	// Update the logger w/ these conditions (IS THIS THE RIGHT PLACE TO DO THIS???)
	m_logger->addTargets(m_trialParams);

	// Select the first condition
	nextCondition();
	return true;
}

void Session::onInit(String filename, String userName, String description) {
	// Initialize presentation states
	presentationState = PresentationState::initial;
	m_feedbackMessage = "Click to spawn a target, then use shift on red target to begin.";

	m_config = m_app->experimentConfig;									// Setup config from app
	m_session = m_config.getSessionConfigById(m_app->getDropDownSessId());

	// Setup the logger and create results file
	m_logger = Logger::create();
	m_logger->createResultsFile(filename, userName, description);

	m_hasSession = m_session != nullptr;
	if (m_hasSession) {
		// Iterate over the sessions here and add a config for each
		Array<Array<Param>> params = m_config.getExpConditions(m_session->id);
		setupTrialParams(params);
	}
	else {												// Initialize PsychHelper based on the configuration.
		presentationState = PresentationState::feedback;
	}
}

float Session::randSign() {
	if (Random::common().uniform() > 0.5) {
		return 1;
	}
	else {
		return -1;
	}
}

void Session::randomizePosition(shared_ptr<TargetEntity> target) {
	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_userSpawnDistance, 0.0f, 0.0f);

	Param tParam = m_trialParams[m_currTrialIdx][target->paramIdx()];
	bool isWorldSpace = tParam.str["destSpace"] == "world";
	Point3 loc;

	if (isWorldSpace) {
		loc = tParam.bounds.randomInteriorPoint();		// Set a random position in the bounds
		target->resetMotionParams();					// Reset the target motion behavior
	}
	else {
		float rot_pitch = randSign() * Random::common().uniform(tParam.val["minEccV"], tParam.val["maxEccV"]);
		float rot_yaw = randSign() * Random::common().uniform(tParam.val["minEccH"], tParam.val["maxEccH"]);
		CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, rot_yaw, rot_pitch, 0.0f);
		loc = f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
	}
	target->setFrame(loc);
}

void Session::initTargetAnimation() {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	static const Point3 initialSpawnPos = m_app->activeCamera()->frame().translation + Point3(-m_userSpawnDistance, 0.0f, 0.0f);
	CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, 0.0f, 0.0f, 0.0f);

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (presentationState == PresentationState::task) {
		for (int i = 0; i < m_trialParams[m_currTrialIdx].size(); i++) {
			Param target = m_trialParams[m_currTrialIdx][i];
			float rot_pitch = randSign() * Random::common().uniform(target.val["minEccV"], target.val["maxEccV"]);
			float rot_yaw = randSign() * Random::common().uniform(target.val["minEccH"], target.val["maxEccH"]);
			float visualSize = G3D::Random().common().uniform(target.val["minVisualSize"], target.val["maxVisualSize"]);
			bool isWorldSpace = target.str["destSpace"] == "world";

			f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, rot_yaw, rot_pitch, 0.0f);

			// Check for case w/ destination array
			if (target.val["destCount"] > 0.0) {
				Point3 offset =isWorldSpace ? Point3(0.0, 0.0, 0.0) : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				m_app->spawnDestTarget(
					offset,
					target.destinations,
					visualSize,
					m_config.targetHealthColors[0],
					String(target.str["id"]),
					i,
					(int)target.val["respawns"],
					String(target.str["name"])
				);
			}
			// Otherwise check if this is a jumping target
			else if (String(target.str["jumpEnabled"].c_str()) == "true") {
				Point3 offset = isWorldSpace ? target.bounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				shared_ptr<JumpingEntity> t = m_app->spawnJumpingTarget(
					offset,
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
					i,
					(int) target.val["respawns"],
					String(target.str["name"])
				);
				t->setWorldSpace(isWorldSpace);
				if (isWorldSpace) {
					t->setBounds(target.bounds);
				}
			}
			else {
				Point3 offset = isWorldSpace ? target.bounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				shared_ptr<FlyingEntity> t = m_app->spawnFlyingTarget(
					offset,
					visualSize,
					m_config.targetHealthColors[0],
					{ target.val["minSpeed"], target.val["maxSpeed"] },
					{ target.val["minMotionChangePeriod"], target.val["maxMotionChangePeriod"] },
					initialSpawnPos,
					String(target.str["id"]),
					i,
					(int)target.val["respawns"],
					String(target.str["name"])
				);
				t->setWorldSpace(isWorldSpace);
				if (isWorldSpace) {
					t->setBounds(target.bounds);
				}
			}
		}
	}
	else {
		// Make sure we reset the target color here (avoid color bugs)
		m_app->spawnFlyingTarget(
			f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
			m_config.dummyTargetSize,
			m_config.dummyTargetColor,
			{ 0.0f, 0.0f },
			{ 1000.0f, 1000.f },
			initialSpawnPos,
			"dummy",
			0
		);

		// TODO: Remove this testing new target type
		//m_app->spawnTestTarget(initialSpawnPos);
	}

	// Reset number of destroyed targets
	m_app->destroyedTargets = 0;
	// reset click counter
	m_clickCount = 0;
}

void Session::processResponse()
{
	m_taskExecutionTime = m_timer.getTime();
	// Get total target count here
	int totalTargets = 0;
	for (Param t : m_trialParams[m_currTrialIdx]) {
		if (t.val["respawns"] == -1) {
			totalTargets = MAXINT;		// Ininite spawn case
			break;
		}
		else {
			totalTargets += (int)t.val["respawns"];
		}
	}		
	m_response = totalTargets - m_app->destroyedTargets; // Number of targets remaining
	recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
	
	m_remaining[m_currTrialIdx] -= 1;

	String sess = String(m_trialParams[m_currTrialIdx][0].str["sessionID"]);

	// Check for whether all targets have been destroyed
	if (m_response == 0) {
		m_totalRemainingTime += (double(m_config.taskDuration) - m_taskExecutionTime);
		if (m_config.getSessionConfigById(sess)->sessDescription == "training") {
			m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
		}
	}
	else {
		if (m_config.getSessionConfigById(sess)->sessDescription == "training") {
			m_feedbackMessage = "Failure!";
		}
	}
}

void Session::updatePresentationState()
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
			if (isComplete()) {
				m_logger->closeResultsFile();																// Close the current results file
				m_app->markSessComplete(String(m_trialParams[m_currTrialIdx][0].str["sessionID"]));			// Add this session to user's completed sessions
				m_app->updateSessionDropDown();

				int score = int(m_totalRemainingTime);
				m_feedbackMessage = format("Session complete! You scored %d!", score);						// Update the feedback message
				newState = PresentationState::scoreboard;
			}
			else {
				m_feedbackMessage = "";
				nextCondition();
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

void Session::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
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

void Session::recordTrialResponse()
{
	//String sess = String(m_psych.mMeasurements[m_psych.mCurrentConditionIndex].TargetParameters[0].str["session"]);
	String sess = String(m_trialParams[m_currTrialIdx][0].str["sessionID"]);

	// Trials table. Record trial start time, end time, and task completion time.
	Array<String> trialValues = {
		String(std::to_string(m_currTrialIdx)),
		"'" + sess + "'",
		"'" + m_config.getSessionConfigById(sess)->sessDescription + "'",
		"'" + m_taskStartTime + "'",
		"'" + m_taskEndTime + "'",
		String(std::to_string(m_taskExecutionTime)),
		String(std::to_string(m_response))
	};
	m_logger->recordTrialResponse(trialValues);

	// Target_Trajectory table. Write down the recorded target trajectories.
	m_logger->recordTargetTrajectory(m_targetTrajectory);
	m_targetTrajectory.clear();

	// Player_Action table. Write down the recorded player actions.
	m_logger->recordPlayerActions(m_playerActions);
	m_playerActions.clear();

	// Frame_Info table. Write down all frame info.
	m_logger->recordFrameInfo(m_frameInfo);
	m_frameInfo.clear();
}

void Session::accumulateTrajectories()
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

void Session::accumulatePlayerAction(String action, String targetName)
{
	BEGIN_PROFILER_EVENT("accumulatePlayerAction");
	// recording target trajectories
	Point2 dir = m_app->getViewDirection();
	Point3 loc = m_app->getPlayerLocation();
	Array<String> playerActionValues = {
		"'" + Logger::genUniqueTimestamp() + "'",
		String(std::to_string(dir.x)),
		String(std::to_string(dir.y)),
		String(std::to_string(loc.x)),
		String(std::to_string(loc.y)),
		String(std::to_string(loc.z)),
		"'" + action + "'",
		"'" + targetName + "'",
	};
	m_playerActions.push_back(playerActionValues);
	END_PROFILER_EVENT();
}

void Session::accumulateFrameInfo(RealTime t, float sdt, float idt) {
	Array<String> frameValues = {
		"'" + Logger::genUniqueTimestamp() + "'",
		String(std::to_string(idt)),
		String(std::to_string(sdt))
	};
	m_frameInfo.push_back(frameValues);
}

bool Session::responseReady() {
	double timeNow = System::time();
	if ((timeNow - m_lastFireAt) > (m_config.weapon.firePeriod)) {
		m_lastFireAt = timeNow;
		return true;
	}
	else {
		return false;
	}
}

double Session::weaponCooldownPercent() {
	if (m_config.weapon.firePeriod == 0.0f) return 1.0;
	return min((System::time() - m_lastFireAt) / m_config.weapon.firePeriod, 1.0);
}

int Session::remainingAmmo() {
	return m_config.weapon.maxAmmo - m_clickCount;
}


float Session::getRemainingTrialTime() {
	return m_config.taskDuration - m_timer.getTime();
}

float Session::getProgress() {
	if (m_session != nullptr) {
		int completed = 0;
		for (bool c : m_remaining) {
			if (c) completed++;
		}
		return completed / (float)m_session->getTotalTrials();
	}
	return fnan();
}

int Session::getScore() {
	return (int)(10.0 * m_totalRemainingTime);
}

String Session::getFeedbackMessage() {
	return m_feedbackMessage;
}