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
#include "ConfigFiles.h"
#include "TargetEntity.h"
#include "GuiElements.h"
#include "PyLogger.h"

struct State {
	Array<shared_ptr<Surface>> surfaceArray;
	Array<shared_ptr<Surface2D>> surface2DArray;
    Array<shared_ptr<TargetEntity>> targets;
    Array<CFrame> targetFrames;
};

class FlyingEntity;
class JumpingEntity;

struct LatencyConfig {

    /** When the player fires, this defines what mouse position is used. */
    G3D_DECLARE_ENUM_CLASS(HitDetectionInput,
        /** Always chooses the most recent player position and rotation, even if the client hasn't got a frame to show it.
         * There may be a disconnect between crosshair and hit detection. */
        LATEST,

        /** Always match hit detection with what is seen on-screen. */
        VISUAL
    );
    
    /** When the server gets the fire event (ray start/direction), against which time in the game state history does it perform the hit scan. */
    enum HitDetectionTime {
		/** The server rolls back time and performs hit detection matching
		 * the game state to the time the player fires. This will be newer than
		 * the visual timestamp so again, the player needs to lead their shots. */
		HIT_TIMESTAMP_CLICK = 0,

		/** The server rolls back time and performs hit detection matching
         * the game state to the timestamp of the image displayed when the player
         * fires. */
        HIT_TIMESTAMP_VISUAL = 1,
    
        /** The server performs hit detection with its current game state even
         though the player shot some time ago. The player will need to lead
         their shots a lot! */
        // HIT_TIMESTAMP_ONPACKET,
    };

    enum LatewarpMethod {
        /** Present old frames. There is no late-warp or input latency compensation. */
        LATEWARP_NONE,

        /** Warp rotation only */
        LATEWARP_ROTATION,

        /** Warp both rotation and translation */
        LATEWARP_FULLTRANSFORM,
    };

    enum LatencySimulation {
        /** No artificial latency. */
        // LATENCY_DELAY_NONE

        /** We render into a frame queue and do an image based late-warp when displaying the delayed frame */
        LATENCY_DELAY_FRAMES,

        /** We render using new camera input but delayed scene objects */
        LATENCY_DELAY_POSES,
    };

	String getHitDetectionInputShortName() const {
		return hitDetectionInput == HitDetectionInput::VISUAL ? "inputvisual" : "inputlatest";
	}

	String getHitDetectionTimeShortName() const {
		return hitDetectionTime == HIT_TIMESTAMP_VISUAL ? "timevisual" : "timeclick";
	}

	String getLatewarpMethodShortName() const {
		switch (latewarpMethod) {
		case LATEWARP_NONE: return "nowarp";
		case LATEWARP_ROTATION: return "warprot";
		case LATEWARP_FULLTRANSFORM: return "warp6dof";
		default: return "invalidwarp";
		}
	}

	String getLatencySimulationShortName() const {
		return latencySimulation == LATENCY_DELAY_FRAMES ? "frameq" : "poseq";
	}

	String getConfigShortName() const {
		return format("%s_%s_%s_%s",
			getLatewarpMethodShortName().c_str(),
			getLatencySimulationShortName().c_str(),
			getHitDetectionTimeShortName().c_str(),
			getHitDetectionInputShortName().c_str());
	}

    HitDetectionInput     hitDetectionInput;
    HitDetectionTime      hitDetectionTime;
    LatewarpMethod        latewarpMethod;
    LatencySimulation     latencySimulation;
    
    RealTime              delayClientDatacenter;
    RealTime              delayDatacenterGameserver;

    LatencyConfig() :
        hitDetectionInput(HitDetectionInput::VISUAL),
        hitDetectionTime(HIT_TIMESTAMP_CLICK),
		//latewarpMethod(LATEWARP_ROTATION),
		latewarpMethod(LATEWARP_NONE),
		latencySimulation(LATENCY_DELAY_FRAMES),

        // TODO: these should come from SessionConfig
        delayClientDatacenter(0.1f),
        delayDatacenterGameserver(0.04f)
    {
    }
	
    LatencyConfig(
		HitDetectionInput initHitDetectionInput,
		HitDetectionTime initHitDetectionTime,
		LatewarpMethod initLatewarpMethod,
		LatencySimulation initLatencySimulation
		) :
		hitDetectionInput(initHitDetectionInput),
		hitDetectionTime(initHitDetectionTime),
		latewarpMethod(initLatewarpMethod),
		latencySimulation(initLatencySimulation)
	{
		// TODO: these should come from SessionConfig
		delayClientDatacenter = 0.1f;
		delayDatacenterGameserver = 0.04f;
	}
};

