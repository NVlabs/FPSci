#include "ReactionExperiment.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <map>

void ReactionExperiment::initPsychHelper()
{
	// Add conditions, one per one intensity.
	// TODO: This must smartly iterate for every combination of an arbitrary number of arrays.
	Array<Param> params = m_config.getReactionExpConditions();
	for (auto p : params) {
		// Define properties of psychophysical methods
		PsychophysicsDesignParameter psychParam;
		psychParam.mMeasuringMethod = PsychophysicsMethod::MethodOfConstantStimuli;
		psychParam.mIsDefault = false;
		psychParam.mStimLevels.push_back(m_config.taskDuration);		// Shorter task is more difficult. However, we are currently doing unlimited time.
		psychParam.mMaxTrialCounts.push_back(p.val["trialCount"]);		
		m_psych.addCondition(p, psychParam);
	}

	// call it once all conditions are defined.
	m_psych.chooseNextCondition();
}

void ReactionExperiment::onInit() {
	// initialize presentation states
	m_app->m_presentationState = PresentationState::initial;
	m_feedbackMessage = "Reaction speed test. Click on green!";

	// default values
	m_config = m_app->m_experimentConfig;
	// TODO: This should all move into configuration file.
	/*m_config.add("targetFrameRate", m_app->m_experimentConfig.sessions[m_app->m_user.currentSession].frameRate);
	m_config.add("targetFrameLag", (float)m_app->m_experimentConfig.sessions[m_app->m_user.currentSession].frameDelay);
	m_config.add("feedbackDuration", m_app->m_experimentConfig.feedbackDuration);
	m_config.add("meanWaitDuration", m_app->m_experimentConfig.readyDuration);
	m_config.add("taskDuration", m_app->m_experimentConfig.taskDuration);
	m_config.add("minimumForeperiod", m_app->m_experimentConfig.reactions[0].minimumForeperiod);
	m_config.add("trialCount", 20);
	m_config.add("intensities", (std::vector<float>)*m_app->m_experimentConfig.reactions[0].intensities.getCArray());*/

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

void ReactionExperiment::processResponse()
{
	m_taskExecutionTime = m_app->timer.getTime();
	m_taskEndTime = System::time();
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
		if (newState == PresentationState::task) {
			m_taskStartTime = System::time();
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

void ReactionExperiment::createResultFile()
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
			{ "intensity", "real" },
	};
	createTableInDB(m_db, "Conditions", conditionColumns); // Primary Key needed for this table.

	// populate table
	for (int i = 0; i < m_psych.mMeasurements.size(); ++i)
	{
		std::vector<std::string> conditionValues = {
			std::to_string(i), // this index is uniquely and statically assigned to each SingleThresholdMeasurement.
			std::to_string(m_psych.mMeasurements[i].getParam().val["targetFrameRate"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["targetFrameLag"]),
			std::to_string(m_psych.mMeasurements[i].getParam().val["intensity"]),
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
}

void ReactionExperiment::recordTrialResponse()
{
	std::vector<std::string> trialValues = {
		std::to_string(m_psych.mCurrentConditionIndex),
		std::to_string(m_taskStartTime),
		std::to_string(m_taskEndTime),
		std::to_string(m_taskExecutionTime),
	};
	insertRowIntoDB(m_db, "Trials", trialValues);
}

void ReactionExperiment::closeResultFile()
{
	sqlite3_close(m_db);
}

