/**
  \file maxPerf/FPSciApp.h

  Sample application showing how to render simple graphics with maximum throughput and 
  minimum latency by stripping away most high level VFX and convenience features for
  development. This approach is good for some display and perception research. For general
  game and rendering applications, look at the G3D starter app and vrStarter which give very
  performance with a lot of high-level game engine features.

 */
#pragma once
#include <G3D/G3D.h>
#include "ConfigFiles.h"
#include "TargetEntity.h"
#include "PlayerEntity.h"
#include "GuiElements.h"
#include "PythonLogger.h"
#include "Weapon.h"
#include "CombatText.h"

class Session;
class DialogBase;
class WaypointManager;

// An enum that tracks presentation state within a trial. Duration defined in Session.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { initial, ready, task, feedback, scoreboard, complete };

class FPSciApp : public GApp {
protected:
	static const int						MAX_HISTORY_TIMING_FRAMES = 360;	///< Length of the history queue for m_frameDurationQueue
	shared_ptr<Sound>						m_sceneHitSound;					///< Sound for target exploding

	shared_ptr<GFont>						m_combatFont;						///< Font used for floating combat text
	Array<shared_ptr<FloatingCombatText>>	m_combatTextList;					///< Array of existing combat text

	shared_ptr<Weapon>						m_weapon;							///< Current weapon

	Array<shared_ptr<VisibleEntity>>		m_explosions;						///< Model for target destroyed decal
	Array<RealTime>							m_explosionRemainingTimes;			///< Time for end of explosion
	int										m_explosionIdx = 0;					///< Explosion index
	const int								m_maxExplosions = 20;				///< Maximum number of simultaneous explosions
		
	const int								m_MatTableSize = 10;				///< Set this to set # of color "levels"
	Array<shared_ptr<UniversalMaterial>>	m_materials;						///< This stores the color materials

	Table<String, Array<shared_ptr<ArticulatedModel>>> m_explosionModels;

	/** Used for visualizing history of frame times. Temporary, awaiting a G3D built-in that does this directly with a texture. */
	Queue<float>							m_frameDurationQueue;				///< Queue for history of frrame times

	/** Used to detect GUI changes to m_reticleIndex */
	int										m_lastReticleLoaded = -1;			///< Last loaded reticle (used for change detection)
	float									m_debugMenuHeight = 0.0f;			///< Height of the debug menu when in developer mode

	RealTime								m_lastJumpTime = 0.0f;				///< Time of last jump

	int										m_lastUniqueID = 0;					///< Counter for creating unique names for various entities
	String									m_loadedScene = "";
	String									m_defaultScene = "FPSci Simple Hallway";	// Default scene to load

	shared_ptr<PythonLogger>				m_pyLogger = nullptr;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
	Array<shared_ptr<Framebuffer>>			m_ldrDelayBufferQueue;
	int										m_currentDelayBufferIndex = 0;

    shared_ptr<UserMenu>					m_userSettingsWindow;				///< User settings window
	bool									m_mouseDirectMode = true;			///< Does the mouse currently have control over the view
	bool									m_updateUserMenu = false;			///< Semaphore to indicate user settings needs update
	bool									m_showUserMenu = true;				///< Show the user menu after update?

	bool									m_firstSession = true;

	shared_ptr<PlayerControls>				m_playerControls;					///< Player controls window (developer mode)
	shared_ptr<RenderControls>				m_renderControls;					///< Render controls window (developer mode)
	shared_ptr<WeaponControls>				m_weaponControls;					///< Weapon controls window (developer mode)

	/** Called from onInit */
	void makeGUI();
	void updateControls(bool firstSession = false);
	virtual void loadModels();

	/** Move a window to the center of the display */
	void moveToCenter(shared_ptr<GuiWindow> window) {
		const float scale = 0.5f / m_userSettingsWindow->pixelScale();
		const Vector2 pos = (scale * renderDevice->viewport().wh()) - (window->bounds().wh() / 2.0f);
		window->moveTo(pos);
	}

	/** Get the current turn scale (per user and scope setting) */
	Vector2 currentTurnScale();
	/** Set the scoped view (and also adjust the turn scale), use setScopeView(!weapon->scoped()) to toggle scope */
	void setScopeView(bool scoped = true);

	void hitTarget(shared_ptr<TargetEntity>);
	void missEvent();

	virtual void drawHUD(RenderDevice *rd);
	void drawClickIndicator(RenderDevice *rd, String mode);

public:

	class Settings : public GApp::Settings {
	public:
		Settings(const StartupConfig& startupConfig, int argc, const char* argv[]);
	};