class Session;
class G3Dialog;
class WaypointManager;

// An enum that tracks presentation state within a trial. Duration defined in session.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { initial, ready, task, feedback, scoreboard, complete };

class Projectile {
public:
	shared_ptr<VisibleEntity>       entity;
	/** When in hitscan mode */
	RealTime                        endTime;
	Projectile() : endTime(0) {}
	Projectile(const shared_ptr<VisibleEntity>& e, RealTime t = 0) : entity(e), endTime(t) {}
};

class FloatingCombatText : public VisibleEntity {
protected:
	shared_ptr<GFont> m_font;
	String m_text;
	float m_size;
	Color4 m_color;
	Color4 m_outline;
	Point3 m_offset;
	Point3 m_velocity;
	float m_fade;
	float m_timeout;
	RealTime m_created;

public:
	static shared_ptr<FloatingCombatText> create(String text, shared_ptr<GFont> font, float size, Color4 color, Color4 outlineColor, Point3 offset, Point3 velocity, float fade, float timeout_s) {
		return  createShared<FloatingCombatText>(text, font, size, color, outlineColor, offset, velocity, fade, timeout_s);
	}
	FloatingCombatText(String text, shared_ptr<GFont> font, float size, Color4 color, Color4 outlineColor, Point3 offset, Point3 velocity, float fade, float timeout_s) {
		m_text = text;
		m_font = font;
		m_size = size;
		m_color = color;
		m_outline = outlineColor;
		m_offset = offset;
		m_velocity = velocity;
		m_fade = fade;
		m_timeout = timeout_s;
		m_created = System::time();		// Capture the time at which this was created
	}

	bool draw(RenderDevice* rd, const Camera& camera, const Framebuffer& framebuffer) {
		// Abort if the timeout has expired (return false to remove this combat text from the tracked array)
		float time_existing = static_cast<float>(System::time() - m_created);
		if (time_existing > m_timeout) {
			return false;
		}

		// Project entity position into image space
		Rect2D viewport = Rect2D(framebuffer.vector2Bounds());
		Point3 position = camera.project(frame().translation, viewport);

		// Abort if the target is not in front of the camera 
		Vector3 diffVector = frame().translation - camera.frame().translation;
		if (camera.frame().lookRay().direction().dot(diffVector) < 0.0f) {
			return true;
		}
		// Abort if the target is not in the view frustum
		if (position == Point3::inf()) {
			return true;
		}
		position += m_offset;						// Apply (initial) offset in pixels
		position += time_existing * m_velocity;		// Update the position based on velocity

		// Apply the fade
		m_color.a *= m_fade;						
		m_outline.a *= m_fade;

		// Draw the font
		m_font->draw2D(rd, m_text, position.xy(), m_size, m_color, m_outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		return true;
	}
};

class App : public GApp {
protected:
	static const float TARGET_MODEL_ARRAY_SCALING;						///< Target model scale factor
	static const float TARGET_MODEL_ARRAY_OFFSET;						///< Target model offset
	static const int MAX_HISTORY_TIMING_FRAMES = 360;					///< Length of the history queue for m_frameDurationQueue

	shared_ptr<ArticulatedModel>    m_viewModel;						///< Model for the weapon
	shared_ptr<Sound>               m_fireSound;						///< Sound for weapon firing
	shared_ptr<Sound>               m_sceneHitSound;					///< Sound for target exploding

	shared_ptr<GFont>				m_combatFont;						///< Font used for floating combat text
	Array<shared_ptr<FloatingCombatText>>	m_combatTextList;			///< Array of existing combat text

	shared_ptr<ArticulatedModel>    m_bulletModel;						///< Model for the "bullet"
	shared_ptr<ArticulatedModel>	m_decalModel;						///< Model for the miss decal
	shared_ptr<VisibleEntity>		m_lastDecal;						///< Model for the last decal we created
	shared_ptr<VisibleEntity>		m_firstDecal;						///< Model for the first decal we created
	shared_ptr<VisibleEntity>		m_explosion;						///< Model for target destroyed decal
	RealTime						m_explosionEndTime;					///< Time for end of explosion
		
