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
#include "FPSciApp.h"
#include "Logger.h"
#include "TargetEntity.h"
#include "PlayerEntity.h"
#include "Dialogs.h"

bool Session::hasNextCondition() const{
	for (int count : m_remainingTrials) {
		if (count > 0) return true;
	}
	return false;
}

bool Session::nextCondition() {
	Array<int> unrunTrialIdxs;
	for (int i = 0; i < m_remainingTrials.size(); i++) {
		if (m_remainingTrials[i] > 0 || m_remainingTrials[i] == -1) {
			unrunTrialIdxs.append(i);
		}
	}
	if (unrunTrialIdxs.size() == 0) return false;
	int idx = Random::common().integer(0, unrunTrialIdxs.size()-1);
	m_currTrialIdx = unrunTrialIdxs[idx];
	return true;
}

bool Session::blockComplete() const{
	bool allTrialsComplete = true;
	for (int remaining : m_remainingTrials) {
		allTrialsComplete = allTrialsComplete && (remaining == 0);
	}
	return allTrialsComplete;
}

bool Session::updateBlock(bool updateTargets) {
	for (int i = 0; i < m_trials.size(); i++) {
		Array<shared_ptr<TargetConfig>> targets = m_trials[i];
		if (m_config->logger.enable && updateTargets) {
			for (int j = 0; j < targets.size(); j++) {
				const String name = format("%s_%d_%s_%d", m_config->id, i, targets[j]->id, j);
				m_logger->addTarget(name, targets[j], m_config->render.frameRate, m_config->render.frameDelay);
			}
		}
		m_remainingTrials.append(m_config->trials[i].count);
		m_targetConfigs.append(targets);
	}
	return nextCondition();
}

void Session::onInit(String filename, String description) {
	// Initialize presentation states
	currentState = PresentationState::initial;
	if (m_config) {
		m_feedbackMessage = formatFeedback(m_config->targetView.showRefTarget ? m_config->feedback.initialWithRef: m_config->feedback.initialNoRef);
	}

	// Get the player from the app
	m_player = m_app->scene()->typedEntity<PlayerEntity>("player");
	m_scene = m_app->scene().get();
	m_camera = m_app->activeCamera();

	m_targetModels = &(m_app->targetModels);

	// Check for valid session
	if (m_hasSession) {
		if (m_config->logger.enable) {
			UserConfig user = *m_app->currentUser();
			// Setup the logger and create results file
			m_logger = FPSciLogger::create(filename, user.id, m_config, description);
			m_dbFilename = filename.substr(0, filename.length() - 3);
			if (m_config->logger.logUsers) {
				m_logger->logUserConfig(user, m_config->id, "start", m_config->player.turnScale);
			}
		}

		runSessionCommands("start");				// Run start of session commands

		// Iterate over the sessions here and add a config for each
		m_trials = m_app->experimentConfig.getTargetsForSession(m_config->id);
		updateBlock(true);
	}
	else {	// Invalid session, move to displaying message
		currentState = PresentationState::sessionFeedback;
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

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (currentState == PresentationState::trialTask) {
		if (m_config->targetView.previewWithRef && m_config->targetView.showRefTarget) {
			// Activate the preview targets
			for (shared_ptr<TargetEntity> target : m_targetArray) {
				target->setCanHit(true);
				m_app->updateTargetColor(target);
				m_hittableTargets.append(target);
			}
		}
		else {
			spawnTrialTargets(initialSpawnPos);			// Spawn all the targets normally
		}
	}
	else { // State is feedback and we are spawning a reference target
		CFrame f = CFrame::fromXYZYPRRadians(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, -initialHeadingRadians, 0.0f, 0.0f);
		// Spawn the reference target
		auto t = spawnReferenceTarget(
			f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
			initialSpawnPos,
			m_config->targetView.refTargetSize,
			m_config->targetView.refTargetColor
		);
		m_hittableTargets.append(t);

		if (m_config->targetView.previewWithRef) {
			spawnTrialTargets(initialSpawnPos, true);		// Spawn all the targets in preview mode
		}
	}

	// Reset number of destroyed targets (in the trial)
	m_destroyedTargets = 0;
	// Reset shot and hit counters (in the trial)
	m_shotCount = 0;
	m_hitCount = 0;
}

void Session::spawnTrialTargets(Point3 initialSpawnPos, bool previewMode) {
	// Iterate through the targets
	for (int i = 0; i < m_targetConfigs[m_currTrialIdx].size(); i++) {

		const String name = format("%s_%d_%s_%d", m_config->id, m_currTrialIdx, m_targetConfigs[m_currTrialIdx][i]->id, i);
		const Color3 spawnColor = previewMode ? m_config->targetView.previewColor : m_config->targetView.healthColors[0];
		shared_ptr<TargetConfig> target = m_targetConfigs[m_currTrialIdx][i];

		const float rot_pitch = randSign() * Random::common().uniform(target->eccV[0], target->eccV[1]);
		const float rot_yaw = randSign() * Random::common().uniform(target->eccH[0], target->eccH[1]);
		const float targetSize = G3D::Random().common().uniform(target->size[0], target->size[1]);
		bool isWorldSpace = target->destSpace == "world";

		CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, rot_yaw - (initialHeadingRadians * 180.0f / (float)pi()), rot_pitch, 0.0f);

		// Check for case w/ destination array
		shared_ptr<TargetEntity> t;
		if (target->destinations.size() > 0) {
			Point3 offset = isWorldSpace ? Point3(0.0, 0.0, 0.0) : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnDestTarget(target, offset, spawnColor, i, name);
		}
		// Otherwise check if this is a jumping target
		else if (target->jumpEnabled) {
			Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnJumpingTarget(target, offset, initialSpawnPos, spawnColor, m_targetDistance, i, name);
		}
		else {
			Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnFlyingTarget(target, offset, initialSpawnPos, spawnColor, i, name);
		}

		// Set whether the target can be hit based on whether we are in preview mode
		t->setCanHit(!previewMode);
		previewMode ? m_unhittableTargets.append(t) : m_hittableTargets.append(t);
	}
}

