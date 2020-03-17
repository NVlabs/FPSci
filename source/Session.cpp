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
#include "Logger.h"
#include "TargetEntity.h"
#include "PlayerEntity.h"
#include "Dialogs.h"

void Session::nextCondition() {
	Array<int> unrunTrialIdxs;
	for (int i = 0; i < m_remainingTrials.size(); i++) {
		if (m_remainingTrials[i] > 0 || m_remainingTrials[i] == -1) {
			unrunTrialIdxs.append(i);
		}
	}
	if (unrunTrialIdxs.size() == 0) return;
	int idx = Random::common().integer(0, unrunTrialIdxs.size()-1);
	m_currTrialIdx = unrunTrialIdxs[idx];
}

bool Session::isComplete() const{
	bool allTrialsComplete = true;
	for (int remaining : m_remainingTrials) {
		allTrialsComplete = allTrialsComplete && (remaining == 0);
	}
	return allTrialsComplete;
}

bool Session::setupTrialParams(Array<Array<shared_ptr<TargetConfig>>> trials) {
	for (int i = 0; i < trials.size(); i++) {
		Array<shared_ptr<TargetConfig>> targets = trials[i];
		for (int j = 0; j < targets.size(); j++) {
			const String name = format("%s_%d_%s_%d", m_config->id, i, targets[j]->id, j);
			if (m_config->logger.enable) {
				m_logger->addTarget(name, targets[j], m_config->render.frameRate, m_config->render.frameDelay);
			}			
		}
		m_remainingTrials.append(m_config->trials[i].count);
		m_targetConfigs.append(targets);
	}
	nextCondition();
	return true;
}

void Session::onInit(String filename, String description) {
	// Initialize presentation states
	presentationState = PresentationState::initial;
	m_feedbackMessage = "Click to spawn a target, then use shift on red target to begin.";

	// Get the player from the app
	m_player = m_app->scene()->typedEntity<PlayerEntity>("player");
	m_scene = m_app->scene().get();
	m_camera = m_app->activeCamera();

	m_targetModels = &(m_app->targetModels);
	m_modelScaleCount = m_app->modelScaleCount;

	// Check for valid session
	if (m_hasSession) {
		if (m_config->logger.enable) {
			UserConfig user = *m_app->getCurrUser();
			// Setup the logger and create results file
			m_logger = Logger::create(filename, user.id, m_config, description);
			if (m_config->logger.logUsers) {
				m_logger->logUserConfig(user, m_config->id, "start");
			}
		}
		// Iterate over the sessions here and add a config for each
		Array<Array<shared_ptr<TargetConfig>>> trials = m_app->experimentConfig.getTargetsForSession(m_config->id);
		setupTrialParams(trials);
	}
	else {	// Invalid session, move to displaying message
		presentationState = PresentationState::scoreboard;
	}
}

void Session::randomizePosition(const shared_ptr<TargetEntity>& target) const {
	static const Point3 initialSpawnPos = m_camera->frame().translation;
	shared_ptr<TargetConfig> config = m_targetConfigs[m_currTrialIdx][target->paramIdx()];
	const bool isWorldSpace = config->destSpace == "world";
	Point3 loc;

	if (isWorldSpace) {
		loc = config->spawnBounds.randomInteriorPoint();		// Set a random position in the bounds
		target->resetMotionParams();							// Reset the target motion behavior
	}
	else {
		const float rot_pitch = randSign() * Random::common().uniform(config->eccV[0], config->eccV[1]);
		const float rot_yaw = randSign() * Random::common().uniform(config->eccH[0], config->eccH[1]);
		const CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, rot_yaw - 180.0f/(float)pi()*initialHeadingRadians, rot_pitch, 0.0f);
		loc = f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
	}
	target->setFrame(loc);
}