	const int m_MatTableSize = 10;										///< Set this to set # of color "levels"
	Array<shared_ptr<UniversalMaterial>>	m_materials;				///< This stores the color materials

	GuiDropDownList*				m_sessDropDown;						///< Dropdown menu for session selection
	GuiDropDownList*				m_userDropDown;						///< Dropdown menu for user selection
	GuiLabel*						m_mouseDPILabel;					///< Label for mouse DPI field
	GuiLabel*						m_cm360Label;						///< Label for cm/360 field

    Queue<State>                    m_stateQueue;
    State                           m_delayedGameState;
    int                             m_poseLagCount = 0;

    shared_ptr<PlayerControls>		m_playerControls;
	shared_ptr<RenderControls>		m_renderControls;
	shared_ptr<WeaponControls>		m_weaponControls;

	Table<String, Array<shared_ptr<ArticulatedModel>>> m_targetModels;
	const int m_modelScaleCount = 30;

	Array<shared_ptr<ArticulatedModel>> m_explosionModels;

	/** Used for visualizing history of frame times. Temporary, awaiting a G3D built-in that does this directly with a texture. */
	Queue<float>                    m_frameDurationQueue;				///< Queue for history of frrame times

	/** Coordinate frame of the weapon, updated in onPose() */
	CFrame                          m_weaponFrame;						///< Frame for the weapon
	int                             m_simulationLagFrames = 0;			///< Count of frames of latency to add
	int                             m_displayLagFrames = 0;				///< Count of rendered frames to delay (depends on simulation method)

	/** Used to detect GUI changes to m_reticleIndex */
	int                             m_lastReticleLoaded = -1;			///< Last loaded reticle (used for change detection)
	float							m_debugMenuHeight = 0.0f;			///< Height of the debug menu when in developer mode
    GuiPane*                        m_currentUserPane;					///< Current user information pane

	// Drop down selection writebacks
	int								m_ddCurrentUser = 0;				///< Index of current user
	int								m_lastSeenUser = -1;				///< Index of last seen user (used for change determination)
	int								m_ddCurrentSession = 0;				///< Index of current session

	/** Projectile if false         */
	bool                            m_hitScan = true;					// NOTE: Projectile mode has not been implemented

	RealTime						m_lastJumpTime = 0.0f;				///< Time of last jump

	int                             m_lastUniqueID = 0;					///< Counter for creating unique names for various entities
	bool							m_sceneLoaded = false;				///< Indicates whether or not the scene has been loaded (prevents reload)
	
	/** Late-warp demo UI */
	Array<String> m_warpCompensation = { "None", "Rotation", "Full Transform" };
	int m_selectedWarpCompensation = 0;
	Array<String> m_warpMethodList = { "Image Operation", "Virtual Camera Steering" };
	int m_selectedMethod = 0;
	Array<String> m_inputTimingList = { "Latest", "On Visual" };
	int m_selectedInputTiming = 0;
	Array<String> m_gameStateTimingList = { "On Click", "On Visual" };
	int m_selectedGameStateTiming = 0;
	bool m_showSOL = false;

	///** Late-warp demo UI-related functions */
	void configureLateWarp(); // TODO: ideally a callback from UI should handle this

	// Hardware logger specific fields
	bool							m_loggerRunning = false;			///< Flag to indicate whether a python logger (for HW logger) is running (i.e. needs to be closed)
	HANDLE							m_loggerHandle = 0;					///< Process handle for the python logger instance (for HW logger) if running
	String							m_logName;							///< The log name used by the python logger instance (for HW logger) if running
	String							m_loadedScene = "";
	String							m_defaultScene = "FPSci Simple Hallway";	// Default scene to load

	shared_ptr<PythonLogger>		m_pyLogger = nullptr;

	/** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
    struct DelayedFrame {
        shared_ptr<Framebuffer> fb;
        CFrame view;
		bool mouseState;
    };
	Array<DelayedFrame>				m_ldrDelayBufferQueue;
	Queue<bool>						m_mouseStatusDelayQueue;
	bool                            m_mouseStateDelayed;
	int                             m_currentDelayBufferIndex = 0;

    /** Parameters for how we simulate and compensate for latency. */
    LatencyConfig                   m_latencyConfig;
    
    bool                            m_latencyVisualizeEnable = false;
    shared_ptr<Framebuffer>         m_latencyVisualizeFB;
    shared_ptr<GBuffer>             m_latencyVisualizeGbuffer;

