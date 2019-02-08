#pragma once
#include <G3D/G3D.h>
#include "App.h"
#include "psychophysics.h"

class App;

namespace Psychophysics
{
	class ReactionExperiment : public Experiment
	{

	protected:
		virtual void updateHelper(const std::string& keyInput, const FSM::State& pastState) override;

		// experimental setting
		class ConditionParams
		{
		public:
			int trialCount; // how many trials per condition?
			std::vector<double> intensities;  // normalized intensity from 0 to 1
			double minimumForeperiod; // minimum foreperiod in sec.
			double meanWaitDuration; // mean wait duration after the minimum foreperiod in sec.
			double feedbackDuration;
			float frameRate;

			ConditionParams()
			{
				trialCount = 20;
				intensities = { 0.4f, 1.0f };
				minimumForeperiod = 1.5;
				meanWaitDuration = 0.5;
				feedbackDuration = 1.0;
			}
		} m_conditionParams;

		App* m_app;
		//shared_ptr<App> m_app;

	public:
		ReactionExperiment(App* app) : Experiment() {
			expName = "ReactionExperiment";
			m_app = app;
		};

		/////// RENDER-RELEVANT PARAMS ///////
		struct RRP
		{
			// these will be updated per trial and sent to the shader
			double intensity;
			double minimumForeperiod;
			double meanWaitDuration;
			double feedbackDuration;
			std::string sceneType;
			float frameRate;
		};
		RRP renderParams;

		/////// Experiment-related variables ///////
		bool m_reacted = false;
		Color3 m_stimColor = Color3::white();

		/////// REQUIRED FUNCTIONS ///////
		void init(std::string subjectID, std::string expVersion, int sessionNum, std::string savePath, bool trainingMode) override;

		FSM::State getFSMState() override { return fsm->currState; };

		void startTimer() override { fsm->startTimer(); };
		float getTime() override { int t = fsm->elapsedTimeMS(); return ((float)t) / 1000.0f; };

		void printDebugInfo();

		std::string getDebugStr();

		void updateRenderParamsForCurrentTrial();

		/////// ANIMATION AND PRESENTATION STATE HANDLING ///////
		std::string m_feedbackMessage = "";

		void updatePresentationState(RealTime framePeriod);

		void updateAnimation(RealTime framePeriod);

		void setFrameRate(float fr);
	};
}