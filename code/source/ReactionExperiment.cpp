#include "ReactionExperiment.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <map>

bool ReactionExperiment::initPsychHelper()
{
	// Add conditions, one per one intensity.
	// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
	shared_ptr<SessionConfig> sess = m_config.getSessionConfigById(m_app->getCurrSessId());
	if (sess == nullptr) return false;
	Array<Param> params = m_config.getReactionExpConditions(sess->id);
	for (auto p : params) {
		// Define properties of psychophysical methods
		PsychophysicsDesignParameter psychParam;
		psychParam.mMeasuringMethod = PsychophysicsMethod::MethodOfConstantStimuli;
		psychParam.mIsDefault = false;
		psychParam.mStimLevels.push_back(m_config.taskDuration);		// Shorter task is more difficult. However, we are currently doing unlimited time.
		psychParam.mMaxTrialCounts.push_back((int)p.val["trialCount"]);
		p.add("session", m_app->getCurrSessId().c_str());
		m_psych.addCondition(p, psychParam);
	}

	// Update the logger w/ these conditions (IS THIS THE RIGHT PLACE TO DO THIS???)
	m_app->m_logger->addConditions(m_psych.mMeasurements);

	// call it once all conditions are defined.
	m_psych.chooseNextCondition();
	return true;
}

void ReactionExperiment::onInit() {
	// initialize presentation states
	m_app->m_presentationState = PresentationState::initial;
	m_feedbackMessage = "Reaction speed test. Click on green!";

	m_config = m_app->m_experimentConfig;				// Get configuration
	m_hasSession = initPsychHelper();					// Initialize PsychHelper based on the configuration.
	if (!m_hasSession) {								// Check for invalid session (nothing to do)
		m_app->m_presentationState = PresentationState::feedback;
	}
}

void ReactionExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
	// The following was in the older version.
	// They do not work when executed here (submitToDisplayMode is not public), but keeping it as record.
	// To be deleted when everything is confirmed to work correctly without it.

	//if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
	//	swapBuffers();
	//}
	//return;
}

void ReactionExperiment::processResponse()
{
	m_taskExecutionTime = m_app->timer.getTime();
	m_taskEndTime = Logger::genUniqueTimestamp();
	if (m_app->m_presentationState == PresentationState::ready) {
		if (m_reacted) {
			// responded too quickly
			m_response = 0; // 1 means success, 0 means failure.
			m_feedbackMessage = "Failure: Responded too quickly.";
		}
	}
	else if (m_app->m_presentationState == PresentationState::task) {
		if (m_taskExecutionTime < 0.1) { // still rejecting because response was too fast (impossible)
			m_response = 0; // 1 means success, 0 means failure.
			m_feedbackMessage = "Failure: Responded too quickly.";
		}
		else {
			m_response = 1; // 1 means success, 0 means failure.
			/*if (m_app->m_expConfig.expMode == "training") {
				m_feedbackMessage = format("%d ms", (int)(m_taskExecutionTime * 1000));
			}
			else {
				m_feedbackMessage = "Success!";
			}*/
		}
	}

	recordTrialResponse(); // NOTE: we need to record response first before processing it with PsychHelper.
	m_psych.processResponse(m_response); // process response.
}

void ReactionExperiment::updatePresentationState(RealTime framePeriod)
{
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState currentState = m_app->m_presentationState;
	PresentationState newState;
	float stateElapsedTime = m_app->timer.getTime();

	if (currentState == PresentationState::initial)
	{
		if (m_reacted)
		{
			m_feedbackMessage = "";
			newState = PresentationState::feedback;
		}
		else { // keep waiting.
			newState = currentState;
		}
	}
	else if (currentState == PresentationState::ready)
	{
		// start task if waited longer than minimum foreperiod AND the probabilistic condition is met (Nickerson & Burhnham 1969, Response times with nonaging foreperiods).
		float taskStartChancePerFrame = (1.0f / m_config.readyDuration) * (float)framePeriod;
		if ((stateElapsedTime > m_psych.getParam().val["minimumForeperiod"]) && (G3D::Random::common().uniform() < taskStartChancePerFrame))
		{
			newState = PresentationState::task;
		}
		else if (m_reacted) // stimulus not shown yet, but responded already -> an immediate trial failure.
		{
			processResponse();
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
			processResponse();
			newState = PresentationState::feedback;
		}
		else newState = currentState;
	}
	else if (currentState == PresentationState::feedback)
	{
		if (stateElapsedTime > m_config.feedbackDuration)
		{
			m_reacted = false;
			if (m_psych.isComplete()) {
				m_feedbackMessage = "Session complete. Thanks!";
				newState = PresentationState::complete;
				if (m_hasSession) {
					m_app->getCurrUser()->completedSessions.append(String(m_psych.getParam().str["session"]));			// Add this session to user's completed sessions
					m_app->userSaveButtonPress();
					Array<String> remaining = m_app->updateSessionDropDown();
					if (remaining.size() > 0) {
						String nextSess = remaining.randomElement();				// Choose a random next session
						m_app->updateSession(nextSess);								// Update the session
					}
					else m_feedbackMessage = "All Sessions Complete!";
				}
				else m_feedbackMessage = "All Sessions Complete!";
			}
			else {
				m_feedbackMessage = "";
				m_psych.chooseNextCondition();
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
		m_app->timer.startTimer();
		if (newState == PresentationState::task) {
			m_taskStartTime = Logger::genUniqueTimestamp();
		}
		m_app->m_presentationState = newState;
	}
}

void ReactionExperiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
{
	// 1. Update presentation state and send task performance to psychophysics library.
	updatePresentationState(rdt);

	// 2. Assign the background color for 2D graphics
	if (m_app->m_presentationState == PresentationState::initial) {
		m_stimColor = Color3::white() * 0.3f;
	}
	else if (m_app->m_presentationState == PresentationState::ready) {
		m_stimColor = Color3::red() * m_psych.getParam().val["intensity"];
	}
	else if (m_app->m_presentationState == PresentationState::task) {
		m_stimColor = Color3::green() * m_psych.getParam().val["intensity"];
	}
	else if (m_app->m_presentationState == PresentationState::feedback) {
		m_stimColor = Color3::white() * 0.3f;
	}
	else if (m_app->m_presentationState == PresentationState::complete) {
		m_stimColor = Color3::white() * 0.2f;
	}
}

void ReactionExperiment::onUserInput(UserInput* ui)
{
	// insert response and uncomment below. 
	if (ui->keyPressed(GKey::LEFT_MOUSE)) {
		m_reacted = true;
	}
}

void ReactionExperiment::onGraphics2D(RenderDevice* rd)
{
	rd->clear();
	const float scale = rd->viewport().width() / 1920.0f;
	Draw::rect2D(rd->viewport(), rd, m_stimColor);

	if (!m_feedbackMessage.empty()) {
		m_app->m_outputFont->draw2D(rd, m_feedbackMessage.c_str(),
			(Point2((float)m_app->window()->width() / 2 - 40, (float)m_app->window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
	}
}

void ReactionExperiment::recordTrialResponse()
{
	String sess = String(m_psych.mMeasurements[m_psych.mCurrentConditionIndex].getParam().str["session"]);

	std::vector<std::string> trialValues = {
		std::to_string(m_psych.mCurrentConditionIndex),
		addQuotes(sess.c_str()),
		addQuotes(m_config.getSessionConfigById(sess)->expMode.c_str()),
		addQuotes(m_taskStartTime),
		addQuotes(m_taskEndTime),
		std::to_string(m_taskExecutionTime),
	};
	m_app->m_logger->recordTrialResponse(trialValues);
}