void Session::initTargetAnimation() {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	const Point3 initialSpawnPos = m_player->getCameraFrame().translation;
	CFrame f = CFrame::fromXYZYPRRadians(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, -initialHeadingRadians, 0.0f, 0.0f);

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (presentationState == PresentationState::task) {
		// Iterate through the targets
		for (int i = 0; i < m_targetConfigs[m_currTrialIdx].size(); i++) {
			
			const String name = format("%s_%d_%s_%d", m_config->id, m_currTrialIdx, m_targetConfigs[m_currTrialIdx][i]->id, i);
			const Color3 initColor = m_config->targetView.healthColors[0];
			shared_ptr<TargetConfig> target = m_targetConfigs[m_currTrialIdx][i];

			const float rot_pitch = randSign() * Random::common().uniform(target->eccV[0], target->eccV[1]);
			const float rot_yaw = randSign() * Random::common().uniform(target->eccH[0], target->eccH[1]);
			const float targetSize = G3D::Random().common().uniform(target->size[0], target->size[1]);
			bool isWorldSpace = target->destSpace == "world";

			CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, rot_yaw- (initialHeadingRadians * 180.0f / (float)pi()), rot_pitch, 0.0f);

			// Check for case w/ destination array
			if (target->destinations.size() > 0) {
				Point3 offset =isWorldSpace ? Point3(0.0, 0.0, 0.0) : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				shared_ptr<TargetEntity> t = spawnDestTarget(target, offset, initColor, i, name);
			}
			// Otherwise check if this is a jumping target
			else if (target->jumpEnabled) {
				Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				shared_ptr<JumpingEntity> t = spawnJumpingTarget(target, offset, initialSpawnPos, initColor, m_targetDistance, i, name);
			}
			else {
				Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
				shared_ptr<FlyingEntity> t = spawnFlyingTarget(target, offset, initialSpawnPos, initColor, i, name);
			}
		}
	}
	else {
		// Make sure we reset the target color here (avoid color bugs)
		spawnReferenceTarget(
			f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
			initialSpawnPos,
			m_config->targetView.refTargetSize,
			m_config->targetView.refTargetColor
		);
	}

	// Reset number of destroyed targets
	m_destroyedTargets = 0;
	// reset click counter
	m_clickCount = 0;
}

void Session::processResponse()
{
	m_taskExecutionTime = m_timer.getTime();
	// Get total target count here
	int totalTargets = 0;
	for (shared_ptr<TargetConfig> target : m_targetConfigs[m_currTrialIdx]) {
		if (target->respawnCount == -1) {
			totalTargets = -1;		// Ininite spawn case
			break;
		}
		else {
			totalTargets += (target->respawnCount+1);
		}
	}
	
	recordTrialResponse(m_destroyedTargets, totalTargets); // NOTE: we need record response first before processing it with PsychHelper.
	if (m_remainingTrials[m_currTrialIdx] > 0) {
		m_remainingTrials[m_currTrialIdx] -= 1;
	}

	// Check for whether all targets have been destroyed
	if (m_remainingTargets == 0) {
		m_totalRemainingTime += (double(m_config->timing.taskDuration) - m_taskExecutionTime);
		if (m_config->description == "training") {
			m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
		}
	}
	else {
		if (m_config->description == "training") {
			m_feedbackMessage = "Failure!";
		}
	}
}

