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

void PsychHelper::addCondition(Param newConditionParam, PsychophysicsDesignParameter newPsychParam)
{
	SingleThresholdMeasurement m(newConditionParam, newPsychParam);
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

Param PsychHelper::getParam()
{
	return mMeasurements[mCurrentConditionIndex].getParam();
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
    // Add conditions, one per one intensity.
    // TODO: need the session config index
    shared_ptr<SessionConfig> sess = m_config.getSessionConfigById(m_app->getDropDownSessId());
    if (sess == nullptr) return false;
    Array<Param> params = m_config.getReactionExpConditions(sess->id);
    for (auto p : params) {
        // Define properties of psychophysical methods
        PsychophysicsDesignParameter psychParam;
        psychParam.mMeasuringMethod = PsychophysicsMethod::MethodOfConstantStimuli;
        psychParam.mIsDefault = false;
        psychParam.mStimLevels.push_back(m_config.taskDuration);		// Shorter task is more difficult. However, we are currently doing unlimited time.
        psychParam.mMaxTrialCounts.push_back((int)p.val["trialCount"]);
        m_psych.addCondition(p, psychParam);
    }

    // Update the logger w/ these conditions (IS THIS THE RIGHT PLACE TO DO THIS???)
    m_app->logger->addConditions(m_psych.mMeasurements);

    // call it once all conditions are defined.
    m_psych.chooseNextCondition();
    return true;
}

void Experiment::onInit() {
    // initialize presentation states
    m_app->m_presentationState = PresentationState::initial;
    m_feedbackMessage = "Reaction speed test. Click on green!";

    m_config = m_app->experimentConfig;				// Get configuration
    m_hasSession = initPsychHelper();					// Initialize PsychHelper based on the configuration.
    if (!m_hasSession) {								// Check for invalid session (nothing to do)
        m_app->m_presentationState = PresentationState::feedback;
    }
}

void Experiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
    // The following was in the older version.
    // They do not work when executed here (submitToDisplayMode is not public), but keeping it as record.
    // To be deleted when everything is confirmed to work correctly without it.

    //if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
    //	swapBuffers();
    //}
    //return;
}

void Experiment::processResponse()
{
    m_taskExecutionTime = timer.getTime();
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

void Experiment::updatePresentationState(RealTime framePeriod)
{
    // This updates presentation state and also deals with data collection when each trial ends.
    PresentationState currentState = m_app->m_presentationState;
    PresentationState newState;
    float stateElapsedTime = timer.getTime();

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
                    m_app->markSessComplete(String(m_psych.getParam().str["session"]));
                    m_app->userSaveButtonPress();
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
        timer.startTimer();
        if (newState == PresentationState::task) {
            m_taskStartTime = Logger::genUniqueTimestamp();
        }
        m_app->m_presentationState = newState;
    }
}

void Experiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
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

void Experiment::onUserInput(UserInput* ui)
{
    // insert response and uncomment below. 
    if (ui->keyPressed(GKey::LEFT_MOUSE)) {
        m_reacted = true;
    }
}

void Experiment::onGraphics2D(RenderDevice* rd)
{
    rd->clear();
    const float scale = rd->viewport().width() / 1920.0f;
    Draw::rect2D(rd->viewport(), rd, m_stimColor);

    if (!m_feedbackMessage.empty()) {
        m_app->m_outputFont->draw2D(rd, m_feedbackMessage.c_str(),
            (Point2((float)m_app->window()->width() / 2 - 40, (float)m_app->window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
    }
}

void Experiment::recordTrialResponse()
{
    String sess = String(m_psych.mMeasurements[m_psych.mCurrentConditionIndex].getParam().str["session"]);

    Array<String> trialValues = {
        String(m_psych.mCurrentConditionIndex),
        "'" + sess + "'",
        "'" + m_config.getSessionConfigById(sess)->expMode + "'",
        "'" + m_taskStartTime + "'",
        "'" + (m_taskEndTime)+"'",
        String(std::to_string(m_taskExecutionTime)),
    };
    m_app->logger->recordTrialResponse(trialValues);
}

