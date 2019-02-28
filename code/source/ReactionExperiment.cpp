#include "ReactionExperiment.h"
#include "App.h"

namespace AbstractFPS
{
	void ReactionExperiment::initPsychHelper()
	{

	}

	void ReactionExperiment::onInit() {
		// Add conditions.
	}

	void ReactionExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
	{
		// The following was in the older version.
		// They do not work when executed here, but keeping it as record.
		// To be deleted when everything is confirmed to work correctly without it.

		//if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
		//	swapBuffers();
		//}
		//return;
	}

	void ReactionExperiment::updatePresentationState(RealTime framePeriod)
	{
		// This updates presentation state and also deals with data collection when each trial ends.
		PresentationState currentState = m_app->m_presentationState;
		PresentationState newState;
		double stateElapsedTime = (double)m_app->timer.getTime();

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
			float taskStartChancePerFrame = (1.0f / (float)renderParams.meanWaitDuration) * (float)framePeriod;
			if ((stateElapsedTime > renderParams.minimumForeperiod) && (G3D::Random::common().uniform() < taskStartChancePerFrame))
			{
				newState = PresentationState::task;
			}
			else if (m_reacted) // stimulus not shown yet, but responded already -> an immediate trial failure.
			{
				m_app->timer.startTimer(); // starting timer so that we get unrealistically small number for failed trials.
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
				if (stateElapsedTime > 0.1) {
					m_app->informTrialSuccess();
					if (trainingMode) {
						m_feedbackMessage = std::to_string(int(stateElapsedTime * 1000)) + " msec";
					}
					else {
						m_feedbackMessage = "Success!";
					}
				}
				else {
					m_app->informTrialFailure();
					m_feedbackMessage = "Failure: Responded too quickly.";
				}
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
			m_app->timer.startTimer();
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
			m_stimColor = Color3::red() * m_psych.getConditionParam["intensity"];
		}
		else if (m_app->m_presentationState == PresentationState::task) {
			m_stimColor = Color3::green() * m_psych.getConditionParam["intensity"];
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

	}

	void ReactionExperiment::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D)
	{

	}

	void ReactionExperiment::createResultFile()
	{

	}

	void ReactionExperiment::recordTrialResponse()
	{

	}

	void ReactionExperiment::closeResultFile()
	{

	}
}