void Session::updatePresentationState()
{
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState currentState = presentationState;
	PresentationState newState;
	int remainingTargets = m_targetArray.size();
	float stateElapsedTime = m_timer.getTime();
	newState = currentState;

	if (currentState == PresentationState::initial)
	{
		if (m_config->player.stillBetweenTrials) {
			m_player->setMoveEnable(false);
		}
		if (!m_app->m_buttonUp)
		{
			//m_feedbackMessage = "";
			newState = PresentationState::feedback;
		}
	}
	else if (currentState == PresentationState::ready)
	{
		if (stateElapsedTime > m_config->timing.readyDuration)
		{
			newState = PresentationState::task;
			if (m_config->player.stillBetweenTrials) {
				m_player->setMoveEnable(true);
			}
		}
	}
	else if (currentState == PresentationState::task)
	{
		if ((stateElapsedTime > m_config->timing.taskDuration) || (remainingTargets <= 0) || (m_clickCount == m_config->weapon.maxAmmo))
		{
			m_taskEndTime = Logger::genUniqueTimestamp();
			processResponse();
			clearTargets(); // clear all remaining targets
			newState = PresentationState::feedback;
			if (m_config->player.stillBetweenTrials) {
				m_player->setMoveEnable(false);
			}
			if (m_config->player.resetPositionPerTrial) {
				m_player->respawn();
			}
		}
	}
	else if (currentState == PresentationState::feedback)
	{
		if ((stateElapsedTime > m_config->timing.feedbackDuration) && (remainingTargets <= 0))
		{
			if (isComplete()) {
				if (m_config->questionArray.size() > 0 && m_currQuestionIdx < m_config->questionArray.size()) {			// Pop up question dialog(s) here if we need to
					if (m_currQuestionIdx == -1){
						m_currQuestionIdx = 0;
						m_app->presentQuestion(m_config->questionArray[m_currQuestionIdx]);
					}
					else if (!m_app->dialog->visible()) {														// Check for whether dialog is closed (otherwise we are waiting for input)
						if (m_app->dialog->complete) {															// Has this dialog box been completed? (or was it closed without an answer?)
							m_config->questionArray[m_currQuestionIdx].result = m_app->dialog->result;			// Store response w/ quesiton
							if (m_config->logger.enable) {
								m_logger->addQuestion(m_config->questionArray[m_currQuestionIdx], m_config->id);	// Log the question and its answer
							}
							m_currQuestionIdx++;																// Present the next question (if there is one)
							if (m_currQuestionIdx < m_config->questionArray.size()) {							// Double check we have a next question before launching the next question
								m_app->presentQuestion(m_config->questionArray[m_currQuestionIdx]);
							}
						}
						else {
							m_app->presentQuestion(m_config->questionArray[m_currQuestionIdx]);					// Relaunch the same dialog (this wasn't completed)
						}	
					}	
				}
				else {
					if (m_config->logger.enable) {
						m_logger->logUserConfig(*m_app->getCurrUser(), m_config->id, "end");
						m_logger->flush(false);
						m_logger.reset();
					}
					m_app->markSessComplete(m_config->id);														// Add this session to user's completed sessions
					m_app->updateSessionDropDown();

					int score = int(m_totalRemainingTime);
					m_feedbackMessage = format("Session complete! You scored %d!", score);						// Update the feedback message
					m_currQuestionIdx = -1;
					newState = PresentationState::scoreboard;
				}
			}
			else {
				m_feedbackMessage = "";
				nextCondition();
				newState = PresentationState::ready;
			}
		}
	}
	else if (currentState == PresentationState::scoreboard) {
		//if (stateElapsedTime > m_scoreboardDuration) {
			newState = PresentationState::complete;
			m_app->openUserSettingsWindow();
			if (m_hasSession) {
				m_app->userSaveButtonPress();												// Press the save button for the user...
				Array<String> remaining = m_app->updateSessionDropDown();
				if (remaining.size() == 0) {
					m_feedbackMessage = "All Sessions Complete!"; // Update the feedback message
					moveOn = false;
				}
				else {
					m_feedbackMessage = "Session Complete!"; // Update the feedback message
					moveOn = true;														// Check for session complete (signal start of next session)
				}
			}
			else {
				m_feedbackMessage = "All Sessions Complete!";							// Update the feedback message
				moveOn = false;
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

void Session::recordTrialResponse(int destroyedTargets, int totalTargets)
{
	if (!m_config->logger.enable) return;		// Skip this if the logger is disabled
	if (m_config->logger.logTrialResponse) {
		// Trials table. Record trial start time, end time, and task completion time.
		Logger::TrialValues trialValues = {
			String(std::to_string(m_currTrialIdx)),
			"'" + m_config->id + "'",
			"'" + m_config->description + "'",
			"'" + m_taskStartTime + "'",
			"'" + m_taskEndTime + "'",
			String(std::to_string(m_taskExecutionTime)),
			String(std::to_string(destroyedTargets)),
			String(std::to_string(totalTargets))
		};
		m_logger->logTrial(trialValues);
	}
}

void Session::accumulateTrajectories()
{
	if (notNull(m_logger) && m_config->logger.logTargetTrajectories) {
		for (shared_ptr<TargetEntity> target : m_targetArray) {
			if (!target->isLogged()) continue;
			// recording target trajectories
			Point3 targetAbsolutePosition = target->frame().translation;
			Point3 initialSpawnPos = m_camera->frame().translation;
			Point3 targetPosition = targetAbsolutePosition - initialSpawnPos;
					   
			//// below for 2D direction calculation (azimuth and elevation)
			//Point3 t = targetPosition.direction();
			//float az = atan2(-t.z, -t.x) * 180 / pif();
			//float el = atan2(t.y, sqrtf(t.x * t.x + t.z * t.z)) * 180 / pif();
			TargetLocation location = TargetLocation(Logger::getFileTime(), target->name(), targetPosition);
			m_logger->logTargetLocation(location);
		}
	}
	// recording view direction trajectories
	accumulatePlayerAction(PlayerActionType::Aim);
}

void Session::accumulatePlayerAction(PlayerActionType action, String targetName)
{
	if (notNull(m_logger) && m_config->logger.logPlayerActions) {
		BEGIN_PROFILER_EVENT("accumulatePlayerAction");
		// recording target trajectories
		Point2 dir = getViewDirection();
		Point3 loc = getPlayerLocation();
		PlayerAction pa = PlayerAction(Logger::getFileTime(), dir, loc, action, targetName);
		m_logger->logPlayerAction(pa);
		END_PROFILER_EVENT();
	}
}

void Session::accumulateFrameInfo(RealTime t, float sdt, float idt) {
	if (notNull(m_logger) && m_config->logger.logFrameInfo) {
		m_logger->logFrameInfo(FrameInfo(Logger::getFileTime(), sdt));
	}
}

bool Session::canFire() {
	if (isNull(m_config)) return true;
	double timeNow = System::time();
	if ((timeNow - m_lastFireAt) > (m_config->weapon.firePeriod)) {
		m_lastFireAt = timeNow;
		return true;
	}
	else {
		return false;
	}
}

float Session::weaponCooldownPercent() const {
	if (isNull(m_config)) return 1.0;
	if (m_config->weapon.firePeriod == 0.0f) return 1.0;
	return min((float)(System::time() - m_lastFireAt) / m_config->weapon.firePeriod, 1.0f);
}

int Session::remainingAmmo() const {
	if (isNull(m_config)) return 100;
	return m_config->weapon.maxAmmo - m_clickCount;
}


float Session::getRemainingTrialTime() {
	if (isNull(m_config)) return 10.0;
	return m_config->timing.taskDuration - m_timer.getTime();
}

float Session::getProgress() {
	if (notNull(m_config)) {
		float remainingTrials = 0.f;
		for (int tcount : m_remainingTrials) {
			if (tcount < 0) return 0.f;				// Infinite trials, never make any progress
			remainingTrials += (float)tcount;
		}
		return 1.f - (remainingTrials / m_config->getTotalTrials());
	}
	return fnan();
}

int Session::getScore() {
	return (int)(10.0 * m_totalRemainingTime);
}

String Session::getFeedbackMessage() {
	return m_feedbackMessage;
}

void Session::endLogging() {
	if (m_logger != nullptr) {
		m_logger.reset();
	}
}

// Comment these since they are unused
/** Spawn a randomly parametrized target */
//void App::spawnParameterizedRandomTarget(float motionDuration=4.0f, float motionDecisionPeriod=0.5f, float speed=2.0f, float radius=10.0f, float scale=2.0f) {
//    Random& rng = Random::threadCommon();
//
//    // Construct a reference frame
//    // Remove the vertical component
//    Vector3 Z = -activeCamera()->frame().lookVector();
//    debugPrintf("lookatZ = [%.4f, %.4f, %.4f]\n", Z.x, Z.y, Z.z);
//    debugPrintf("origin  = [%.4f, %.4f, %.4f]\n", activeCamera()->frame().translation.x, activeCamera()->frame().translation.y, activeCamera()->frame().translation.z);
//    Z.y = 0.0f;
//    Z = Z.direction();
//    Vector3 Y = Vector3::unitY();
//    Vector3 X = Y.cross(Z);
//
//    // Make a random vector in front of the player in a narrow field of view
//    Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.5f, 0.5f)).direction();
//
//    // Ray from user/camera toward intended spawn location
//    Ray ray = Ray::fromOriginAndDirection(activeCamera()->frame().translation, dir);
//
//    //distance = rng.uniform(2.0f, distance - 1.0f);
//    const shared_ptr<FlyingEntity>& target =
//        spawnTarget(ray.origin() + ray.direction() * radius,
//            scale, false,
//            Color3::wheelRandom());
//
//    // Choose some destination locations based on speed and motionDuration
//    const Point3& center = ray.origin();
//    Array<Point3> destinationArray;
//    // [radians/s] = [m/s] / [m/radians]
//    float angularSpeed = speed / radius;
//    // [rad] = [rad/s] * [s] 
//    float angleChange = angularSpeed * motionDecisionPeriod;
//
//    destinationArray.push(target->frame().translation);
//    int tempInt = 0;
//    for (float motionTime = 0.0f; motionTime < motionDuration; motionTime += motionDecisionPeriod) {
//        // TODO: make angle change randomize correction, should be placed on circle around previous point
//        float pitch = 0.0f;
//        float yaw = tempInt++ % 2 == 0 ? angleChange : -angleChange;
//        //float yaw = rng.uniform(-angleChange, angleChange);
//        //float pitch = rng.uniform(-angleChange, angleChange);
//        const Vector3& dir = CFrame::fromXYZYPRRadians(0.0f, 0.0f, 0.0f, yaw, pitch, 0.0f).rotation * ray.direction();
//        ray.set(ray.origin(), dir);
//        destinationArray.push(center + dir * radius);
//    }
//    target->setSpeed(speed); // m/s
//    // debugging prints
//    for (Point3* p = destinationArray.begin(); p != destinationArray.end(); ++p) {
//        debugPrintf("[%.2f, %.2f, %.2f]\n", p->x, p->y, p->z);
//    }
//    target->setDestinations(destinationArray, center);
//}
//
///** Spawn a random non-parametrized target */
//void Session::spawnRandomTarget() {
//	Random& rng = Random::threadCommon();
//
//	bool done = false;
//	int tries = 0;
//
//	// Construct a reference frame
//	// Remove the vertical component
//	Vector3 Z = -activeCamera()->frame().lookVector();
//	Z.y = 0.0f;
//	Z = Z.direction();
//	Vector3 Y = Vector3::unitY();
//	Vector3 X = Y.cross(Z);
//
//	do {
//		// Make a random vector in front of the player in a narrow field of view
//		Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.3f, 0.5f)).direction();
//
//		// Make sure the spawn location is visible
//		Ray ray = Ray::fromOriginAndDirection(activeCamera()->frame().translation, dir);
//		float distance = finf();
//		scene()->intersect(ray, distance);
//
//		if ((distance > 2.0f) && (distance < finf())) {
//            distance = rng.uniform(2.0f, distance - 1.0f);
//			const shared_ptr<FlyingEntity>& target =
//                spawnTarget(ray.origin() + ray.direction() * distance, 
//                    rng.uniform(0.1f, 1.5f), rng.uniform() > 0.5f,
//                    Color3::wheelRandom());
//
//            // Choose some destination locations
//            const Point3& center = ray.origin();
//            Array<Point3> destinationArray;
//            destinationArray.push(target->frame().translation);
//            for (int i = 0; i < 20; ++i) {
//        		const Vector3& dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.3f, 0.5f)).direction();
//                destinationArray.push(center + dir * distance);
//            }
//            target->setSpeed(2.0f); // m/s
//            target->setDestinations(destinationArray, center);
//
//			done = true;
//		}
//		++tries;
//	} while (!done && tries < 100);
//}

/** Spawn a flying entity target */
//shared_ptr<FlyingEntity> Session::spawnTarget(const Point3& position, float scale, bool spinLeft, const Color3& color, String modelName) {
//	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
//	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(format("target%03d", ++m_lastUniqueID), m_scene, m_targetModels[modelName][scaleIndex], CFrame());
//	target->setFrame(position);
//	target->setColor(color);
//
//	// Don't set a track. We'll take care of the positioning after creation
//	/*
//	String animation = format("combine(orbit(0, %d), CFrame::fromXYZYPRDegrees(%f, %f, %f))", spinLeft ? 1 : -1, position.x, position.y, position.z);
//	const shared_ptr<Entity::Track>& track = Entity::Track::create(target.get(), scene().get(), Any::parse(animation));
//	target->setTrack(track);
//	*/
//
//	insertTarget(target);
//	return target;
//}

shared_ptr<TargetEntity> Session::spawnDestTarget(
	shared_ptr<TargetConfig> config,
	const Point3& position,
	const Color3& color,
	const int paramIdx,
	const String& name)
{
	// Create the target
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);

	const shared_ptr<TargetEntity>& target = TargetEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], position, scaleIndex, paramIdx);

	// Update parameters for the target
	target->setHitSound(config->hitSound, config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, config->destroyedSoundVol);
	target->setFrame(position);
	target->setColor(color);

	// Add target to array and scene
	insertTarget(target);
	return target;
}

