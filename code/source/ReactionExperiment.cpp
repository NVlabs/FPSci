#include "ReactionExperiment.h"
#include "App.h"

namespace AbstractFPS
{
	void ReactionExperiment::onInit() {
		// Perform the three steps of starting a psychophysics experiment instance: describe, add conditions, init.
		// Step 1. Describe.
		m_psych.clear();
		AbstractFPS::Descriptor desc;
		desc.mDescList.insert(std::pair<std::string, std::string>("experimentName","ReactionExperiment"));
		// TODO: There must be a simpler way.
		desc.mDescList.insert(std::pair<std::string, std::string>("expMode", m_app->m_experimentConfig.expMode.c_str()));
		desc.mDescList.insert(std::pair<std::string, std::string>("subjectID", m_app->m_user.subjectID.c_str()));
		desc.mDescList.insert(std::pair<std::string, std::string>("taskType", m_app->m_experimentConfig.taskType.c_str()));
		// expVersion not needed
		// sceneName not needed
		desc.mDescList.insert(std::pair<std::string, std::string>("appendingDescription", m_app->m_experimentConfig.appendingDescription.c_str()));
		desc.mParamList.insert(std::pair<std::string, float>("targetFrameRate", m_app->m_experimentConfig.targetFrameRate));
		m_psych.describeExperiment(desc);

		// Step 2. Add conditions.
	}

	void ReactionExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
	{

	}

	void ReactionExperiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
	{

	}

	void ReactionExperiment::onUserInput(UserInput* ui)
	{

	}

	void ReactionExperiment::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D)
	{

	}
}

