#pragma once
#include <G3D/G3D.h>
#include "Weapon.h"
#include "GuiElements.h"
#include "FPSciAnyTableReader.h"

class SceneConfig {
public:

	String name;										///< Name of the scene to load
	String playerCamera;								///< Name of the camera to use for the player

	//float gravity = fnan();							///< Gravity for the PhysicsScene
	float resetHeight = fnan();							///< Reset height for the PhysicsScene

	Point3 spawnPosition = { fnan(),fnan(), fnan() };	///< Location for player spawn
	float spawnHeadingDeg = fnan();						///< Heading for player spawn (in degrees)

	SceneConfig() {}
	SceneConfig(const Any& any);

	Any toAny(const bool forceAll = false) const;
	bool operator!=(const SceneConfig& other) const;
};

class RenderConfig {
public:
	// Rendering parameters
	float           frameRate = 1000.0f;						///< Target (goal) frame rate (in Hz)
	int             frameDelay = 0;								///< Integer frame delay (in frames)
	Array<float>	frameTimeArray = { };						///< Array of target frame times (in seconds)
	bool			frameTimeRandomize = false;					///< Whether to choose a sequential or random item from frameTimeArray
	String			frameTimeMode = "always";					///< Mode to use for frame time selection (can be "always", "taskOnly", or "restartWithTask", case insensitive)

	float           hFoV = 103.0f;							    ///< Field of view (horizontal) for the user
	
	Array<int>		resolution2D = { 0, 0 };					///< Optional 2D buffer resolution
	Array<int>		resolution3D = { 0, 0 };					///< Optional 3D buffer resolution
	Array<int>		resolutionComposite = { 0, 0 };				///< Optional composite buffer resolution	

	String			shader2D = "";								///< Option for the filename of a custom shader to run on 2D content only
	String          shader3D = "";								///< Option for the filename of a custom shader to run on 3D content only
	String			shaderComposite = "";						///< Option for the filename of a custom shader to run on the (final) composited 2D/3D content	float           hFoV = 103.0f;							    ///< Field of view (horizontal) for the user
	
	// Samplers only exist between buffers with (possibly) different resolution, all equal sized buffers use Sampler::buffer()
	Sampler			sampler2D = Sampler::video();				///< Sampler for sampling the shader2D iChannel0 input
	Sampler			sampler2DOutput = Sampler::video();			///< Sampler for sampling the LDR 2D output into the framebuffer/composite input buffer
	Sampler			sampler3D = Sampler::video();				///< Sampler for sampling the framebuffer into the HDR 3D buffer (including if using a shader)
	Sampler			sampler3DOutput = Sampler::video();			///< Sampler for sampling the HDR 3D output buffer back into the framebuffer
	Sampler			samplerPrecomposite = Sampler::video();		///< Sampler for precomposite (framebuffer blit) to composite input buffer
	Sampler			samplerComposite = Sampler::video();		///< Sampler for sampling the shaderComposite iChannel0 input
	Sampler			samplerFinal = Sampler::video();			///< Sampler for sampling composite (shader) output buffer into the final framebuffer

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;

};

class PlayerConfig {
public:
	// View parameters
	float           moveRate = 0.0f;							///< Player move rate (defaults to no motion)
	float           height = 1.5f;								///< Height for the player view (in walk mode)
	float           crouchHeight = 1.5f;						///< Height for the player view (during crouch in walk mode)
	float           jumpVelocity = 0.0f;						///< Jump velocity for the player
	float           jumpInterval = 0.5f;						///< Minimum time between jumps in seconds
	bool            jumpTouch = true;							///< Require the player to be touch a surface to jump?
	Vector3         gravity = Vector3(0.0f, -10.0f, 0.0f);		///< Gravity vector
	Vector2			moveScale = Vector2(1.0f, 1.0f);			///< Player (X/Y) motion scaler
	Vector2			turnScale = Vector2(1.0f, 1.0f);			///< Player (horizontal/vertical) turn rate scaler
	Array<bool>		axisLock = { false, false, false };			///< World-space player motion axis lock
	bool			stillBetweenTrials = false;					///< Disable player motion between trials?
	bool			resetPositionPerTrial = false;				///< Reset the player's position on a per trial basis (to scene default)

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

/** Storage for static (never changing) HUD elements */
struct StaticHudElement {
	String			filename;												///< Filename of the image
	Vector2			position;												///< Position to place the element (fractional window-space)
	Vector2			scale = Vector2(1.0, 1.0);								///< Scale to apply to the image

