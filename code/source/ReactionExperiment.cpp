#include "ReactionExperiment.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <map>

namespace AbstractFPS
{
	void ReactionExperiment::initPsychHelper()
	{
		// Define properties of psychophysical methods
		AbstractFPS::PsychophysicsDesignParameter psychParam;
		psychParam.mMeasuringMethod = AbstractFPS::PsychophysicsMethod::MethodOfConstantStimuli;
		psychParam.mIsDefault = false;
		psychParam.mStimLevels.push_back(m_app->m_experimentConfig.taskDuration); // Shorter task is more difficult. However, we are currently doing unlimited time.
		psychParam.mMaxTrialCounts.push_back(m_app->m_experimentConfig.trialCount);

		// Initialize PsychHelper.
		m_psych.clear(); // TODO: check whether necessary.

		// Add conditions, one per one intensity.
		// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
		for (auto i : m_app->m_experimentConfig.intensities)
		{
			// insert all the values potentially useful for analysis.
			Param p;
			p.add("intensity", i);
			p.add("targetFrameRate", m_app->m_experimentConfig.targetFrameRate);
			// soon we need to add frame delay as well.
			m_psych.addCondition(p, psychParam);
		}
	}

	void ReactionExperiment::onInit() {
		// initialize presentation states
		m_app->m_presentationState = PresentationState::initial;
		m_feedbackMessage = "Reaction speed test. Click on green!";

		// default values
		// TODO: This should all move into configuration file.
		m_app->m_experimentConfig.feedbackDuration = 1.0;
		m_app->m_experimentConfig.meanWaitDuration = 0.5;
		m_app->m_experimentConfig.taskDuration = 100000.0;
		m_app->m_experimentConfig.minimumForeperiod = 1.5;
		m_app->m_experimentConfig.trialCount = 3;
		m_app->m_experimentConfig.intensities.append(0.4);
		m_app->m_experimentConfig.intensities.append(1.0);

		if (m_app->m_experimentConfig.expMode == "training") { // shorter experiment if in training
			// NOTE: maybe not a good idea with dedicated subject.
			m_app->m_experimentConfig.trialCount = m_app->m_experimentConfig.trialCount / 3;
		}

		// initialize PsychHelper based on the configuration.
		initPsychHelper();

		// create the result file based on experimental configuration.
		createResultFile();
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
			float taskStartChancePerFrame = (1.0f / m_app->m_experimentConfig.meanWaitDuration) * (float)framePeriod;
			if ((stateElapsedTime > m_app->m_experimentConfig.minimumForeperiod) && (G3D::Random::common().uniform() < taskStartChancePerFrame))
			{
				newState = PresentationState::task;
			}
			else if (m_reacted) // stimulus not shown yet, but responded already -> an immediate trial failure.
			{
				m_taskExecutionTime = stateElapsedTime;
				m_response = 0; // 1 means success, 0 means failure.
				recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
				m_psych.processResponse(m_response); // process response.
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
					m_taskExecutionTime = stateElapsedTime;
					m_response = 1; // 1 means success, 0 means failure.
					recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
					m_psych.processResponse(m_response); // process response.
					if (m_app->m_experimentConfig.expMode == "training") {
						m_feedbackMessage = format("%d ms", (int)(m_taskExecutionTime * 1000));
					}
					else {
						m_feedbackMessage = "Success!";
					}
				}
				else {
					m_taskExecutionTime = stateElapsedTime;
					m_response = 0; // 1 means success, 0 means failure.
					recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
					m_psych.processResponse(m_response); // process response.
					m_feedbackMessage = "Failure: Responded too quickly.";
				}
				newState = PresentationState::feedback;
			}
			else newState = currentState;
		}
		else if (currentState == PresentationState::feedback)
		{
			if (stateElapsedTime > m_app->m_experimentConfig.feedbackDuration)
			{
				m_reacted = false;
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

	void ReactionExperiment::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D)
	{
		rd->clear();
		const float scale = rd->viewport().width() / 1920.0f;
		Draw::rect2D(rd->viewport(), rd, m_stimColor);

		if (!m_feedbackMessage.empty()) {
			m_app->m_outputFont->draw2D(rd, m_feedbackMessage.c_str(),
				(Point2((float)m_app->window()->width() / 2 - 40, (float)m_app->window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
		}
	}

	void ReactionExperiment::createResultFile()
	{
		// create a unique file name
		String timeStr(genUniqueTimestamp());
		mResultFileName = ("result_data/" + m_app->m_experimentConfig.taskType + "_" + m_app->m_user.subjectID + "_" + timeStr + ".csv").c_str(); // we may include subject name here.

		// create the file
		std::ofstream resultFile;
		resultFile.open(mResultFileName);

		// write column names
		resultFile << "staircaseID" << ",";
		for (auto keyval : m_psych.getParam().val)
		{
			resultFile << keyval.first << ",";
		}
		for (auto keyval : m_psych.getParam().str)
		{
			resultFile << keyval.first << ",";
		}
		//resultFile << "stimLevel" << ","; // normally we would need this but not now.
		resultFile << "response" << "," << "elapsedTime" << std::endl;

		// close the file
		resultFile.close();
	}

	void ReactionExperiment::recordTrialResponse()
	{
		// TODO: replace it with sqlite command later.
		std::ofstream resultFile(mResultFileName, std::ios_base::app);
		resultFile << m_psych.mCurrentConditionIndex << ",";
		for (auto keyval : m_psych.getParam().val)
		{
			resultFile << keyval.second << ",";
		}
		for (auto keyval : m_psych.getParam().str)
		{
			resultFile << keyval.second << ",";
		}
		//resultFile << m_psych.getStimLevel() << ","; // normally we would need this but not now.
		resultFile << m_response << "," << m_taskExecutionTime << std::endl;
		resultFile.close();
	}

	void ReactionExperiment::closeResultFile()
	{

	}
}

