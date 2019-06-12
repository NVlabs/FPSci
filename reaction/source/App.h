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
#include "Logger.h"
#include <chrono>

// An enum that tracks presentation state within a trial. Duration defined in experiment.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { initial, ready, task, feedback, scoreboard, complete };

class App : public GApp {
protected:
	static const float TARGET_MODEL_ARRAY_SCALING;
	static const float TARGET_MODEL_ARRAY_OFFSET;
	/** Length of the history queue for m_frameDurationQueue */
	static const int MAX_HISTORY_TIMING_FRAMES = 360;

	shared_ptr<Sound>               m_fireSound;

	const int m_MatTableSize = 10;							///< Set this to set # of color "levels"
	Array<shared_ptr<UniversalMaterial>>	m_materials;	///< This stores the color materials

	GuiDropDownList*				m_sessDropDown;
	GuiDropDownList*				m_userDropDown;

	/** Used for visualizing history of frame times. Temporary, awaiting a G3D built-in that does this directly with a texture. */
	Queue<float>                    m_frameDurationQueue;

	int                             m_displayLagFrames = 0;

	/** Used to detect GUI changes */
	float                           m_sceneBrightness = 1.0f;
	//bool                            m_renderViewModel = false;
	//bool                            m_renderHud = false;
	bool                            m_renderFPS = false;
    GuiPane*                        m_currentUserPane;

	// Drop down selection writebacks
	int								m_ddCurrentUser = 0;
	int								m_lastSeenUser = -1;
	int								m_ddCurrentSession = 0;

	/** Set to true to lower rendering quality to increase performance. */
	//bool                              m_emergencyTurbo = false;

	/** Projectile if false         */
	bool                            m_hitScan = true;

	int                             m_lastUniqueID = 0;

	bool							m_sceneLoaded = false;				// Indicates whether or not the scene has been loaded (prevents reload)
	bool							m_loggerRunning = false;
	HANDLE							m_loggerHandle = 0;
	String							m_logName;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
	Array<shared_ptr<Framebuffer>>  m_ldrDelayBufferQueue;
	int                             m_currentDelayBufferIndex = 0;

    shared_ptr<GuiWindow>           m_userSettingsWindow;
    bool                            m_userSettingsMode = false;

	/** Called from onInit */
	void makeGUI();
	void printExpConfigToLog(ExperimentConfig config);
	void printUserTableToLog(UserTable table);
	void printUserStatusTableToLog(UserStatusTable table);
	void updateUser(void);
    void updateUserGUI();

public:
	/* Moving from proctected so that Experiment classes can use it. */
	shared_ptr<GFont>               m_outputFont;
	shared_ptr<GFont>               m_hudFont;
	shared_ptr<Texture>             m_hudTexture;
	bool                            m_renderHud = false;
	bool                            m_emergencyTurbo = false;
	double m_weaponStrength = 1.0; // Strength of each shot. 1 will instantly destroy the target (total target health is 1).

	App(const GApp::Settings& settings = GApp::Settings());

	/** Parameter configurations */
	UserTable						userTable;					// Table of per user information (DPI/cm/360) that doesn't change across experiment
	UserStatusTable					userStatusTable;			// Table of user status (session ordering/completed sessions) that do change across experiments
	ExperimentConfig                experimentConfig;			// Configuration for the experiment and its sessions

	/** Pointer to Experiment class */
	shared_ptr<Experiment> ex;

	/** Pointer to Logger class */
	shared_ptr<Logger> logger;

	/** Call to set the 3D scene brightness. Default is 1.0. */
	void setSceneBrightness(float b);
    /** callback for saving user config */
	void userSaveButtonPress(void);
	/** used by experiment class to finish data logging */
	void mergeCurrentLogToCurrentDB();

    Array<String> updateSessionDropDown(void);
    String getDropDownSessId(void);
	String getDropDownUserId(void);

	void markSessComplete(String id);

	shared_ptr<UserConfig> getCurrUser(void);

	void updateSessionPress(void);

	void updateSession(String id);

	void runPythonLogger(String logName, String com, bool hasSync, String syncComPort);

	void killPythonLogger();
    void quitRequest();

	bool pythonMergeLogs(String basename);

	void setDisplayLatencyFrames(int f);
    
	/** opens the user settings window */
    void openUserSettingsWindow();

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
    virtual void oneFrame() override;
	
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
	//Point2 getMouseMotion(); // TODO: how do we do this?
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