    shared_ptr<GuiWindow>           m_userSettingsWindow;
    bool                            m_userSettingsMode = true;

	/** Called from onInit */
	void makeGUI();
	void updateControls();
	void loadModels();
	void loadDecals();
	void updateUser(void);
    void updateUserGUI();

	void drawHUD(RenderDevice *rd);
	void drawClickIndicator(RenderDevice *rd, bool mouseStatus, String mode);

public:

	static StartupConfig startupConfig;

	/* Moving from proctected so that Experiment classes can use it. */
	shared_ptr<GFont>               outputFont;						///< Font used for output
	shared_ptr<GFont>               hudFont;						///< Font used in HUD
	Array<shared_ptr<GFont>>		floatingCombatText;				///< Floating combat text array
	shared_ptr<Texture>             reticleTexture;					///< Texture used for reticle
	shared_ptr<Texture>             hudTexture;						///< Texture used for HUD
	shared_ptr<GuiTheme>			theme;	
	bool                            emergencyTurbo = false;			///< Lower rendering quality to improve performance

	App(const GApp::Settings& settings = GApp::Settings());
	
	/** For automated testing, the app will run for a set number of frames and exit. */
	bool m_testMode = false;
	bool m_dumpNextFrame = false;
	Ray m_lastFireGamestateRay;
	Ray m_lastFireVisualRay;
	String m_frameDumpFilename;
	bool testModeRequested();
	void dumpNextFrame(String filename);

	/** Array of all targets in the scene */
	Array<shared_ptr<TargetEntity>> targetArray;					///< Array of drawn targets
	Array<Projectile>               projectileArray;				///< Arrray of drawn projectiles

	int destroyedTargets = 0;										///< Number of targets destroyed

	/** Parameter configurations */
	UserTable						userTable;						///< Table of per user information (DPI/cm/360) that doesn't change across experiment
	UserStatusTable					userStatusTable;				///< Table of user status (session ordering/completed sessions) that do change across experiments
	ExperimentConfig                experimentConfig;				///< Configuration for the experiment and its sessions
	KeyMapping						keyMap;
	shared_ptr<WaypointManager>		waypointManager;				///< Waypoint mananger pointer
	
	shared_ptr<SessionConfig>		sessConfig = SessionConfig::create();			///< Current session config
	shared_ptr<G3Dialog>			dialog;							///< Dialog box

	shared_ptr<Session> sess;										///< Pointer to the experiment

	bool renderFPS = false;				///< Control flag used to draw (or not draw) FPS information to the display	
	float lastSetFrameRate = 0.0f;		///< Last set frame rate
	const int numReticles = 55;			///< Total count of reticles available to choose from
	float sceneBrightness = 1.0f;		///< Scene brightness scale factor

	/** Call to change the reticle. */
	void setReticle(int r);

    const LatencyConfig& latencyConfig() const {
        return m_latencyConfig;
    }

	/** Destroy a target from the targets array */
	void destroyTarget(int index);
	/** Show the player controls */
	void showPlayerControls();
	/** Show the render controls */
	void showRenderControls();
	/** Show the weapon controls */
	void showWeaponControls();
	/** Save scene w/ updated player position */
	void exportScene();
	/** Provide all latency parameters at once. NOTE: do not call this mid-frame. */
	void setLatewarpConfig(LatencyConfig config);

	/** Utility functions for late warp demo UI */
	int getFrameRate() {
		return int(roundf(1.0f / m_wallClockTargetDuration));
	}
	float getLatency() {
		return m_wallClockTargetDuration * (m_displayLagFrames + m_cameraDelayFrames);
	}
	String getLateWarpMethodName() {
		if (m_latencyConfig.latewarpMethod == LatencyConfig::LatewarpMethod::LATEWARP_NONE) {
			return "None";
		} else if (m_latencyConfig.latewarpMethod == LatencyConfig::LatewarpMethod::LATEWARP_ROTATION) {
			if (m_latencyConfig.latencySimulation == LatencyConfig::LatencySimulation::LATENCY_DELAY_FRAMES) {
				return "Image-based warp";
			} else if (m_latencyConfig.latencySimulation == LatencyConfig::LatencySimulation::LATENCY_DELAY_POSES) {
				return "Camera rotation";
			} else {
                debugAssert(false);
                return "Undefined condition";
            }
		} else {
            debugAssert(false);
			return "N/A";
		}
	}


