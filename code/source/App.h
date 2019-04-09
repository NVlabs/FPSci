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
#include "ExperimentConfig.h"
#include "Experiment.h"
#include "TargetExperiment.h"
#include "ReactionExperiment.h"
#include "ComPortDriver.h"
#include <chrono>

// TODO: This has to be replaced with G3D timer.
class Timer
{
public:
	std::chrono::steady_clock::time_point startTime;
	void startTimer() { startTime = std::chrono::steady_clock::now(); };
	float getTime()
	{
		auto now = std::chrono::steady_clock::now();
		int t = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(now - startTime).count();
		return ((float)t) / 1000.0f;
	};
};

class TargetEntity;

// An enum that tracks presentation state within a trial. Duration defined in experiment.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { initial, ready, task, feedback , complete };

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
	static const float TARGET_MODEL_ARRAY_OFFSET;
	/** Length of the history queue for m_frameDurationQueue */
	static const int MAX_HISTORY_TIMING_FRAMES = 360;
	const int                       numReticles = 55;

	//shared_ptr<GFont>               m_outputFont;
	//shared_ptr<GFont>               m_hudFont;
	//shared_ptr<Texture>             m_reticleTexture;
	//shared_ptr<Texture>             m_hudTexture;
	shared_ptr<ArticulatedModel>    m_viewModel;
	shared_ptr<Sound>               m_fireSound;
	shared_ptr<Sound>               m_explosionSound;

	shared_ptr<ArticulatedModel>    m_laserModel;
	shared_ptr<ArticulatedModel>	m_decalModel;
	shared_ptr<VisibleEntity>		m_lastDecal;
	shared_ptr<VisibleEntity>		m_firstDecal;
	shared_ptr<ArticulatedModel>	m_explosionModel;
	shared_ptr<VisibleEntity>		m_explosion;
	RealTime						m_explosionEndTime;

	GuiDropDownList*				m_sessDropDown;

	/** m_targetModelArray[10] is the base size. Away from that they get larger/smaller by TARGET_MODEL_ARRAY_SCALING */
	Array<shared_ptr<ArticulatedModel>>  m_targetModelArray;

	/** Used for visualizing history of frame times. Temporary, awaiting a G3D built-in that does this directly with a texture. */
	Queue<float>                    m_frameDurationQueue;

	/** Coordinate frame of the weapon, updated in onPose() */
	CFrame                          m_weaponFrame;
	int                             m_displayLagFrames = 0;

	/** Used to detect GUI changes to m_reticleIndex */
	int                             m_lastReticleLoaded = -1;
	int                             m_reticleIndex = 41;
	float                           m_sceneBrightness = 1.0f;
	bool                            m_renderViewModel = false;
	//bool                            m_renderHud = false;
	bool                            m_renderFPS = false;
	bool                            m_renderHitscan = false;

	/** Set to true to lower rendering quality to increase performance. */
	//bool                              m_emergencyTurbo = false;

	/** Projectile if false         */
	bool                            m_hitScan = true;

	int                             m_lastUniqueID = 0;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
	Array<shared_ptr<Framebuffer>>  m_ldrDelayBufferQueue;
	int                             m_currentDelayBufferIndex = 0;

    shared_ptr<GuiWindow>           m_userSettingsWindow;
    bool                            m_userSettingsMode = true;

	/** Called from onInit */
	void makeGUI();
	void loadModels();
	void destroyTarget(int index);

	CComPortDriver m_com;

public:
	/* Moving from proctected so that Experiment classes can use it. */
	shared_ptr<GFont>               m_outputFont;
	shared_ptr<GFont>               m_hudFont;
	shared_ptr<Texture>             m_reticleTexture;
	shared_ptr<Texture>             m_hudTexture;
	bool                            m_renderHud = false;
	bool                            m_emergencyTurbo = false;


	App(const GApp::Settings& settings = GApp::Settings());

	/** Array of all targets in the scene */
	Array<shared_ptr<VisibleEntity>> m_targetArray;
	Array<Projectile>               m_projectileArray;

	/** Parameter configurations */
	UserConfig                      m_user;
	ExperimentConfig                m_experimentConfig;

	//TODO: Remove it when we are using G3D timer
	Timer timer;

	/** Pointer to Experiment class */
	shared_ptr<Experiment> m_ex;

	/** To control target motion */
	CFrame                          m_motionFrame; // object at 10 m away in -z direction in this coordinate frame

	/** Call to change the reticle. */
	void setReticle(int r);

	/** Increment the current reticle index */
	void nextReticle() {
		setReticle((m_reticleIndex + 1) % numReticles);
	}

    /** Creates a random target with motion based on parameters 
    @param motionDuration time in seconds to produce a motion path for
    @param motionDecisionPeriod time in seconds when new motion direction is chosen
    @param speed world-space velocity (m/s) of target
    @param radius world-space distance to target
    @param scale size of target TODO: is this radius or diameter in meters?*/
    void spawnParameterizedRandomTarget(float motionDuration, float motionDecisionPeriod, float speed, float radius, float scale);

	/** Creates a random target in front of the player */
	void spawnRandomTarget();

	/** Creates a spinning target */
	shared_ptr<TargetEntity> spawnTarget(const Point3& position, float scale, bool spinLeft = true, const Color3& color = Color3::red());

	/** Call to set the 3D scene brightness. Default is 1.0. */
	void setSceneBrightness(float b);

	void userSaveButtonPress(void);

	Array<String> updateSessionDropDown(void);

	String getCurrentSessionId(void);

	void updateSessionPress(void);

	void updateSession(String id);

	void setDisplayLatencyFrames(int f);
    
	SystemConfig getSystemInfo(void);

	/** reads current user settings to update sensitivity in the controller */
    void updateMouseSensitivity();

	/** Fire the weapon - hits targets, draws decals, starts explosions */
	void fire();

	/** clear all targets (used when clearing remaining targets at the end of a trial) */
	void clearTargets();

	int displayLatencyFrames() const {
		return m_displayLagFrames;
	}
	
	
	virtual void onPostProcessHDR3DEffects(RenderDevice *rd) override;

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

	// variables for experiments
    const float                     m_targetDistance = 1.0f;
    const float                     m_spawnDistance = 0.0f;
    const float                     m_projectileSpeed = 0.0f; // meters per second
	const float                     m_projectileShotPeriod = 0.3f; // minimum time between two repeated shots
    const float                     m_projectileSize = 0.5f;

    //Attempts to bound target within visible space
    //const float                     m_yawBound = 5.0f;
	
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
	Color3                          m_targetColor = Color3::red();
	Color3                          m_reticleColor;
	// TODO: m_targetHealth is only relevant to TargetExperiment.
	float						    m_targetHealth; // 1 if never hit, 0 if hit. Binary for instant hit weapon, but tracking weapon will continuously reduce it.
	bool                            m_isTrackingOn; // true if down AND weapon type is tracking, false otherwise.


protected:
	double                          m_t_lastAnimationUpdate;
	double                          m_t_stateStart;
	double                          m_t_lastProjectileShot = -inf();

public:
	bool							m_buttonUp = true;

	void resetView();
	Point2 getViewDirection();
	Point2 getTargetDirection();
	Point3 getTargetPosition();
	Point2 getMouseMotion(); // TODO: how do we do this?
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
