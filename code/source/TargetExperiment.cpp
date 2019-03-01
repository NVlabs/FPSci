#include "TargetExperiment.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <map>

namespace AbstractFPS
{
	void TargetExperiment::initPsychHelper()
	{
		// Define properties of psychophysical methods
		AbstractFPS::PsychophysicsDesignParameter psychParam;
		psychParam.mMeasuringMethod = AbstractFPS::PsychophysicsMethod::MethodOfConstantStimuli;
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
			p.add("targetFrameLag", m_app->m_experimentConfig.targetFrameLag);
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
		m_app->m_experimentConfig.visualSize = 0.02;
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
			m_app->m_experimentConfig.trialCount = m_app->m_experimentConfig.trialCount / 3;
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


	void TargetExperiment::updatePresentationState()
	{
		// This updates presentation state and also deals with data collection when each trial ends.
		PresentationState currentState = m_app->m_presentationState;
		PresentationState newState;
		double stateElapsedTime = (double)m_app->timer.getTime();

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
				newState = PresentationState::task;
			}
		}
		else if (currentState == PresentationState::task)
		{
			if ((stateElapsedTime > m_app->m_experimentConfig.taskDuration) || (m_app->m_targetHealth <= 0))
			{
				m_taskExecutionTime = stateElapsedTime;
				m_response = (m_app->m_targetHealth <= 0)? 1 : 0; // 1 means success, 0 means failure.
				recordTrialResponse(); // NOTE: we need record response first before processing it with PsychHelper.
				m_psych.processResponse(m_response); // process response.
				if (m_app->m_experimentConfig.expMode == "training") {
					m_feedbackMessage = format("%d ms!", (int)(m_taskExecutionTime * 1000));
				}
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
			m_app->m_presentationState = newState;
			//If we switched to task, call initTargetAnimation to handle new trial
			if ((newState == PresentationState::task) || (newState == PresentationState::feedback)) {
				initTargetAnimation();
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

		// 4. Update tunnel and target colors
		if (m_app->m_presentationState == PresentationState::ready)
		{
			if (m_app->m_targetHealth > 0)
			{
				m_app->m_targetColor = Color3::red().pow(2.0f);
			}
			else
			{
				m_app->m_targetColor = Color3::green().pow(2.0f);
				// If the target is dead, empty the projectiles
				m_app->m_projectileArray.fastClear();
			}
		}
		else if (m_app->m_presentationState == PresentationState::task)
		{
			m_app->m_targetColor = m_app->m_targetHealth * Color3::cyan().pow(2.0f) + (1.0f - m_app->m_targetHealth) * Color3::brown().pow(2.0f);
		}
		else if (m_app->m_presentationState == PresentationState::feedback)
		{
			if (m_app->m_targetHealth > 0)
			{
				m_app->m_targetColor = Color3::green().pow(2.0f);
			}
			else
			{
				m_app->m_targetColor = Color3::green().pow(2.0f);
				// If the target is dead, empty the projectiles
				m_app->m_projectileArray.fastClear();
			}
		}
		else if (m_app->m_presentationState == PresentationState::complete) {
			m_app->drawMessage("Experiment Completed. Thanks!");
		}

		// 5. Clear m_TargetArray. Append an object with m_targetLocation if necessary ('task' and 'feedback' states).
		Point3 t_pos = m_app->m_motionFrame.pointToWorldSpace(Point3(0, 0, -m_app->m_targetDistance));


		if (m_app->m_targetHealth > 0.f) {
			// Don't spawn a new target every frame
			if (m_app->m_targetArray.size() == 0) {
				m_app->spawnTarget(t_pos, m_app->m_experimentConfig.visualSize);
			}
			else {
				// TODO: don't hardcode assumption of a single target
				m_app->m_targetArray[0]->setFrame(t_pos);
			}
		}
	}

	void TargetExperiment::onUserInput(UserInput* ui)
	{
		// insert response and uncomment below. 
		if (ui->keyPressed(GKey::LEFT_MOUSE)) {
			m_app->fire();
		}
	}

	void TargetExperiment::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D)
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

	void TargetExperiment::recordTrialResponse()
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

	void TargetExperiment::closeResultFile()
	{
		// TODO: Decide whether we need this function.
	}
}