void Session::processResponse()
{
	m_taskExecutionTime = m_timer.getTime();

	const int totalTargets = totalTrialTargets();
	// Record the trial response into the database
	recordTrialResponse(m_destroyedTargets, totalTargets); 
	if (m_remainingTrials[m_currTrialIdx] > 0) {
		m_remainingTrials[m_currTrialIdx] -= 1;
	}

	// Check for whether all targets have been destroyed
	if (m_destroyedTargets == totalTargets) {
		m_totalRemainingTime += (double(m_config->timing.maxTrialDuration) - m_taskExecutionTime);
		m_feedbackMessage = formatFeedback(m_config->feedback.trialSuccess);
	}
	else {
		m_feedbackMessage = formatFeedback(m_config->feedback.trialFailure);
	}
}

void Session::updatePresentationState()
{
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState newState;
	int remainingTargets = m_hittableTargets.size();
	float stateElapsedTime = m_timer.getTime();
	newState = currentState;

	if (currentState == PresentationState::initial)
	{
		if (m_config->player.stillBetweenTrials) {
			m_player->setMoveEnable(false);
		}
		if (!(m_app->buttonUp && m_config->timing.clickToStart)) {
			newState = PresentationState::trialFeedback;
		}
	}
	else if (currentState == PresentationState::pretrial)
	{
		if (stateElapsedTime > m_config->timing.pretrialDuration)
		{
			newState = PresentationState::trialTask;
			if (m_config->player.stillBetweenTrials) {
				m_player->setMoveEnable(true);
			}

			closeTrialProcesses();						// End previous process (if running)
			runTrialCommands("start");					// Run start of trial commands

		}
	}
	else if (currentState == PresentationState::trialTask)
	{
		if ((stateElapsedTime > m_config->timing.maxTrialDuration) || (remainingTargets <= 0) || (m_shotCount == m_config->weapon.maxAmmo))
		{
			m_taskEndTime = FPSciLogger::genUniqueTimestamp();
			processResponse();
			clearTargets(); // clear all remaining targets
			newState = PresentationState::trialFeedback;
			if (m_config->player.stillBetweenTrials) {
				m_player->setMoveEnable(false);
			}
			if (m_config->player.resetPositionPerTrial) {
				m_player->respawn();
			}

			closeTrialProcesses();				// Stop start of trial processes
			runTrialCommands("end");			// Run the end of trial processes

			// Reset weapon cooldown
			m_lastFireAt = 0.f;
		}
	}
	else if (currentState == PresentationState::trialFeedback)
	{
		if ((stateElapsedTime > m_config->timing.trialFeedbackDuration) && (remainingTargets <= 0))
		{
			if (blockComplete()) {
				m_currBlock++;		// Increment the block index
				if (m_currBlock > m_config->blockCount) {
					// Check for end of session (all blocks complete)
					if (m_config->questionArray.size() > 0 && m_currQuestionIdx < m_config->questionArray.size()) {
						// Pop up question dialog(s) here if we need to
						if (m_currQuestionIdx == -1) {
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
							endLogging();
						}
						m_app->markSessComplete(m_config->id);														// Add this session to user's completed sessions

						m_feedbackMessage = formatFeedback(m_config->feedback.sessComplete);						// Update the feedback message
						m_currQuestionIdx = -1;
						newState = PresentationState::sessionFeedback;
					}
				}
				else {					// Block is complete but session isn't
					m_feedbackMessage = formatFeedback(m_config->feedback.blockComplete);
					updateBlock();
					newState = PresentationState::initial;
				}
			}
			else {
				m_feedbackMessage = "";				// Clear the feedback message
				nextCondition();
				newState = PresentationState::pretrial;
			}
		}
	}
	else if (currentState == PresentationState::sessionFeedback) {
		if (m_hasSession) {
			if (stateElapsedTime > m_config->timing.sessionFeedbackDuration && (!m_config->timing.sessionFeedbackRequireClick || !m_app->buttonUp)) {
				newState = PresentationState::complete;
        
				// Save current user config and status
				m_app->saveUserConfig();											
				m_app->saveUserStatus();
        
				closeSessionProcesses();					// Close the process we started at session start (if there is one)
				runSessionCommands("end");					// Launch processes for the end of the session

				Array<String> remaining = m_app->updateSessionDropDown();
				if (remaining.size() == 0) {
					m_feedbackMessage = formatFeedback(m_config->feedback.allSessComplete); // Update the feedback message
					moveOn = false;
					if (m_app->experimentConfig.closeOnComplete || m_config->closeOnComplete) {
						m_app->quitRequest();
					}
				}
				else {
					m_feedbackMessage = formatFeedback(m_config->feedback.sessComplete);	// Update the feedback message
					if (m_config->closeOnComplete) {
						m_app->quitRequest();
					}
					moveOn = true;														// Check for session complete (signal start of next session)
				}

				if (m_app->experimentConfig.closeOnComplete) {
					m_app->quitRequest();
				}
			}
		}
		else {
			// Go ahead and move to the complete state since there aren't any valid sessions
			newState = PresentationState::complete;
			m_feedbackMessage = formatFeedback("All sessions complete!");
			moveOn = false;
		}
	}

	else {
		newState = currentState;
	}

	if (currentState != newState)
	{ // handle state transition.
		m_timer.startTimer();
		if (newState == PresentationState::trialTask) {
			m_taskStartTime = FPSciLogger::genUniqueTimestamp();
		}
		currentState = newState;
		//If we switched to task, call initTargetAnimation to handle new trial
		if ((newState == PresentationState::trialTask) || (newState == PresentationState::trialFeedback && hasNextCondition() && m_config->targetView.showRefTarget)) {
			initTargetAnimation();
		}
	}
}