shared_ptr<FlyingEntity> Session::spawnReferenceTarget(
	const Point3& position,
	const Point3& orbitCenter,
	const float size,
	const Color3& color)
{
	const int scaleIndex = clamp(iRound(log(size) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	const shared_ptr<FlyingEntity>& target = FlyingEntity::create("reference", m_scene, (*m_targetModels)["reference"][scaleIndex], CFrame());

	// Setup additional target parameters
	target->setFrame(position);
	target->setColor(color);

	// Add target to array and scene
	insertTarget(target);
	return target;
}

shared_ptr<FlyingEntity> Session::spawnFlyingTarget(
	shared_ptr<TargetConfig> config,
	const Point3& position,
	const Point3& orbitCenter,
	const Color3& color,
	const int paramIdx,
	const String& name)
{
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const bool isWorldSpace = config->destSpace == "world";

	// Setup the target
	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], orbitCenter, scaleIndex, paramIdx);
	target->setFrame(position);
	target->setWorldSpace(isWorldSpace);
	if (isWorldSpace) {
		target->setBounds(config->moveBounds);
	}
	target->setHitSound(config->hitSound, config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, config->destroyedSoundVol);
	target->setColor(color);

	// Add the target to the scene/target array
	insertTarget(target);
	return target;
}

shared_ptr<JumpingEntity> Session::spawnJumpingTarget(
	shared_ptr<TargetConfig> config,
	const Point3& position,
	const Point3& orbitCenter,
	const Color3& color,
	const float targetDistance,
	const int paramIdx,
	const String& name)
{
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const bool isWorldSpace = config->destSpace == "world";

	// Setup the target
	const shared_ptr<JumpingEntity>& target = JumpingEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], scaleIndex, orbitCenter, targetDistance, paramIdx);
	target->setFrame(position);
	target->setWorldSpace(isWorldSpace);
	if (isWorldSpace) {
		target->setMoveBounds(config->moveBounds);
	}
	target->setHitSound(config->hitSound, config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, config->destroyedSoundVol);
	target->setColor(color);

	// Add the target to the scene/target array
	insertTarget(target);
	return target;
}

void Session::insertTarget(shared_ptr<TargetEntity> target) {
	target->setShouldBeSaved(false);
	m_targetArray.append(target);
	m_scene->insert(target);
}

void Session::destroyTarget(int index) {
	// Not a reference because we're about to manipulate the array
	const shared_ptr<VisibleEntity> target = m_targetArray[index];
	// Remove the target from the target array
	m_targetArray.fastRemove(index);
	// Remove the target from the scene
	m_scene->removeEntity(target->name());
}

void Session::destroyTarget(shared_ptr<TargetEntity> target) {
	for (int i = 0; i < m_targetArray.size(); i++) {
		if (m_targetArray[i]->name() == target->name()) {
			m_targetArray.fastRemove(i);
		}
	}
	m_scene->removeEntity(target->name());
}

/** Clear all targets one by one */
void Session::clearTargets() {
	while (m_targetArray.size() > 0) {
		destroyTarget(0);
	}
}