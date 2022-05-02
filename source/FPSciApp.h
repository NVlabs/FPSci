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

#include "ExperimentConfig.h"
#include "StartupConfig.h"
#include "KeyMapping.h"
#include "SystemConfig.h"
#include "SystemInfo.h"
#include "UserConfig.h"
#include "UserStatus.h"
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
enum PresentationState { initial, pretrial, trialTask, trialFeedback, sessionFeedback, complete };
static String presentationStateToString(const PresentationState& state) {
	String stateStr = "N/A";
	switch (state) {
	case initial: stateStr = "initial"; break;
	case pretrial: stateStr = "pretrial";  break;
	case trialTask: stateStr = "trialTask"; break;
	case trialFeedback: stateStr = "trialFeedback";  break;
	case sessionFeedback: stateStr = "sessionFeedback";  break;
	case complete: stateStr = "complete"; break;
	}
	return stateStr;
}

class FPSciApp : public GApp {
public:
	enum MouseInputMode {					///< Enumerated type for controlling the mouse input mode
		MOUSE_DISABLED = 0,					/// No mouse interaction
		MOUSE_CURSOR = 1,					/// Cursor-based interaction
		MOUSE_FPM = 2,						/// First-person manipulator-based interaction
	};

protected:
	static const int						MAX_HISTORY_TIMING_FRAMES = 360;	///< Length of the history queue for m_frameDurationQueue
	shared_ptr<Sound>						m_sceneHitSound;					///< Sound for scene collision
	shared_ptr<Sound>						m_refTargetHitSound;				///< Sound for hitting the reference target

	shared_ptr<GFont>						m_combatFont;						///< Font used for floating combat text
	Array<shared_ptr<FloatingCombatText>>	m_combatTextList;					///< Array of existing combat text

	Array<shared_ptr<VisibleEntity>>		m_explosions;						///< Model for target destroyed decal
	Array<RealTime>							m_explosionRemainingTimes;			///< Time for end of explosion
	int										m_explosionIdx = 0;					///< Explosion index
	const int								m_maxExplosions = 20;				///< Maximum number of simultaneous explosions
	
	Table<String, Array<shared_ptr<ArticulatedModel>>> m_explosionModels;
	/** table of shaders cached for the 2D shader parameters set per session */
	Table<String, shared_ptr<G3D::Shader>> m_shaderTable;

	/** Used for visualizing history of frame times. Temporary, awaiting a G3D built-in that does this directly with a texture. */
	Queue<float>							m_frameDurationQueue;				///< Queue for history of frame times

	/** Used to detect GUI changes to m_reticleIndex */
	int										m_lastReticleLoaded = -1;			///< Last loaded reticle (used for change detection)
	float									m_debugMenuHeight = 0.0f;			///< Height of the debug menu when in developer mode

	RealTime								m_lastJumpTime = 0.0f;				///< Time of last jump
public:
	RealTime								m_lastOnSimulationRealTime = 0.0f;	///< Wall clock time last onSimulation finished
	SimTime									m_lastOnSimulationSimTime = 0.0f;	///< Simulation time last onSimulation finished
	SimTime									m_lastOnSimulationIdealSimTime = 0.0f;	///< Ideal simulation time last onSimulation finished
protected:
	float									m_currentWeaponDamage = 0.0f;		///< A hack to avoid passing damage through callbacks

	int										m_lastUniqueID = 0;					///< Counter for creating unique names for various entities
	SceneConfig								m_loadedScene;						///< Configuration for loaded scene
	String									m_defaultSceneName = "FPSci Simple Hallway";	// Default scene to load

	String									m_expConfigHash;					///< String hash of experiment config file

	shared_ptr<PythonLogger>				m_pyLogger = nullptr;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
	Array<shared_ptr<Framebuffer>>			m_ldrDelayBufferQueue;
	int										m_currentDelayBufferIndex = 0;

    shared_ptr<UserMenu>					m_userSettingsWindow;				///< User settings window
	MouseInputMode							m_mouseInputMode = MouseInputMode::MOUSE_CURSOR;	///< Does the mouse currently have control over the view
	bool									m_showUserMenu = true;				///< Show the user menu after update?