void Session::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
{
	// 1. Update presentation state and send task performance to psychophysics library.
	updatePresentationState();

	// 2. Record target trajectories, view direction trajectories, and mouse motion.
	if (currentState == PresentationState::trialTask)
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
		FPSciLogger::TrialValues trialValues = {
			String(std::to_string(m_currTrialIdx)),
			"'" + m_config->id + "'",
			"'" + m_config->description + "'",
			format("'Block %d'", m_currBlock),
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
			TargetLocation location = TargetLocation(FPSciLogger::getFileTime(), target->name(), targetPosition);
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
		PlayerAction pa = PlayerAction(FPSciLogger::getFileTime(), dir, loc, action, targetName);
		m_logger->logPlayerAction(pa);
		END_PROFILER_EVENT();
	}
	
	// Count hits here
	if (action == PlayerActionType::Hit || action == PlayerActionType::Destroy) { m_hitCount++; }
}

void Session::accumulateFrameInfo(RealTime t, float sdt, float idt) {
	if (notNull(m_logger) && m_config->logger.logFrameInfo) {
		m_logger->logFrameInfo(FrameInfo(FPSciLogger::getFileTime(), sdt));
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
	return m_config->weapon.maxAmmo - m_shotCount;
}


float Session::getRemainingTrialTime() {
	if (isNull(m_config)) return 10.0;
	return m_config->timing.maxTrialDuration - m_timer.getTime();
}

float Session::getProgress() {
	if (notNull(m_config)) {
		float remainingTrials = 0.f;
		for (int tcount : m_remainingTrials) {
			if (tcount < 0) return 0.f;				// Infinite trials, never make any progress
			remainingTrials += (float)tcount;
		}
		return 1.f - (remainingTrials / m_config->getTrialsPerBlock());
	}
	return fnan();
}

int Session::getScore() {
	return (int)(10.0 * m_totalRemainingTime);
}

String Session::formatCommand(const String& input) {
	const char delimiter = '%';			///< Start of all valid substrings
	String formatted = input;			///< Output string
	int foundIdx = 0;					///< Index for searching for substrings

	const String loggerComPort = "%loggerComPort";
	const String syncComPort = "%loggerSyncComPort";
	const String dbFilename = "%dbFilename";

	while ((foundIdx = (int)formatted.find(delimiter, (size_t)foundIdx)) > -1) {
		if (!formatted.compare(foundIdx, loggerComPort.length(), loggerComPort)) {
			if (m_app->systemConfig.loggerComPort.empty()) {
				throw "Found \"%loggerComPort\" substring in a command, but no \"loggerComPort\" is provided in the config!";
			}
			formatted = formatted.substr(0, foundIdx) + m_app->systemConfig.loggerComPort + formatted.substr(foundIdx + loggerComPort.length());
		}
		else if (!formatted.compare(foundIdx, syncComPort.length(), syncComPort)) {
			if (m_app->systemConfig.syncComPort.empty()) {
				throw "Found \"%loggerSyncComPort\" substring in a command, but no \"loggerSyncComPort\" is provided in the config!";
			}
			formatted = formatted.substr(0, foundIdx) + m_app->systemConfig.syncComPort + formatted.substr(foundIdx + syncComPort.length());
		}
		else if (!formatted.compare(foundIdx, dbFilename.length(), dbFilename)) {
			if (m_dbFilename.empty()) {
				throw "No database filename found to support the %dbFilename substring!";
			}
			formatted = formatted.substr(0, foundIdx) + m_dbFilename + formatted.substr(foundIdx + dbFilename.length());
		}
		else {
			foundIdx++;
		}
	}
	return formatted;
}

String Session::formatFeedback(const String& input) {
	const char delimiter = '%';			///< Start of all valid substrings
	String formatted = input;			///< Output string
	int foundIdx = 0;					///< Index for searching for substrings

	// Substrings for replacement
	const String totalTimeLeftS			= "%totalTimeLeftS";				///< Sum of time remaining over all completed trials
	const String lastBlock				= "%lastBlock";						///< The last (completed) block in this session
	const String currBlock				= "%currBlock";						///< The current block of the session (next block at end of session)
	const String totalBlocks			= "%totalBlocks";					///< The total blocks specified in the session
	const String trialTaskTimeMs		= "%trialTaskTimeMs";				///< The time spent in the task state of this trial (in ms)
	const String trialTargetsDestroyed	= "%trialTargetsDestroyed";			///< The number of targets destroyed in this trial
	const String trialTotalTargets		= "%trialTotalTargets";				///< The number of targets in this trial ("infinite" if any target respawns infinitely)
	const String trialShotsHit			= "%trialShotsHit";					///< The number of shots hit in this trial
	const String trialTotalShots		= "%trialTotalShots";				///< The number of shots taken in this trial

	// Walk through the string looking for instances of the delimiter
	while ((foundIdx = (int)formatted.find(delimiter, (size_t)foundIdx)) > -1) {
		if(!formatted.compare(foundIdx, totalTimeLeftS.length(), totalTimeLeftS)){
			formatted = formatted.substr(0, foundIdx) + format("%.2f", m_totalRemainingTime) + formatted.substr(foundIdx + totalTimeLeftS.length());
		}
		else if (!formatted.compare(foundIdx, lastBlock.length(), lastBlock)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_currBlock - 1) + formatted.substr(foundIdx + lastBlock.length());
		}
		else if (!formatted.compare(foundIdx, currBlock.length(), currBlock)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_currBlock) + formatted.substr(foundIdx + currBlock.length());
		}
		else if (!formatted.compare(foundIdx, totalBlocks.length(), totalBlocks)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_config->blockCount) + formatted.substr(foundIdx + totalBlocks.length());
		}
		else if (!formatted.compare(foundIdx, trialTaskTimeMs.length(), trialTaskTimeMs)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", (int)(m_taskExecutionTime * 1000)) + formatted.substr(foundIdx + trialTaskTimeMs.length());
		}
		else if (!formatted.compare(foundIdx, trialTargetsDestroyed.length(), trialTargetsDestroyed)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_destroyedTargets) + formatted.substr(foundIdx + trialTargetsDestroyed.length());
		}
		else if (!formatted.compare(foundIdx, trialTotalTargets.length(), trialTotalTargets)) {
			String totalTargetsString;
			int totalTargets = totalTrialTargets();
			if (totalTargets > 0) {
				// Finite target count
				totalTargetsString += format("%d", totalTargets);
			}
			else { 
				// Inifinite target count case
				totalTargetsString += "infinite";
			}
			formatted = formatted.substr(0, foundIdx) + totalTargetsString + formatted.substr(foundIdx + trialTotalTargets.length());
		}
		else if (!formatted.compare(foundIdx, trialShotsHit.length(), trialShotsHit)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_hitCount) + formatted.substr(foundIdx + trialShotsHit.length());
		}
		else if (!formatted.compare(foundIdx, trialTotalShots.length(), trialTotalShots)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_shotCount) + formatted.substr(foundIdx + trialTotalShots.length());
		}
		else {
			// Bump the found index past this character (not a valid substring)
			foundIdx++;
		}
	}
	return formatted;
}

