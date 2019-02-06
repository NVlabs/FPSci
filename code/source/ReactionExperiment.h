#pragma once
#include <G3D/G3D.h>
#include "psychophysics.h"

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
			double readyDuration; // 'be ready' duration in sec.
			double meanWaitDuration; // mean wait duration in sec.
			float frameRate;

			ConditionParams()
			{
				trialCount = 10;
				intensities = { 0.2f, 1.0f };
				readyDuration = 0.5;
				meanWaitDuration = 0.0;
				frameRate = 240.f;
			}
		} m_conditionParams;

	public:
		/////// RENDER-RELEVANT PARAMS ///////
		struct RRP
		{
			// these will be updated per trial and sent to the shader
			double intensity;
			double readyDuration;
			double meanWaitDuration;
			std::string sceneType;
			float frameRate;
		};
		RRP renderParams;

		std::string expName = "ReactionExperiment";

		/////// REQUIRED FUNCTIONS ///////
		void init(std::string subjectID, std::string expVersion, int sessionNum, std::string savePath, bool trainingMode) override;

		FSM::State getFSMState() override { return fsm->currState; };
		bool isExperimentDone();

		void startTimer() override { fsm->startTimer(); };
		float getTime() { int t = fsm->elapsedTimeMS(); return ((float)t) / 1000.0f; };

		void printDebugInfo();

		std::string getDebugStr();

		void updateRenderParamsForCurrentTrial();
	};
}