	bool									m_firstSession = true;				///< Flag indicating that this is the first session run
	UserConfig								m_lastSavedUser;					///< Used to track if user has changed since last save

	bool									m_sceneHasPlayerEntity = false;		///< Flag indicating whether loaded scene has a PlayerEntity specified
	Table<String, CFrame>					m_initialCameraFrames;				///< Initial frames for all cameras in the scene

	shared_ptr<PlayerControls>				m_playerControls;					///< Player controls window (developer mode)
	shared_ptr<RenderControls>				m_renderControls;					///< Render controls window (developer mode)
	shared_ptr<WeaponControls>				m_weaponControls;					///< Weapon controls window (developer mode)


	// Shader buffers
	shared_ptr<Framebuffer>					m_ldrBuffer2D;						///< Buffer to use for 2D content (if split)
	shared_ptr<Framebuffer>					m_ldrShader2DOutput;				///< Buffer to use for 2D shader output (if provided)
	shared_ptr<Framebuffer>					m_hdrShader3DOutput;				///< Buffer to use for 3D shader output (if provided)
	shared_ptr<Framebuffer>					m_ldrBufferPrecomposite;			///< Buffer to blit LDR framebuffer into before converting to composite resolution
	shared_ptr<Framebuffer>					m_ldrBufferComposite;				///< Buffer to use for input to composited shader (if provided)
	shared_ptr<Framebuffer>					m_ldrShaderCompositeOutput;			///< Buffer to use for composite shader output (if provided)

	// Shader parameters
	int										m_frameNumber = 0;					///< Frame number (since the start of the session)
	RealTime								m_startTime;						///< Start time (for the session)
	RealTime								m_last2DTime, m_last3DTime, m_lastCompositeTime;		///< Times used for iTimeDelta

	/** Called from onInit */
	void makeGUI();
	void updateControls(bool firstSession = false);
	
	void loadConfigs(const ConfigFiles& configs);

	virtual void loadModels();
	
	/** Initializes player settings from configs and resets player to initial position 
		Also updates mouse sensitivity. */
	void initPlayer(bool firstSpawn = false);

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

	void hitTarget(shared_ptr<TargetEntity> target);
	void missEvent();

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

	ReticleConfig					reticleConfig;					///< Config for the active reticle
	shared_ptr<Texture>             reticleTexture;					///< Texture used for reticle
	
	Table<String, shared_ptr<Texture>> hudTextures;					///< Textures used for the HUD
	shared_ptr<GuiTheme>			theme;	

	FPSciApp(const GApp::Settings& settings = GApp::Settings());

	/** Parameter configurations */
	UserTable						userTable;						///< Table of per user information (DPI/cm/360) that doesn't change across experiment
	UserStatusTable					userStatusTable;				///< Table of user status (session ordering/completed sessions) that do change across experiments
	ExperimentConfig                experimentConfig;				///< Configuration for the experiment and its sessions
	SystemConfig					systemConfig;					///< System configuration
	KeyMapping						keyMap;
	shared_ptr<WaypointManager>		waypointManager;				///< Waypoint mananger pointer
	
	shared_ptr<SessionConfig>		sessConfig = SessionConfig::create();			///< Current session config
	shared_ptr<DialogBase>			dialog;							///< Dialog box
	Question						currentQuestion;				///< Currently presented question

	Table<String, Array<shared_ptr<ArticulatedModel>>>	targetModels;

	/** A table of sounds that targets can use to allow sounds to finish playing after they're destroyed */
	Table<String, shared_ptr<Sound>>					soundTable;

	/** A table of materials for models to use */
	Table<String, Array<shared_ptr<UniversalMaterial>>>	materials;
	const int											matTableSize = 13;	///< Set this to set # of color "levels"
	
	Array<shared_ptr<UniversalMaterial>> makeMaterials(shared_ptr<TargetConfig> tconfig);
	Color4 lerpColor(Array<Color4> colors, float a);

	shared_ptr<Session> sess;					///< Pointer to the experiment
	shared_ptr<Camera> playerCamera;			///< Pointer to the player camera						
	shared_ptr<Weapon> weapon;					///< Current weapon