	/** global startup config - sets developer flags and experiment/user paths */
	static StartupConfig startupConfig;

	/* Moving from proctected so that Experiment classes can use it. */
	shared_ptr<GFont>               outputFont;						///< Font used for output
	shared_ptr<GFont>               hudFont;						///< Font used in HUD
	Array<shared_ptr<GFont>>		floatingCombatText;				///< Floating combat text array
	shared_ptr<Texture>             reticleTexture;					///< Texture used for reticle
	Table<String, shared_ptr<Texture>> hudTextures;					///< Textures used for the HUD
	shared_ptr<GuiTheme>			theme;	
	bool                            emergencyTurbo = false;			///< Lower rendering quality to improve performance

	FPSciApp(const GApp::Settings& settings = GApp::Settings());

	/** Parameter configurations */
	UserTable						userTable;						///< Table of per user information (DPI/cm/360) that doesn't change across experiment
	UserStatusTable					userStatusTable;				///< Table of user status (session ordering/completed sessions) that do change across experiments
	ExperimentConfig                experimentConfig;				///< Configuration for the experiment and its sessions
	LatencyLoggerConfig				latencyLoggerConfig;			///< Configuration for the system/hardware
	KeyMapping						keyMap;
	shared_ptr<WaypointManager>		waypointManager;				///< Waypoint mananger pointer
	
	shared_ptr<SessionConfig>		sessConfig = SessionConfig::create();			///< Current session config
	shared_ptr<DialogBase>			dialog;							///< Dialog box

	Table<String, Array<shared_ptr<ArticulatedModel>>>	targetModels;
	const int											modelScaleCount = 30;

	shared_ptr<Session> sess;										///< Pointer to the experiment

	bool renderFPS = false;				///< Control flag used to draw (or not draw) FPS information to the display	
	int  displayLagFrames = 0;			///< Count of frames of latency to add
	float lastSetFrameRate = 0.0f;		///< Last set frame rate
	const int numReticles = 55;			///< Total count of reticles available to choose from
	float sceneBrightness = 1.0f;		///< Scene brightness scale factor

	Vector2 displayRes;

	/** Call to change the reticle. */
	void setReticle(int r);
	/** Show the player controls */
	void showPlayerControls();
	/** Show the render controls */
	void showRenderControls();
	/** Show the weapon controls */
	void showWeaponControls();
	/** Save scene w/ updated player position */
	void exportScene();

	float debugMenuHeight() {
		return m_debugMenuHeight;
	}

    /** callbacks for saving user status and config */
	void saveUserConfig(void) {
		userTable.save(startupConfig.userConfig());
		logPrintf("User table saved.\n");			// Print message to log
	}
	void saveUserStatus(void) { 
		userStatusTable.save(startupConfig.userStatusConfig()); 
		logPrintf("User status saved.\n");
	}

	// Pass throughts to user settings window (for now)
	Array<String> updateSessionDropDown(void) { return m_userSettingsWindow->updateSessionDropDown(); }
	shared_ptr<UserConfig> const currentUser(void) {  return userTable.getUserById(userStatusTable.currentUser); }

	void markSessComplete(String id);
	virtual void updateSession(const String& id);
	void updateParameters(int frameDelay, float frameRate);
	void presentQuestion(Question question);

    void quitRequest();
	void toggleUserSettingsMenu();
	   
	/** opens the user settings window */
    void openUserSettingsWindow();
	void closeUserSettingsWindow();

	/** changes the mouse interaction (camera direct vs pointer) */
	void setDirectMode(bool enable = true);
	/** reads current user settings to update sensitivity in the controller */
    void updateMouseSensitivity();
	
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
	virtual void onAfterEvents() override;
	virtual void onUserInput(UserInput* ui) override;
	virtual void onCleanup() override;
    virtual void oneFrame() override;

	bool							m_buttonUp = true;
	bool							m_frameToggle = false;		///< Simple toggle flag used for frame rate click-to-photon monitoring
};

// The "old" way of animation
/** Parameters related to animation during a trial. */
// Animation flow.
// updateAnimation() is called at the beginning of onGraphics3D. Workflow in updateLocation()
//       1. Check if trial time passed m_trialParam.trialDuration.
//                 If yes, end the current trial and initialize the next trial.
//       2. Check if motionChange is required.
//                 If yes, update m_rotationAxis. This is chosen among the vectors orthogonal to [camera - target] line.
//       3. update target location.
//       4. append to m_TargetArray an object with m_targetLocation.