String Session::getFeedbackMessage() {
	return m_feedbackMessage;
}

void Session::endLogging() {
	if (m_logger != nullptr) {
		m_logger->logUserConfig(*m_app->currentUser(), m_config->id, "end", m_config->player.turnScale);
		m_logger->flush(false);
		m_logger.reset();
	}
}

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
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);

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
	const int scaleIndex = clamp(iRound(log(size) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
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
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
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
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
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

void Session::destroyTarget(shared_ptr<TargetEntity> target) {
	// Remove target from the scene
	m_scene->removeEntity(target->name());
	// Remove target from master list
	for (int i = 0; i < m_targetArray.size(); i++) {
		if (m_targetArray[i]->name() == target->name()) { m_targetArray.fastRemove(i); }
	}
	// Remove target from (un)hittable array
	for (int i = 0; i < m_hittableTargets.size(); i++) {
		if (m_hittableTargets[i]->name() == target->name()) { 
			m_hittableTargets.fastRemove(i); 
			return;	// Target can't be both hittable and unhittable
		}
	}
	for (int i = 0; i < m_unhittableTargets.size(); i++) {
		if (m_unhittableTargets[i]->name() == target->name()) { m_unhittableTargets.fastRemove(i); }
	}

}

/** Clear all targets one by one */
void Session::clearTargets() {
	for(auto target : m_targetArray) {
		destroyTarget(target);
	}
}
