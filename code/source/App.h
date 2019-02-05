/**
  \file maxPerf/App.h

  Sample application showing how to render simple graphics with maximum throughput and 
  minimum latency by stripping away most high level VFX and convenience features for
  development. This approach is good for some display and perception research. For general
  game and rendering applications, look at the G3D starter app and vrStarter which give very
  performance with a lot of high-level game engine features.

 */
#pragma once
#include <G3D/G3D.h>
#include "experiment.h"
#include "ExperimentSettingsList.h"

// An enum that tracks presentation state within a trial. Duration defined in experiment.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { ready, task, feedback , complete };

class Projectile {
public:
	shared_ptr<VisibleEntity>       entity;
	/** When in hitscan mode */
	RealTime                        endTime;
	Projectile() : endTime(0) {}
	Projectile(const shared_ptr<VisibleEntity>& e, RealTime t = 0) : entity(e), endTime(t) {}
};

class App : public GApp {
protected:
	static const float TARGET_MODEL_ARRAY_SCALING;
	const int                       numReticles = 55;

	shared_ptr<GFont>               m_outputFont;
	shared_ptr<GFont>               m_hudFont;
	shared_ptr<Texture>             m_reticleTexture;
	shared_ptr<Texture>             m_hudTexture;
	shared_ptr<ArticulatedModel>    m_viewModel;
	shared_ptr<Sound>               m_fireSound;
	shared_ptr<Sound>               m_explosionSound;

	shared_ptr<ArticulatedModel>    m_laserModel;

	/** m_targetModelArray[10] is the base size. Away from that they get larger/smaller by TARGET_MODEL_ARRAY_SCALING */
	Array<shared_ptr<ArticulatedModel>>  m_targetModelArray;

	/** Array of all targets in the scene */
	Array<shared_ptr<VisibleEntity>> m_targetArray;

	Array<Projectile>               m_projectileArray;

	/** Coordinate frame of the weapon, updated in onPose() */
	CFrame                          m_weaponFrame;
	CFrame                          m_motionFrame; // object at 10 m away in -z direction in this coordinate frame

	int                             m_displayLagFrames = 0;

	/** Used to detect GUI changes to m_reticleIndex */
	int                             m_lastReticleLoaded = -1;
	int                             m_reticleIndex = 0;
	float                           m_sceneBrightness = 1.0f;
	bool                            m_renderViewModel = true;
	bool                            m_renderHud = false;
	bool                            m_renderFPS = true;
	bool                            m_renderHitscan = false;

	/** Projectile if false         */
	bool                            m_hitScan = true;

	int                             m_lastUniqueID = 0;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
	Array<shared_ptr<Framebuffer>>  m_ldrDelayBufferQueue;
	int                             m_currentDelayBufferIndex = 0;

	/** Called from onInit */
	void makeGUI();
	void loadModels();
	void destroyTarget(int index);

public:

	App(const GApp::Settings& settings = GApp::Settings());

	/** Call to change the reticle. */
	void setReticle(int r);

	/** Increment the current reticle index */
	void nextReticle() {
		setReticle((m_reticleIndex + 1) % numReticles);
	}

	shared_ptr<VisibleEntity> spawnTarget(const Point3& position, float scale);

	/** Call to set the 3D scene brightness. Default is 1.0. */
	void setSceneBrightness(float b);

	void setDisplayLatencyFrames(int f);

	int displayLatencyFrames() const {
		return m_displayLagFrames;
	}

	virtual void onInit() override;
	virtual void onAI() override;
	virtual void onNetwork() override;
	virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
	virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
	virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;
	virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;
	virtual bool onEvent(const GEvent& e) override;
	virtual void onUserInput(UserInput* ui) override;
	virtual void onCleanup() override;

    const float                     m_targetDistance = 30.0f;
    const float                     m_projectileSpeed = 150.0f; // meters per second
	const float                     m_projectileShotPeriod = 0.3f; // minimum time between two repeated shots
    const float                     m_projectileSize = 0.5f;
	
	// hardware setting
	struct ScreenSetting
	{
		float viewingDistance = 0.5f; // in m
		float screenDiagonal = 25.0f * 0.0254f; // in m (diagonal)
		Vector2 resolution = Vector2(1920, 1080);
		float pixelSize = screenDiagonal / sqrt(resolution.x * resolution.x + resolution.y * resolution.y);
		Vector2 screenSize = resolution * pixelSize;
	} m_screenSetting;
	enum PresentationState          m_presentationState; // which sequence are we in?
	float                           m_targetHealth; // 1 if never hit, 0 if hit. Binary for instant hit weapon, but tracking weapon will continuously reduce it.
	Color3                          m_tunnelColor;
	Color3                          m_targetColor;
	Color3                          m_reticleColor;
	bool                            m_isTrackingOn; // true if down AND weapon type is tracking, false otherwise.
    double                          m_mouseDPI = 2400.0; // normal mice are 800.0. Gaming mice go up to 12000.0. Josef's G502 is set to 2400.0
    double                          m_cmp360 = 12.75; // Joohwan set this to ~12.75, Josef prefers ~9.25
    ExperimentSettingsList          m_experimentSettingsList;

protected:
	double                          m_t_lastAnimationUpdate;
	double                          m_t_stateStart;
	double                          m_t_lastProjectileShot = -inf();

public:
	Psychophysics::EccentricityExperiment ex;

	void initTrialAnimation();
	void resetView();
	void processUserInput(const GEvent& e);
	void initPsychophysicsLib();

	void updateAnimation(RealTime framePeriod);
	void informTrialSuccess();
	void informTrialFailure();
	void updateTrialState();
};

// The 'old' way of animation
/** Parameters related to animation during a trial. */
// Animation flow.
// updateAnimation() is called at the beginning of onGraphics3D. Workflow in updateLocation()
//       1. Check if trial time passed m_trialParam.trialDuration.
//                 If yes, end the current trial and initialize the next trial.
//       2. Check if motionChange is required.
//                 If yes, update m_rotationAxis. This is chosen among the vectors orthogonal to [camera - target] line.
//       3. update target location.
//       4. append to m_TargetArray an object with m_targetLocation.