	StaticHudElement() {};
	StaticHudElement(const Any& any);

	Any toAny(const bool forceAll = false) const;
	bool operator!=(const StaticHudElement& other) const;
};

class HudConfig {
public:
	// HUD parameters
	bool            enable = false;											///< Master control for all HUD elements
	bool            showBanner = false;						                ///< Show the banner display
	String			bannerTimerMode = "remaining";							///< Time to show in the banner ("remaining", "elapsed", or "none")
	bool			bannerShowProgress = true;								///< Show progress in banner?
	bool			bannerShowScore = true;									///< Show score in banner?
	float           bannerVertVisible = 0.41f;				                ///< Vertical banner visibility
	float           bannerLargeFontSize = 30.0f;				            ///< Banner percent complete font size
	float           bannerSmallFontSize = 14.0f;				            ///< Banner detail font size
	String          hudFont = "dominant.fnt";				                ///< Font to use for Heads Up Display

	// Player health bar
	bool            showPlayerHealthBar = false;							///< Display a player health bar?
	Vector2         playerHealthBarSize = Vector2(0.1f, 0.02f);				///< Player health bar size (as a ratio of screen size)
	Point2          playerHealthBarPos = Point2(0.005f, 0.01f);				///< Player health bar position (as a ratio of screen size)
	Vector2         playerHealthBarBorderSize = Vector2(0.001f, 0.002f);	///< Player health bar border size (as a ratio of screen size)
	Color4          playerHealthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);		///< Player health bar border color
	Array<Color4>   playerHealthBarColors = {								///< Player health bar start/end colors
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Weapon status
	bool            showAmmo = false;											///< Display remaining ammo
	Point2          ammoPosition = Point2(10.0f, 10.0f);						///< Position of the ammo indicator text
	float           ammoSize = 24.0f;											///< Font size for ammo indicator text
	Color4          ammoColor = Color4(1.0, 1.0, 1.0, 1.0);						///< Color for ammo indicator text
	Color4          ammoOutlineColor = Color4(0.0, 0.0, 0.0, 1.0);				///< Outline color for ammo indicator text
	bool            renderWeaponStatus = true;									///< Display weapon cooldown
	String          cooldownMode = "ring";										///< Currently "ring" and "box" are supported
	String          weaponStatusSide = "left";									///< Only applied in "box" mode, can be "right" or "left"
	float           cooldownInnerRadius = 40.0f;								///< Inner radius for cooldown ring
	float           cooldownThickness = 10.0f;									///< Thickness of cooldown ring
	int             cooldownSubdivisions = 64;									///< Number of polygon divisions in the "ring"
	Color4          cooldownColor = Color4(1.0f, 1.0f, 1.0f, 0.75f);			///< Cooldown ring color when active (transparent when inactive)

	Array<StaticHudElement> staticElements;										///< A set of static HUD elements to draw

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class AudioConfig {
public:
	// Sounds
	String			sceneHitSound = "sound/fpsci_miss_100ms.wav";		///< Sound to play when hitting the scene
	float			sceneHitSoundVol = 1.0f;							///< Volume to play scene hit sound at
	String			refTargetHitSound = "";								///< Reference target hit sound (filename)
	float			refTargetHitSoundVol = 1.0f;						///< Volume to play target hit sound at
	bool			refTargetPlayFireSound = false;						///< Play the weapon's fire sound when shooting at the reference target

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class TimingConfig {
public:
	// Timing parameters
	float			pretrialDuration = 0.5f;					///< Time in ready (pre-trial) state in seconds
	float			pretrialDurationLambda = fnan();			///< Lambda used for pretrial duration truncated exponential randomization
	Array<float>	pretrialDurationRange = {};					///< Pretrial duration range (if randomized)
	float           maxTrialDuration = 100000.0f;				///< Maximum time spent in any one trial task
	float           trialFeedbackDuration = 1.0f;				///< Time in the per-trial feedback state in seconds
	float			sessionFeedbackDuration = 2.0f;				///< Time in the session feedback state in seconds
	bool			clickToStart = true;						///< Require a click before starting the first session (spawning the reference target)
	bool			sessionFeedbackRequireClick = false;		///< Require a click to progress from the session feedback?
	float			maxPretrialAimDisplacement = 180.0f;		///< Maximum (angular) aim displacement in the pretrial duration in degrees, set to negative value to disable

	// Trial count
	int             defaultTrialCount = 5;						///< Default trial count

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};


class FeedbackConfig {
public:
	String initialWithRef = "Click to spawn a target, then use shift on red target to begin.";		///< An initial feedback message to show when reference targets are drawn
	String initialNoRef = "Click to start the session!";											///< An initial feedback message to show when reference targets aren't drawn
	String aimInvalid = "Invalid trial! Do not displace your aim during the pretrial duration.";	///< Message to display when a trial is invalidated due to pretrial aim displacement
	String trialSuccess = "%trialTaskTimeMs ms!";													///< Successful trial feedback message
	String trialFailure = "Failure!";																///< Failed trial feedback message
	String blockComplete = "Block %lastBlock complete! Starting block %currBlock.";					///< Block complete feedback message
	String sessComplete = "Session complete! You scored %totalTimeLeftS!";							///< Session complete feedback message
	String allSessComplete = "All Sessions Complete!";												///< All sessions complete feedback message

	float fontSize = 20.0f;											///< Default font scale/size

	Color4 color = Color3(0.638f, 1.0f, 0.0f);						///< Color to draw the feedback message foreground
	Color4 outlineColor = Color4::clear();							///< Color to draw the feedback message background
	Color4 backgroundColor = Color4(0.0f, 0.0f, 0.0f, 0.5f);		///< Background color

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class TargetViewConfig {
public:
	// Target color based on health
	Array<Color3>   healthColors = {									    ///< Target start/end color (based on target health)
		Color3(0.0, 1.0, 0.0),
		Color3(1.0, 0.0, 0.0)
	};

	// Target health bars
	bool            showHealthBars = false;									///< Display a target health bar?
	Point2          healthBarSize = Point2(100.0f, 10.0f);					///< Health bar size (in pixels)
	Point3          healthBarOffset = Point3(0.0f, -50.0f, 0.0f);			///< Offset from target to health bar (in pixels)
	Point2          healthBarBorderSize = Point2(2.0f, 2.0f);				///< Thickness of the target health bar border
	Color4          healthBarBorderColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);	///< Health bar border color
	Array<Color4>   healthBarColors = {										///< Target health bar start/end color
		Color4(0.0, 1.0, 0.0, 1.0),
		Color4(1.0, 0.0, 0.0, 1.0)
	};

	// Floating combat text controls
	bool            showCombatText = false;								///< Display floating combat text?
	String          combatTextFont = "dominant.fnt";					///< Font to use for combat text
	float           combatTextSize = 16.0;								///< Font size for floating combat text
	Color4          combatTextColor = Color4(1.0, 0.0, 0.0, 1.0);		///< The main color for floating combat text
	Color4          combatTextOutline = Color4(0.0, 0.0, 0.0, 1.0);		///< Combat text outline color
	Point3          combatTextOffset = Point3(0.0, -10.0, 0.0);			///< Initial offset for combat text
	Point3          combatTextVelocity = Point3(0.0, -100.0, 0.0);		///< Move rate/vector for combat text
	float           combatTextFade = 0.98f;								///< Fade rate for combat text (0 implies don't fade)	
	float           combatTextTimeout = 0.5f;							///< Time for combat text to disappear (in seconds)

	// Reference/dummy target
	bool			showRefTarget = true;								///< Show the reference target?
	float           refTargetSize = 0.05f;								///< Size of the reference target
	Color3          refTargetColor = Color3(1.0, 0.0, 0.0);				///< Default reference target color
	Any refTargetModelSpec = PARSE_ANY(ArticulatedModel::Specification{	///< Basic model spec for reference target
		filename = "model/target/target.obj";
		cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings{
					allowVertexMerging = true;
					forceComputeNormals = false;
					forceComputeTangents = false;
					forceVertexMerging = true;
					maxEdgeLength = inf;
					maxNormalWeldAngleDegrees = 0;
					maxSmoothAngleDegrees = 0;
		};
	});

	bool			clearDecalsWithRef = false;							///< Clear the decals created from the reference target at the start of the task
	bool			previewWithRef = false;								///< Show preview of per-trial targets with the reference?
	bool			showRefDecals = true;								///< Show missed shot decals when shooting at the reference target?
	Color3			previewColor = Color3(0.5, 0.5, 0.5);				///< Color to show preview targets in

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class ClickToPhotonConfig {
public:
	// Click-to-photon
	bool            enabled = false;                            ///< Render click to photon box
	String          side = "right";                             ///< "right" for right side, otherwise left
	String			mode = "total";								///< Mode used to signal either minimum system latency ("minimum"), or added latency ("total"), or "both"
	Point2          size = Point2(0.05f, 0.035f);				///< Size of the click-to-photon area (ratio of screen space)
	float           vertPos = 0.5f;				                ///< Percentage of the screen down to locate the box
	Array<Color3>   colors = {				                    ///< Colors to apply to click to photon box
		Color3::white() * 0.2f,
		Color3::white() * 0.8f
	};

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class LoggerConfig {
public:
	// Enable flags for log
	bool enable = true;					///< High-level logging enable flag (if false no output is created)							
	bool logTargetTrajectories = true;	///< Log target trajectories in table?
	bool logFrameInfo = true;			///< Log frame info in table?
	bool logPlayerActions = true;		///< Log player actions in table?
	bool logTrialResponse = true;		///< Log trial response in table?
	bool logUsers = true;				///< Log user information in table?
	bool logOnChange = false;			///< Only log to Player_Action/Target_Trajectory table when the player/target position/orientation changes

	bool logToSingleDb = true;			///< Log all results to a single db file?

	// Session parameter logging
	Array<String> sessParamsToLog = { "frameRate", "frameDelay" };			///< Parameter names to log to the Sessions table of the DB

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

class CommandSpec {
public:
	String	cmdStr;										///< Command string
	bool foreground = false;							///< Flag to indicate foreground vs background
	bool blocking = false;								///< Flag to indicate to block on this process complete

	CommandSpec() {}
	CommandSpec(const Any& any);

	Any toAny(const bool forceAll = true) const;
};

class CommandConfig {
public:
	Array<CommandSpec> sessionStartCmds;				///< Command to run on start of a session
	Array<CommandSpec> sessionEndCmds;						///< Command to run on end of a session
	Array<CommandSpec> trialStartCmds;						///< Command to run on start of a trial
	Array<CommandSpec> trialEndCmds;							///< Command to run on end of a trial

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, const bool forceAll = false) const;
};

class Question {
public:
	enum Type {
		None,
		MultipleChoice,
		Entry,
		Rating,
		DropDown
	};

	Type type = Type::None;
	String prompt = "";					///< Text display to prompt the user for a response
	Array<String> options;				///< List of options provided (if multiple choice)
	Array<GKey> optionKeys;				///< Keys corresponding to the options provided (if multiple choice/ranking)
	String title = "Feedback";			///< Title of the window displayed (if not fullscreen)
	String result = "";					///< Reported result (not specified as configuration)
	bool fullscreen = false;			///< Show this question as fullscreen?
	bool showCursor = true;				///< Show cursor during question response window (allow clicking for selection)?
	bool randomOrder = false;			///< Randomize question response order?
	int optionsPerRow = 3;				///< Number of options per row (defaults to 3 for multiple choice, # of options for ratings)
	float promptFontSize = -1.f;		///< Font size for prompt text
	float optionFontSize = -1.f;		///< Font size for question entry/options
	float buttonFontSize = -1.f;		///< Font size for submit/cancel buttons

	Question() {};
	Question(const Any& any);
};

class FpsConfig : public ReferenceCountedObject {
public:
	int	            settingsVersion = 1;						///< Settings version

	// Sub structures
	SceneConfig			scene;									///< Scene related config parameters			
	RenderConfig		render;									///< Render related config parameters
	PlayerConfig		player;									///< Player related config parameters
	HudConfig			hud;									///< HUD related config parameters
	AudioConfig			audio;									///< Audio related config parameters
	TimingConfig		timing;									///< Timing related config parameters
	FeedbackConfig		feedback;								///< Feedback message config parameters
	TargetViewConfig	targetView;								///< Target drawing config parameters
	ClickToPhotonConfig clickToPhoton;							///< Click to photon config parameters
	LoggerConfig		logger;									///< Logging configuration
	WeaponConfig		weapon;			                        ///< Weapon to be used
	MenuConfig			menu;									///< User settings window configuration
	CommandConfig		commands;								///< Commands to run during execution
	Array<Question>		questionArray;							///< Array of questions for this experiment/trial

	// Constructors
	FpsConfig(const Any& any) {
		load(any);
	}

	FpsConfig(const Any& any, const FpsConfig& defaultConfig) {
		*this = defaultConfig;
		load(any);
	}

	FpsConfig() {}

	void load(const Any& any) {
		FPSciAnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);
		render.load(reader, settingsVersion);
		player.load(reader, settingsVersion);
		hud.load(reader, settingsVersion);
		targetView.load(reader, settingsVersion);
		clickToPhoton.load(reader, settingsVersion);
		audio.load(reader, settingsVersion);
		timing.load(reader, settingsVersion);
		feedback.load(reader, settingsVersion);
		logger.load(reader, settingsVersion);
		menu.load(reader, settingsVersion);
		commands.load(reader, settingsVersion);
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("scene", scene);
			reader.getIfPresent("weapon", weapon);
			reader.getIfPresent("questions", questionArray);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in FpsConfig.\n", settingsVersion);
			break;
		}

		// Warning message for deprecated sceneName parameter
		String sceneName = "";
		if (reader.getIfPresent("sceneName", sceneName)) {
			logPrintf("WARNING: deprecated sceneName parameter found. The value will not be used. Switch to the following:\n");
			logPrintf("    scene = { name = \"%s\"; };\n", sceneName);
		}
	}

	Any toAny(const bool forceAll = false) const {
		Any a(Any::TABLE);
		FpsConfig def;
		a["settingsVersion"] = settingsVersion;
		if (forceAll || def.scene != scene) a["scene"] = scene;
		a = render.addToAny(a, forceAll);
		a = player.addToAny(a, forceAll);
		a = hud.addToAny(a, forceAll);
		a = targetView.addToAny(a, forceAll);
		a = clickToPhoton.addToAny(a, forceAll);
		a = audio.addToAny(a, forceAll);
		a = timing.addToAny(a, forceAll);
		a = feedback.addToAny(a, forceAll);
		a = logger.addToAny(a, forceAll);
		a = menu.addToAny(a, forceAll);
		a = commands.addToAny(a, forceAll);
		a["weapon"] = weapon.toAny(forceAll);
		return a;
	}
};