	String getGameStateTiming() {
		if (m_latencyConfig.hitDetectionTime == LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_CLICK) {
			return "Latest state";
		}
		else if (m_latencyConfig.hitDetectionTime == LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_VISUAL) {
			return "As on screen";
		} else {
            debugAssert(false);
            return "Unknown condition";
        }
	}

	float debugMenuHeight() {
		return m_debugMenuHeight;
	}

    /** Creates a random target with motion based on parameters 
    @param motionDuration time in seconds to produce a motion path for
    @param motionDecisionPeriod time in seconds when new motion direction is chosen
    @param speed world-space velocity (m/s) of target
    @param radius world-space distance to target
    @param scale size of target TODO: is this radius or diameter in meters?*/
    void spawnParameterizedRandomTarget(float motionDuration, float motionDecisionPeriod, float speed, float radius, float scale);

	shared_ptr<TargetEntity> spawnDestTarget(const Point3 position, Array<Destination> dests, float scale, const Color3& color, String id, int paramIdx, int respawns = 0, String name="", bool isLogged=true);

	/** Creates a random target in front of the player */
	void spawnRandomTarget();

	/** Creates a spinning target */
	shared_ptr<FlyingEntity> spawnTarget(const Point3& position, float scale, bool spinLeft = true, const Color3& color = Color3::red(), String modelName= "model/target/target.obj");

	/** Creates a flying target */
	shared_ptr<FlyingEntity> spawnFlyingTarget(
		const Point3& position,
		float scale,
		const Color3& color,
		const Vector2& speedRange,
		const Vector2& motionChangePeriodRange,
		bool upperHemisphereOnly,
		Point3 orbitCenter,
		String modelName,
		int paramIdx,
		Array<bool> axisLock,
		int respawns = 0,
		String name = "",
		bool isLogged=true
	);

	/** Creates a jumping target */
	shared_ptr<JumpingEntity> spawnJumpingTarget(
		const Point3& position,
		float scale,
		const Color3& color,
        const Vector2& speedRange,
        const Vector2& motionChangePeriodRange,
        const Vector2& jumpPeriodRange,
		const Vector2& distanceRange,
		const Vector2& jumpSpeedRange,
		const Vector2& gravityRange,
		Point3 orbitCenter,
		float targetDistance,
		String modelName,
		int paramIdx,
		Array<bool> axisLock,
		int respawns = 0,
		String name = "",
		bool isLogged=true
	);


    /** callback for saving user config */
	void userSaveButtonPress(void);

	Array<String> updateSessionDropDown(void);
	String getDropDownSessId(void);
	void markSessComplete(String id);
	void updateSessionPress(void);
	void updateSession(const String& id);
	void updateParameters();
	void setFramerate(float frameRate);
	void setLatencyFrames(int frameDelay);
	void presentQuestion(Question question);

	String getDropDownUserId(void);
	shared_ptr<UserConfig> getCurrUser(void);

    void quitRequest();
	   
	/** opens the user settings window */
    void openUserSettingsWindow();

	/** reads current user settings to update sensitivity in the controller */
    void updateMouseSensitivity();

	/** Fire the weapon - hits targets, draws decals, starts explosions */
	shared_ptr<TargetEntity> fire(bool destroyImmediately=false);

	/** clear all targets (used when clearing remaining targets at the end of a trial) */
	void clearTargets();

	/** get the current view direction */
	Point2 getViewDirection();

	Point3 getPlayerLocation();
	
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

	bool							m_buttonUp = true;
	Point3	currentGamerPosition = Point3(0,0,0);
	float gamersHealth = 10000;
	bool gamerMoving = false;
	bool playerHitRender = false;
	float renderTheEffect = 0;
	void drawHittingEffect(RenderDevice *rd);
	int hittingRenderCount = 0;
	PresentationState previousState = PresentationState::task;

	Queue<CFrame> m_cameraPoseQueue;
	int m_cameraDelayFrames = 0;

    /** The camera pose m_cameraDelayFrames frames ago after going through m_cameraPoseQueue */
    CFrame m_delayedCameraFrame;

    /** In onGraphics3D this is set to the camera pose of the frame, whether late-warped or not. */
	CFrame m_displayedCameraFrame;
    
    /** Always contains the latest camera pose even if latency is being injected. Also disables effects. */
	shared_ptr<Camera> m_noLatencyCamera;
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