	bool		renderFPS			= false;	///< Control flag used to draw (or not draw) FPS information to the display	
	int			displayLagFrames	= 0;		///< Count of frames of latency to add
	float		lastSetFrameRate	= 0.0f;		///< Last set frame rate
	const int	numReticles			= 55;		///< Total count of reticles available to choose from
	float		sceneBrightness		= 1.0f;		///< Scene brightness scale factor
	
	bool		shootButtonUp			= true;	///< Tracks shoot button state (used for click indicator)
	bool		shootButtonJustReleased = false;///< Tracks shoot button state (used for weapon timing)
	bool		shootButtonJustPressed = false;	///< Tracks shoot button state (used for weapon timing)

	bool		frameToggle			= false;	///< Simple toggle flag used for frame rate click-to-photon monitoring
	bool		updateUserMenu		= false;	///< Semaphore to indicate user settings needs update
	bool		reinitExperiment	= false;	///< Semaphore to indicate experiment needs to be reinitialized

	int			experimentIdx = 0;				///< Index of the current experiment

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

	const Array<String> experimentNames() const {
		Array<String> names;
		for (auto config : startupConfig.experimentList) { names.append(config.name); }
		return names;
	}

    /** callbacks for saving user status and config */
	void saveUserConfig(bool onDiff);
	void saveUserConfig() { saveUserConfig(false); }
	void saveUserStatus(void);

	// Pass throughts to user settings window (for now)
	Array<String> updateSessionDropDown(void) { return m_userSettingsWindow->updateSessionDropDown(); }
	shared_ptr<UserConfig> const currentUser(void) {  return userTable.getUserById(userStatusTable.currentUser); }

	void markSessComplete(String id);
	/** Updates experiment state to the provided session id and updates player parameters (including mouse sensitivity) */
	virtual void updateSession(const String& id, bool forceReload = false);
	void updateParameters(int frameDelay, float frameRate);
	void updateTargetColor(const shared_ptr<TargetEntity>& target);
	void presentQuestion(Question question);

    void quitRequest();
	void toggleUserSettingsMenu();
	   
	/** opens the user settings window */
    void openUserSettingsWindow();
	void closeUserSettingsWindow();

	/** changes the mouse interaction (camera direct vs pointer) */
	void setMouseInputMode(MouseInputMode mode = MOUSE_FPM);
	/** reads current user settings to update sensitivity in the controller */
    void updateMouseSensitivity();
	
	/** Initialize an experiment */
	void initExperiment();

	virtual void onInit() override;
	virtual void onAI() override;
	virtual void onNetwork() override;
	virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
	virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
	virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;
	virtual bool onEvent(const GEvent& e) override;
	virtual void onAfterEvents() override;
	virtual void onUserInput(UserInput* ui) override;
	virtual void onCleanup() override;
    virtual void oneFrame() override;

	// In FPSciGraphics.cpp
	virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
	virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;
	virtual void onPostProcessHDR3DEffects(RenderDevice* rd) override;
	
	void draw2DElements(RenderDevice* rd, Vector2 resolution);			///< Draw the undelayed 2D elements
	void drawDelayed2DElements(RenderDevice* rd, Vector2 resolution);	///< Draw the delayed 2D elements

	virtual void drawHUD(RenderDevice* rd, Vector2 resolution);						///< Draw HUD elements
	void drawClickIndicator(RenderDevice* rd, String mode, Vector2 resolution);		///< Draw the click-to-photon click indicator
	void updateFPSIndicator(RenderDevice* rd, Vector2 resolution);					///< Update and draw a (custom) frame time indicator (developer mode feature)
	void drawFeedbackMessage(RenderDevice* rd);										///< Draw a user feedback message (at full render device resolution)

	void updateShaderBuffers();									///< Regenerate buffers (for configured shaders)

	/** calls rd->pushState with the right delayed buffer. Creates buffers if needed */
	void pushRdStateWithDelay(RenderDevice* rd, Array<shared_ptr<Framebuffer>> &delayBufferQueue, int &delayIndex, int lagFrames = 0);
	/** calls rd->popState and advances the delayIndex. Copies the latest delay buffer into the current framebuffer */
	void popRdStateWithDelay(RenderDevice* rd, const Array<shared_ptr<Framebuffer>> &delayBufferQueue, int& delayIndex, int lagFrames = 0);

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
