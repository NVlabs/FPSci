#pragma once

#include <G3D/G3D.h>
#include "Weapon.h"
#include "TargetEntity.h"

template <class T>
static bool operator!=(Array<T> a1, Array<T> a2) {
	for (int i = 0; i < a1.size(); i++) {
		if (a1[i] != a2[i]) return true;
	}
	return false;
}
template <class T>
static bool operator==(Array<T> a1, Array<T> a2) {
	return !(a1 != a2);
}


/** Trial count class (optional for alternate TargetConfig/count table lookup) */
class TrialCount {
public:
	Array<String>	ids;			///< Trial ID list
	int				count = 1;		///< Count of trials to be performed
	static int		defaultCount;	///< Default count to use

	TrialCount() {};

	TrialCount(Array<String> trialIds, int trialCount) {
		ids = trialIds;
		count = trialCount;
	}

	/** Load from Any */
	TrialCount(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("ids", ids, "An \"ids\" field must be provided for each set of trials!");
			if (!reader.getIfPresent("count", count)) {
				count = defaultCount;
			}
			if(count < 1) {
				throw format("Trial count < 1 not allowed! (%d count for trial with targets: %s)", count, Any(ids).unparse());
			}
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["ids"] = ids;
		a["count"] = count;
		return a;
	}
};

class Question {
public:
	enum Type {
		None,
		MultipleChoice,
		Entry,
		Rating
	};

	Type type = Type::None;
	String prompt = "";
	Array<String> options;
	Array<GKey> optionKeys;
	String title = "Feedback";
	String result = "";
	bool fullscreen = false;
	bool showCursor = true;

	Question() {};

	Question(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		String typeStr;

		switch (settingsVersion) {
		case 1:
			// Get the question type
			reader.get("type", typeStr, "A \"type\" field must be provided with every question!");
			// Pase the type and get options for multiple choice
			if (!typeStr.compare("MultipleChoice")) {
				type = Type::MultipleChoice;
				reader.get("options", options, "An \"options\" Array must be specified with \"MultipleChoice\" style questions!");
			}
			else if (!typeStr.compare("Entry")) {
				type = Type::Entry;
			}
			else if (!typeStr.compare("Rating")) {
				type = Type::Rating;
				reader.get("options", options, "An \"options\" Array must be specified with \"Rating\" style questions!");
			}
			else {
				throw format("Unrecognized question \"type\" String \"%s\". Valid options are \"MultipleChoice\" or \"Entry\"", typeStr);
			}

			// Get the question prompt (required) and title (optional)
			reader.get("prompt", prompt, "A \"prompt\" field must be provided with every question!");
			reader.getIfPresent("title", title);
			reader.getIfPresent("fullscreen", fullscreen);
			reader.getIfPresent("showCursor", showCursor);

			// Handle (optional) key binds for options (if provided)
			if (type == Type::Rating || type == Type::MultipleChoice) {
				reader.getIfPresent("optionKeys", optionKeys);
				// Check length of option keys matches length of options (if provided)
				if (optionKeys.length() > 0 && optionKeys.length() != options.length()) {
					throw format("Length of \"optionKeys\" (%d) did not match \"options\" (%d) for question!", optionKeys.length(), options.length());
				}
			}
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in Question.\n", settingsVersion);
			break;
		}
	}

};

class RenderConfig {
public:
	// Rendering parameters
	float           frameRate = 1000.0f;						///< Target (goal) frame rate (in Hz)
	int             frameDelay = 0;								///< Integer frame delay (in frames)
	String          shader = "";								///< Option for a custom shader name
	float           hFoV = 103.0f;							    ///< Field of view (horizontal) for the user

	void load(AnyTableReader reader, int settingsVersion=1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("frameRate", frameRate);
			reader.getIfPresent("frameDelay", frameDelay);
			reader.getIfPresent("shader", shader);
			reader.getIfPresent("horizontalFieldOfView", hFoV);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		RenderConfig def;
		if(forceAll || def.frameRate != frameRate)		a["frameRate"] = frameRate;
		if(forceAll || def.frameDelay != frameDelay)	a["frameDelay"] = frameDelay;
		if(forceAll || def.hFoV != hFoV)				a["horizontalFieldOfView"] = hFoV;
		if(forceAll || def.shader != shader)			a["shader"] = shader;
		return a;
	}

};

class PlayerConfig {
public:
	// View parameters
	float           moveRate = 0.0f;							///< Player move rate (defaults to no motion)
	float           height = 1.5f;								///< Height for the player view (in walk mode)
	float           crouchHeight = 0.8f;						///< Height for the player view (during crouch in walk mode)
	float           jumpVelocity = 3.5f;						///< Jump velocity for the player
	float           jumpInterval = 0.5f;						///< Minimum time between jumps in seconds
	bool            jumpTouch = true;							///< Require the player to be touch a surface to jump?
	Vector3         gravity = Vector3(0.0f, -10.0f, 0.0f);		///< Gravity vector
	Vector2			moveScale = Vector2(1.0f, 1.0f);			///< Player (X/Y) motion scaler
	Vector2			turnScale = Vector2(1.0f, 1.0f);			///< Player (horizontal/vertical) turn rate scaler
	Array<bool>		axisLock = { false, false, false };			///< World-space player motion axis lock
	bool			stillBetweenTrials = false;					///< Disable player motion between trials?
	bool			resetPositionPerTrial = false;				///< Reset the player's position on a per trial basis (to scene default)

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("moveRate", moveRate);
			reader.getIfPresent("moveScale", moveScale);
			reader.getIfPresent("turnScale", turnScale);
			reader.getIfPresent("playerHeight", height);
			reader.getIfPresent("crouchHeight", crouchHeight);
			reader.getIfPresent("jumpVelocity", jumpVelocity);
			reader.getIfPresent("jumpInterval", jumpInterval);
			reader.getIfPresent("jumpTouch", jumpTouch);
			reader.getIfPresent("playerGravity", gravity);
			reader.getIfPresent("playerAxisLock", axisLock);
			reader.getIfPresent("disablePlayerMotionBetweenTrials", stillBetweenTrials);
			reader.getIfPresent("resetPlayerPositionBetweenTrials", resetPositionPerTrial);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		PlayerConfig def;
		if(forceAll || def.moveRate != moveRate )			a["moveRate"] = moveRate;
		if(forceAll || def.moveScale != moveScale )			a["moveScale"] = moveScale;
		if(forceAll || def.height != height )				a["playerHeight"] = height;
		if(forceAll || def.crouchHeight != crouchHeight )	a["crouchHeight"] = crouchHeight;
		if(forceAll || def.jumpVelocity != jumpVelocity)	a["jumpVelocity"] = jumpVelocity;
		if(forceAll || def.jumpInterval != jumpInterval)	a["jumpInterval"] = jumpInterval;
		if(forceAll || def.jumpTouch != jumpTouch)			a["jumpTouch"] = jumpTouch;
		if(forceAll || def.gravity != gravity)				a["playerGravity"] = gravity;
		if(forceAll || def.axisLock != axisLock)			a["playerAxisLock"] = axisLock;
		if(forceAll || def.stillBetweenTrials != stillBetweenTrials)		a["disablePlayerMotionBetweenTrials"] = stillBetweenTrials;
		if(forceAll || def.resetPositionPerTrial != resetPositionPerTrial)	a["resetPlayerPositionBetweenTrials"] = resetPositionPerTrial;
		return a;
	}

};

/** Storage for static (never changing) HUD elements */
struct StaticHudElement {
	String			filename;												///< Filename of the image
	Vector2			position;												///< Position to place the element (fractional window-space)
	Vector2			scale = Vector2(1.0, 1.0);								///< Scale to apply to the image

	StaticHudElement() {};
	StaticHudElement(const Any& any) {
		AnyTableReader reader(any);
		reader.get("filename", filename, "Must provide filename for all Static HUD elements!");
		reader.get("position", position, "Must provide position for all static HUD elements");
		reader.getIfPresent("scale", scale);
	}

	Any toAny(const bool forceAll = false) const {
		Any a(Any::TABLE);
		a["filename"] = filename;
		a["position"] = position;
		if (forceAll || scale != Vector2(1.0, 1.0))  a["scale"] = scale;
		return a;
	}

	bool operator!=(StaticHudElement other) const {
		return filename != other.filename ||
			position != other.position ||
			scale != other.scale;
	}
};

class HudConfig {
public:
	// HUD parameters
	bool            enable = false;											///< Master control for all HUD elements
	bool            showBanner = false;						                ///< Show the banner display
	float           bannerVertVisible = 0.41f;				                ///< Vertical banner visibility
	float           bannerLargeFontSize = 30.0f;				            ///< Banner percent complete font size
	float           bannerSmallFontSize = 14.0f;				            ///< Banner detail font size
	String          hudFont = "dominant.fnt";				                ///< Font to use for Heads Up Display

	// Player health bar
	bool            showPlayerHealthBar = false;							///< Display a player health bar?
	Point2          playerHealthBarSize = Point2(200.0f, 20.0f);			///< Player health bar size (in pixels)
	Point2          playerHealthBarPos = Point2(10.0f, 10.0f);				///< Player health bar position (in pixels)
	Point2          playerHealthBarBorderSize = Point2(2.0f, 2.0f);			///< Player health bar border size
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

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("showHUD", enable);
			reader.getIfPresent("showBanner", showBanner);
			reader.getIfPresent("hudFont", hudFont);
			reader.getIfPresent("showPlayerHealthBar", showPlayerHealthBar);
			reader.getIfPresent("playerHealthBarSize", playerHealthBarSize);
			reader.getIfPresent("playerHealthBarPosition", playerHealthBarPos);
			reader.getIfPresent("playerHealthBarBorderSize", playerHealthBarBorderSize);
			reader.getIfPresent("playerHealthBarBorderColor", playerHealthBarBorderColor);
			reader.getIfPresent("playerHealthBarColors", playerHealthBarColors);
			reader.getIfPresent("showAmmo", showAmmo);
			reader.getIfPresent("ammoPosition", ammoPosition);
			reader.getIfPresent("ammoSize", ammoSize);
			reader.getIfPresent("ammoColor", ammoColor);
			reader.getIfPresent("ammoOutlineColor", ammoOutlineColor);
			reader.getIfPresent("renderWeaponStatus", renderWeaponStatus);
			reader.getIfPresent("weaponStatusSide", weaponStatusSide);
			reader.getIfPresent("cooldownMode", cooldownMode);
			reader.getIfPresent("cooldownInnerRadius", cooldownInnerRadius);
			reader.getIfPresent("cooldownThickness", cooldownThickness);
			reader.getIfPresent("cooldownSubdivisions", cooldownSubdivisions);
			reader.getIfPresent("cooldownColor", cooldownColor);
			reader.getIfPresent("staticHUDElements", staticElements);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		HudConfig def;
		if(forceAll || def.enable != enable)											a["showHUD"] = enable;
		if(forceAll || def.showBanner != showBanner)									a["showBanner"] = showBanner;
		if(forceAll || def.hudFont != hudFont)											a["hudFont"] = hudFont;
		if(forceAll || def.showPlayerHealthBar != showPlayerHealthBar)					a["showPlayerHealthBar"] = showPlayerHealthBar;
		if(forceAll || def.playerHealthBarSize != playerHealthBarSize)					a["playerHealthBarSize"] = playerHealthBarSize;
		if(forceAll || def.playerHealthBarPos != playerHealthBarPos)					a["playerHealthBarPosition"] = playerHealthBarPos;
		if(forceAll || def.playerHealthBarBorderSize != playerHealthBarBorderSize)		a["playerHealthBarBorderSize"] = playerHealthBarBorderSize;
		if(forceAll || def.playerHealthBarBorderColor != playerHealthBarBorderColor)	a["playerHealthBarBorderColor"] = playerHealthBarBorderColor;
		if(forceAll || def.playerHealthBarColors != playerHealthBarColors)				a["playerHealthBarColors"] = playerHealthBarColors;
		if(forceAll || def.showAmmo != showAmmo)										a["showAmmo"] = showAmmo;
		if(forceAll || def.ammoPosition != ammoPosition)								a["ammoPosition"] = ammoPosition;
		if(forceAll || def.ammoSize != ammoSize)										a["ammoSize"] = ammoSize;
		if(forceAll || def.ammoColor != ammoColor)										a["ammoColor"] = ammoColor;
		if(forceAll || def.ammoOutlineColor != ammoOutlineColor)						a["ammoOutlineColor"] = ammoOutlineColor;
		if(forceAll || def.renderWeaponStatus != renderWeaponStatus)					a["renderWeaponStatus"] = renderWeaponStatus;
		if(forceAll || def.weaponStatusSide != weaponStatusSide)						a["weaponStatusSide"] = weaponStatusSide;
		if(forceAll || def.cooldownMode != cooldownMode)								a["cooldownMode"] = cooldownMode;
		if(forceAll || def.cooldownInnerRadius != cooldownInnerRadius)					a["cooldownInnerRadius"] = cooldownInnerRadius;
		if(forceAll || def.cooldownThickness != cooldownThickness)						a["cooldownThickness"] = cooldownThickness;
		if(forceAll || def.cooldownSubdivisions != cooldownSubdivisions)				a["cooldownSubdivisions"] = cooldownSubdivisions;
		if(forceAll || def.cooldownColor != cooldownColor)								a["cooldownColor"] = cooldownColor;
		if (forceAll || def.staticElements != staticElements)							a["staticHUDElements"] = staticElements;
		return a;
	}
};

class SceneConfig {
public:

	String name;										///< Name of the scene to load
	String playerCamera;								///< Name of the camera to use for the player

	//float gravity = fnan();							///< Gravity for the PhysicsScene
	float resetHeight = fnan();							///< Reset height for the PhysicsScene

	Point3 spawnPosition = { fnan(),fnan(), fnan() };	///< Location for player spawn
	float spawnHeading = fnan();						///< Heading for player spawn

	SceneConfig() {}
	SceneConfig(const Any& any) {
		AnyTableReader reader(any);
		int settingsVersion = 1;
		reader.getIfPresent("settingsVersion", settingsVersion);
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("name", name);
			reader.getIfPresent("playerCamera", playerCamera);
			//reader.getIfPresent("gravity", gravity);
			reader.getIfPresent("resetHeight", resetHeight);
			reader.getIfPresent("spawnPosition", spawnPosition);
			reader.getIfPresent("spawnHeading", spawnHeading);
			break;
		default:
			throw format("Did not recognize scene config settings version: %d", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = false) const {
		Any a(Any::TABLE);
		SceneConfig def;
		if (forceAll || def.name != name)					a["name"] = name;
		if (forceAll || def.playerCamera != playerCamera)   a["playerCamera"] = playerCamera;
		//if (forceAll || def.gravity != gravity)				a["gravity"] = gravity;
		if (forceAll || def.resetHeight != resetHeight)		a["resetHeight"] = resetHeight;
		if (forceAll || def.spawnPosition != spawnPosition) a["spawnPosition"] = spawnPosition;
		if (forceAll || def.spawnHeading != spawnHeading)   a["spawnHeading"] = spawnHeading;
		return a;
	}

	bool operator!=(SceneConfig other) const {
		return name != other.name ||
			//gravity != other.gravity ||
			resetHeight != other.resetHeight ||
			spawnPosition != other.spawnPosition ||
			spawnHeading != other.spawnHeading;
	}
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

	bool			previewWithRef = false;								///< Show preview of per-trial targets with the reference
	Color3			previewColor = Color3(0.5, 0.5, 0.5);				///< Color to show preview targets in

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("showTargetHealthBars", showHealthBars);
			reader.getIfPresent("targetHealthBarSize", healthBarSize);
			reader.getIfPresent("targetHealthBarOffset", healthBarOffset);
			reader.getIfPresent("targetHealthBarBorderSize", healthBarBorderSize);
			reader.getIfPresent("targetHealthBarBorderColor", healthBarBorderColor);
			reader.getIfPresent("targetHealthColors", healthColors);
			reader.getIfPresent("targetHealthBarColors", healthBarColors);
			reader.getIfPresent("showFloatingCombatText", showCombatText);
			reader.getIfPresent("floatingCombatTextSize", combatTextSize);
			reader.getIfPresent("floatingCombatTextFont", combatTextFont);
			reader.getIfPresent("floatingCombatTextColor", combatTextColor);
			reader.getIfPresent("floatingCombatTextOutlineColor", combatTextOutline);
			reader.getIfPresent("floatingCombatTextOffset", combatTextOffset);
			reader.getIfPresent("floatingCombatTextVelocity", combatTextVelocity);
			reader.getIfPresent("floatingCombatTextFade", combatTextFade);
			reader.getIfPresent("floatingCombatTextTimeout", combatTextTimeout);
			reader.getIfPresent("showReferenceTarget", showRefTarget);
			reader.getIfPresent("referenceTargetSize", refTargetSize);
			reader.getIfPresent("referenceTargetColor", refTargetColor);
			reader.getIfPresent("showPreviewTargetsWithReference", previewWithRef);
			reader.getIfPresent("previewTargetColor", previewColor);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		TargetViewConfig def;
		if(forceAll || def.showHealthBars != showHealthBars)				a["showTargetHealthBars"] = showHealthBars;
		if(forceAll || def.healthBarSize != healthBarSize)					a["targetHealthBarSize"] = healthBarSize;
		if(forceAll || def.healthBarOffset != healthBarOffset)				a["targetHealthBarOffset"] = healthBarOffset;
		if(forceAll || def.healthBarBorderSize != healthBarBorderSize)		a["targetHealthBarBorderSize"] = healthBarBorderSize;
		if(forceAll || def.healthBarBorderColor != healthBarBorderColor)	a["targetHealthBarBorderColor"] = healthBarBorderColor;
		if(forceAll || def.healthColors != healthColors)					a["targetHealthColors"] = healthColors;
		if(forceAll || def.healthBarColors != healthBarColors)				a["targetHealthBarColors"] = healthBarColors;
		if(forceAll || def.showCombatText != showCombatText)				a["showFloatingCombatText"] = showCombatText;
		if(forceAll || def.combatTextSize != combatTextSize)				a["floatingCombatTextSize"] = combatTextSize;
		if(forceAll || def.combatTextFont != combatTextFont)				a["floatingCombatTextFont"] = combatTextFont;
		if(forceAll || def.combatTextColor != combatTextColor)				a["floatingCombatTextColor"] = combatTextColor;
		if(forceAll || def.combatTextOutline != combatTextOutline)			a["floatingCombatTextOutlineColor"] = combatTextOutline;
		if(forceAll || def.combatTextOffset != combatTextOffset)			a["floatingCombatTextOffset"] = combatTextOffset;
		if(forceAll || def.combatTextVelocity != combatTextVelocity)		a["floatingCombatTextVelocity"] = combatTextVelocity;
		if(forceAll || def.combatTextFade != combatTextFade)				a["floatingCombatTextFade"] = combatTextFade;
		if(forceAll || def.combatTextTimeout != combatTextTimeout)			a["floatingCombatTextTimeout"] = combatTextTimeout;
		if(forceAll || def.showRefTarget != showRefTarget)					a["showRefTarget"] = showRefTarget;
		if(forceAll || def.refTargetSize != refTargetSize)					a["referenceTargetSize"] = refTargetSize;
		if(forceAll || def.refTargetColor != refTargetColor)				a["referenceTargetColor"] = refTargetColor;
		if(forceAll || def.previewWithRef != previewWithRef)				a["showPreviewTargetsWithReference"] = previewWithRef;
		if(forceAll || def.previewColor != previewColor)					a["previewTargetColor"] = previewColor;
		return a;
	}
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

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("renderClickPhoton", enabled);
			reader.getIfPresent("clickPhotonSide", side);
			reader.getIfPresent("clickPhotonMode", mode);		
			reader.getIfPresent("clickPhotonSize", size);
			reader.getIfPresent("clickPhotonVertPos", vertPos);
			reader.getIfPresent("clickPhotonColors", colors);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		ClickToPhotonConfig def;
		if(forceAll || def.enabled != enabled)		a["renderClickPhoton"] = enabled;
		if(forceAll || def.side != side)			a["clickPhotonSide"] = side;
		if(forceAll || def.size != size)			a["clickPhotonSize"] = size;
		if(forceAll || def.vertPos != vertPos)		a["clickPhotonVertPos"] = vertPos;
		if(forceAll || def.colors != colors)		a["clickPhotonColors"] = colors;
		return a;
	}
};

class AudioConfig {
public:
	// Sounds
	String			sceneHitSound = "sound/fpsci_miss_100ms.wav";								///< Sound to play when hitting the scene
	float			sceneHitSoundVol = 1.0f;

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("sceneHitSound", sceneHitSound);
			reader.getIfPresent("sceneHitSoundVol", sceneHitSoundVol);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		AudioConfig def;
		if(forceAll || def.sceneHitSound != sceneHitSound)			a["sceneHitSound"] = sceneHitSound;
		if(forceAll || def.sceneHitSoundVol != sceneHitSoundVol)	a["sceneHitSoundVol"] = sceneHitSoundVol;
		return a;
	}
};

class TimingConfig {
public:
	// Timing parameters
	float           pretrialDuration = 0.5f;					///< Time in ready (pre-trial) state in seconds
	float           maxTrialDuration = 100000.0f;				///< Maximum time spent in any one trial task
	float           trialFeedbackDuration = 1.0f;				///< Time in the per-trial feedback state in seconds
	float			sessionFeedbackDuration = 2.0f;				///< Time in the session feedback state in seconds
	bool			clickToStart = true;						///< Require a click before starting the first session (spawning the reference target)
	bool			sessionFeedbackRequireClick = false;		///< Require a click to progress from the session feedback?

	// Trial count
	int             defaultTrialCount = 5;						///< Default trial count

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("pretrialDuration", pretrialDuration);
			reader.getIfPresent("maxTrialDuration", maxTrialDuration);
			reader.getIfPresent("trialFeedbackDuration", trialFeedbackDuration);
			reader.getIfPresent("sessionFeedbackDuration", sessionFeedbackDuration);
			reader.getIfPresent("clickToStart", clickToStart);
			reader.getIfPresent("sessionFeedbackRequireClick", sessionFeedbackRequireClick);
			reader.getIfPresent("defaultTrialCount", defaultTrialCount);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		TimingConfig def;
		if (forceAll || def.pretrialDuration != pretrialDuration)				a["pretrialDuration"] = pretrialDuration;
		if (forceAll || def.maxTrialDuration != maxTrialDuration)				a["maxTrialDuration"] = maxTrialDuration;
		if (forceAll || def.trialFeedbackDuration != trialFeedbackDuration)		a["trialFeedbackDuration"] = trialFeedbackDuration;
		if (forceAll || def.sessionFeedbackDuration != sessionFeedbackDuration)	a["sessionFeedbackDuration"] = sessionFeedbackDuration;
		if (forceAll || def.clickToStart != clickToStart)						a["clickToStart"] = clickToStart;
		if (forceAll || def.sessionFeedbackRequireClick != sessionFeedbackRequireClick) a["sessionFeedbackRequireClick"] = sessionFeedbackRequireClick;
		if (forceAll || def.defaultTrialCount != defaultTrialCount)				a["defaultTrialCount"] = defaultTrialCount;
		return a;
	}
};

class FeedbackConfig {
public:
	String initialWithRef = "Click to spawn a target, then use shift on red target to begin.";		///< An initial feedback message to show when reference targets are drawn
	String initialNoRef = "Click to start the session!";											///< An initial feedback message to show when reference targets aren't drawn
	String trialSuccess = "%trialTaskTimeMs ms!" ;													///< Successful trial feedback message
	String trialFailure = "Failure!";																///< Failed trial feedback message
	String blockComplete = "Block %lastBlock complete! Starting block %currBlock.";					///< Block complete feedback message
	String sessComplete = "Session complete! You scored %totalTimeLeftS!";							///< Session complete feedback message
	String allSessComplete = "All Sessions Complete!";												///< All sessions complete feedback message

	float fontSize = 20.0f;											///< Default font scale/size

	Color4 color = Color3(0.638f, 1.0f, 0.0f);						///< Color to draw the feedback message foreground
	Color4 outlineColor = Color4::clear();							///< Color to draw the feedback message background
	Color4 backgroundColor = Color4(0.0f, 0.0f, 0.0f, 0.5f);		///< Background color

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("referenceTargetInitialFeedback", initialWithRef);
			reader.getIfPresent("noReferenceTargetInitialFeedback", initialNoRef);
			reader.getIfPresent("trialSuccessFeedback", trialSuccess);
			reader.getIfPresent("trialFailureFeedback", trialFailure);
			reader.getIfPresent("blockCompleteFeedback", blockComplete);
			reader.getIfPresent("sessionCompleteFeedback", sessComplete);
			reader.getIfPresent("allSessionsCompleteFeedback", allSessComplete);
			reader.getIfPresent("feedbackColor", color);
			reader.getIfPresent("feedbackOutlineColor", outlineColor);
			reader.getIfPresent("feedbackFontSize", fontSize);
			reader.getIfPresent("feedbackBackgroundColor", backgroundColor);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		FeedbackConfig def;
		if (forceAll || def.initialWithRef != initialWithRef)	a["referenceTargetInitialFeedback"] = initialWithRef;
		if (forceAll || def.initialNoRef != initialNoRef)		a["noReferenceTargetInitialFeedback"] = initialNoRef;
		if (forceAll || def.trialSuccess != trialSuccess)		a["trialSuccessFeedback"] = trialSuccess;
		if (forceAll || def.trialFailure != trialFailure)		a["trialFailureFeedback"] = trialFailure;
		if (forceAll || def.blockComplete != blockComplete)		a["blockCompleteFeedback"] = blockComplete;
		if (forceAll || def.sessComplete != sessComplete)		a["sessionCompleteFeedback"] = sessComplete;
		if (forceAll || def.allSessComplete != allSessComplete) a["allSessionsCompleteFeedback"] = allSessComplete;
		if (forceAll || def.color != color)						a["feedbackColor"] = color;
		if (forceAll || def.outlineColor != outlineColor)		a["feedbackOutlineColor"] = outlineColor;
		if (forceAll || def.fontSize != fontSize)				a["feedbackFontSize"] = fontSize;
		if (forceAll || def.backgroundColor != backgroundColor) a["feedbackBackgroundColor"] = backgroundColor;
		return a;
	}
};

class LoggerConfig {
public:
	// Enable flags for log
	bool enable					= true;		///< High-level logging enable flag (if false no output is created)							
	bool logTargetTrajectories	= true;		///< Log target trajectories in table?
	bool logFrameInfo			= true;		///< Log frame info in table?
	bool logPlayerActions		= true;		///< Log player actions in table?
	bool logTrialResponse		= true;		///< Log trial response in table?
	bool logUsers				= true;		///< Log user information in table?

	bool logToSingleDb			= true;		///< Log all results to a single db file?

	// Session parameter logging
	Array<String> sessParamsToLog = {"frameRate", "frameDelay"};			///< Parameter names to log to the Sessions table of the DB

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("logEnable", enable);
			reader.getIfPresent("logTargetTrajectories", logTargetTrajectories);
			reader.getIfPresent("logFrameInfo", logFrameInfo);
			reader.getIfPresent("logPlayerActions", logPlayerActions);
			reader.getIfPresent("logTrialResponse", logTrialResponse);
			reader.getIfPresent("logUsers", logUsers);
			reader.getIfPresent("sessionParametersToLog", sessParamsToLog);
			reader.getIfPresent("logToSingleDb", logToSingleDb);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, bool forceAll = false) const {
		LoggerConfig def;
		if (forceAll || def.enable != enable)								a["logEnable"] = enable;
		if (forceAll || def.logTargetTrajectories != logTargetTrajectories)	a["logTargetTrajectories"] = logTargetTrajectories;
		if (forceAll || def.logFrameInfo != logFrameInfo)					a["logFrameInfo"] = logFrameInfo;
		if (forceAll || def.logPlayerActions != logPlayerActions)			a["logPlayerActions"] = logPlayerActions;
		if (forceAll || def.logTrialResponse != logTrialResponse)			a["logTrialResponse"] = logTrialResponse;
		if (forceAll || def.logUsers != logUsers)							a["logUsers"] = logUsers;
		if (forceAll || def.sessParamsToLog != sessParamsToLog)				a["sessionParametersToLog"] = sessParamsToLog;
		if (forceAll || def.logToSingleDb != logToSingleDb)					a["logToSingleDb"] = logToSingleDb;
		return a;
	}
};

class CommandSpec {
public:
	String	cmdStr;										///< Command string
	bool foreground = false;							///< Flag to indicate foreground vs background
	bool blocking = false;								///< Flag to indicate to block on this process complete

	CommandSpec() {}
	CommandSpec(const Any& any){
		try {
			AnyTableReader reader(any);
			reader.get("command", cmdStr, "A command string must be specified!");
			reader.getIfPresent("foreground", foreground);
			reader.getIfPresent("blocking", blocking);
		}
		catch (ParseError e) {
			// Handle errors related to older (pure) string-based commands
			e.message += "\nCommands must be specified using a valid CommandSpec!\n";
			e.message += "Refer to the general_config.md file for more information.\n";
			e.message += "If migrating from an older experiment config, use the following syntax:\n";
			e.message += "commandsOnTrialStart = ( { command = \"cmd /c echo Trial start>> commandLog.txt\" } );\n";
			throw e;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		CommandSpec def;
		a["command"] = cmdStr;
		if (forceAll || def.foreground != foreground)	a["foreground"] = foreground;
		if (forceAll || def.blocking != blocking)		a["blocking"] = blocking;
		return a;
	}

};

class CommandConfig {
public: 
	Array<CommandSpec> sessionStartCmds;				///< Command to run on start of a session
	Array<CommandSpec> sessionEndCmds;						///< Command to run on end of a session
	Array<CommandSpec> trialStartCmds;						///< Command to run on start of a trial
	Array<CommandSpec> trialEndCmds;							///< Command to run on end of a trial

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("commandsOnSessionStart", sessionStartCmds);
			reader.getIfPresent("commandsOnSessionEnd", sessionEndCmds);
			reader.getIfPresent("commandsOnTrialStart", trialStartCmds);
			reader.getIfPresent("commandsOnTrialEnd", trialEndCmds);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}
	}

	Any addToAny(Any a, const bool forceAll = false) const {
		if (forceAll || sessionStartCmds.size() > 0)		a["commandsOnSessionStart"] = sessionStartCmds;
		if (forceAll || sessionEndCmds.size() > 0)			a["commandsOnSessionEnd"] = sessionEndCmds;
		if (forceAll || trialStartCmds.size() > 0)			a["commandsOnTrialStart"] = trialStartCmds;
		if (forceAll || trialEndCmds.size() > 0)			a["commandsOnTrialEnd"] = trialEndCmds;
		return a;
	}
};

class MenuConfig {
public: 
	// Menu controls
	bool showMenuLogo					= true;							///< Show the FPSci logo in the user menu
	bool showExperimentSettings			= true;							///< Show the experiment settings options (session/user selection)
	bool showUserSettings				= true;							///< Show the user settings options (master switch)
	bool allowSessionChange				= true;							///< Allow the user to change the session with the menu drop-down
	bool allowUserAdd					= false;						///< Allow the user to add a new user to the experiment
	bool allowUserSettingsSave			= true;							///< Allow the user to save settings changes
	bool allowSensitivityChange			= true;							///< Allow in-game sensitivity change		
	
	bool allowTurnScaleChange			= true;							///< Allow the user to apply X/Y turn scaling
	String xTurnScaleAdjustMode			= "None";						///< X turn scale adjustment mode (can be "None" or "Slider")
	String yTurnScaleAdjustMode			= "Invert";						///< Y turn scale adjustment mode (can be "None", "Invert", or "Slider")

	bool allowReticleChange				= false;						///< Allow the user to adjust their crosshair
	bool allowReticleIdxChange			= true;							///< If reticle change is allowed, allow index change
	bool allowReticleSizeChange			= true;							///< If reticle change is allowed, allow size change
	bool allowReticleColorChange		= true;							///< If reticle change is allowed, allow color change
	bool allowReticleChangeTimeChange	= false;						///< Allow the user to change the reticle change time
	bool showReticlePreview				= true;							///< Show a preview of the reticle

	bool showMenuOnStartup				= true;							///< Show the user menu on startup?
	bool showMenuBetweenSessions		= true;							///< Show the user menu between session?

	void load(AnyTableReader reader, int settingsVersion = 1) {
		switch (settingsVersion) {
		case 1:
			reader.getIfPresent("showMenuLogo", showMenuLogo);
			reader.getIfPresent("showExperimentSettings", showExperimentSettings);
			reader.getIfPresent("showUserSettings", showUserSettings);
			reader.getIfPresent("allowSessionChange", allowSessionChange);
			reader.getIfPresent("allowUserAdd", allowUserAdd);
			reader.getIfPresent("allowUserSettingsSave", allowUserSettingsSave);
			reader.getIfPresent("allowSensitivityChange", allowSensitivityChange);
			reader.getIfPresent("allowTurnScaleChange", allowTurnScaleChange);
			reader.getIfPresent("xTurnScaleAdjustMode", xTurnScaleAdjustMode);
			reader.getIfPresent("yTurnScaleAdjustMode", yTurnScaleAdjustMode);
			reader.getIfPresent("allowReticleChange", allowReticleChange);
			reader.getIfPresent("allowReticleIdxChange", allowReticleIdxChange);
			reader.getIfPresent("allowReticleSizeChange", allowReticleSizeChange);
			reader.getIfPresent("allowReticleColorChange", allowReticleColorChange);
			reader.getIfPresent("allowReticleChangeTimeChange", allowReticleChangeTimeChange);
			reader.getIfPresent("showReticlePreview", showReticlePreview);
			reader.getIfPresent("showMenuOnStartup", showMenuOnStartup);
			reader.getIfPresent("showMenuBetweenSessions", showMenuBetweenSessions);
			break;
		default:
			throw format("Did not recognize settings version: %d", settingsVersion);
			break;
		}

	}

	Any addToAny(Any a, const bool forceAll = false) const {
		MenuConfig def;
		if (forceAll || def.showMenuLogo != showMenuLogo)									a["showMenuLogo"] = showMenuLogo;
		if (forceAll || def.showExperimentSettings != showExperimentSettings)				a["showExperimentSettings"] = showExperimentSettings;
		if (forceAll || def.showUserSettings != showUserSettings)							a["showUserSettings"] = showUserSettings;
		if (forceAll || def.allowSessionChange != allowSessionChange)						a["allowSessionChange"] = allowSessionChange;
		if (forceAll || def.allowUserAdd != allowUserAdd)									a["allowUserAdd"] = allowUserAdd;
		if (forceAll || def.allowUserSettingsSave != allowUserSettingsSave)					a["allowUserSettingsSave"] = allowUserSettingsSave;
		if (forceAll || def.allowSensitivityChange != allowSensitivityChange)				a["allowSensitivityChange"] = allowSensitivityChange;
		if (forceAll || def.allowTurnScaleChange != allowTurnScaleChange)					a["allowTurnScaleChange"] = allowTurnScaleChange;
		if (forceAll || def.xTurnScaleAdjustMode != xTurnScaleAdjustMode)					a["xTurnScaleAdjustMode"] = xTurnScaleAdjustMode;
		if (forceAll || def.yTurnScaleAdjustMode != yTurnScaleAdjustMode)					a["yTurnScaleAdjustMode"] = yTurnScaleAdjustMode;
		if (forceAll || def.allowReticleChange != allowReticleChange)						a["allowReticleChange"] = allowReticleChange;
		if (forceAll || def.allowReticleIdxChange != allowReticleIdxChange)					a["allowReticleIdxChange"] = allowReticleIdxChange;
		if (forceAll || def.allowReticleSizeChange != allowReticleSizeChange)				a["allowReticleSizeChange"] = allowReticleSizeChange;
		if (forceAll || def.allowReticleColorChange != allowReticleColorChange)				a["allowReticleColorChange"] = allowReticleColorChange;
		if (forceAll || def.allowReticleChangeTimeChange != allowReticleChangeTimeChange)	a["allowReticleChangeTimeChange"] = allowReticleChangeTimeChange;
		if (forceAll || def.showReticlePreview != showReticlePreview)						a["showReticlePreview"] = showReticlePreview;
		if (forceAll || def.showMenuOnStartup != showMenuOnStartup)							a["showMenuOnStartup"] = showMenuOnStartup;
		if (forceAll || def.showMenuBetweenSessions != showMenuBetweenSessions)				a["showMenuBetweenSessions"] = showMenuBetweenSessions;
		return a;
	}

	bool allowAnyChange() {
		return allowSensitivityChange && allowTurnScaleChange &&
			allowReticleChange && allowReticleIdxChange && allowReticleColorChange && allowReticleSizeChange && allowReticleChangeTimeChange;
	}
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
		AnyTableReader reader(any);
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
		if(forceAll || def.scene != scene) a["scene"] = scene;
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
		a["weapon"] =  weapon.toAny(forceAll);
		return a;
	}
};

/** Configuration for a session worth of trials */
class SessionConfig : public FpsConfig {
public:
	String				id;								///< Session ID
	String				description = "Session";		///< String indicating whether session is training or real
	int					blockCount = 1;					///< Default to just 1 block per session
	Array<TrialCount>	trials;							///< Array of trials (and their counts) to be performed
	bool				closeOnComplete = false;		///< Close application on session completed?
	static FpsConfig	defaultConfig;

	SessionConfig() : FpsConfig(defaultConfig) {}

	static shared_ptr<SessionConfig> create() {
		return createShared<SessionConfig>();
	}

	/** Load from Any */
	SessionConfig(const Any& any) : FpsConfig(any, defaultConfig) {
		TrialCount::defaultCount = timing.defaultTrialCount;
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Unique session info
			reader.get("id", id, "An \"id\" field must be provided for each session!");
			reader.getIfPresent("description", description);
			reader.getIfPresent("closeOnComplete", closeOnComplete);
			reader.getIfPresent("blockCount", blockCount);
			reader.get("trials", trials, format("Issues in the (required) \"trials\" array for session: \"%s\"", id));
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
			break;
		}
	}

	Any toAny(const bool forceAll = false) const {
		// Get the base any config
		Any a = FpsConfig::toAny(forceAll);
		SessionConfig def;

		// Update w/ the session-specific fields
		a["id"] = id;
		a["description"] = description;
		if( forceAll || def.closeOnComplete != closeOnComplete )	a["closeOnComplete"] = closeOnComplete;
		if( forceAll || def.blockCount != blockCount )				a["blockCount"] = blockCount;
		a["trials"] = trials;
		return a;
	}

	/** Get the total number of trials in this session */
	float getTrialsPerBlock(void) {
		float count = 0.f;
		for (const TrialCount& tc : trials) {
			if (count < 0) {
				return finf();
			}
			else {
				count += tc.count;
			}
		}
		return count;
	}
};

/** Experiment configuration */
class ExperimentConfig : public FpsConfig {
public:
	String description = "Experiment";					///< Experiment description
	Array<SessionConfig> sessions;						///< Array of sessions
	Array<TargetConfig> targets;						///< Array of trial configs   
	bool closeOnComplete = false;						///< Close application on all sessions complete

	ExperimentConfig() { init(); }
	
	/** Load from Any */
	ExperimentConfig(const Any& any) : FpsConfig(any) {
		AnyTableReader reader(any);
		switch (settingsVersion) {
		case 1:
			// Setup the default FPS config based on this
			SessionConfig::defaultConfig = (FpsConfig)(*this);												// Setup the default configuration here
			// Experiment-specific info
			reader.getIfPresent("description", description);
			reader.getIfPresent("closeOnComplete", closeOnComplete);
			reader.get("targets", targets, "Issue in the (required) \"targets\" array for the experiment!");	// Targets must be specified for the experiment
			reader.get("sessions", sessions, "Issue in the (required) \"sessions\" array for the experiment config!");
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
			break;
		}

		init();
	}

	void init() {
		// This method handles setting up default targets and sessions when none are provided
		bool addedTargets = false;
		if (targets.size() == 0) {
			addedTargets = true;
			TargetConfig tStatic;
			tStatic.id = "static";
			tStatic.destSpace = "player";
			tStatic.speed = Array<float>({ 0.f, 0.f });
			tStatic.size = Array<float>({ 0.05f, 0.05f });
			
			targets.append(tStatic);

			TargetConfig tMove;
			tMove.id = "moving";
			tMove.destSpace = "player";
			tMove.size = Array<float>({ 0.05f, 0.05f });
			tMove.speed = Array<float>({ 7.f, 10.f });
			tMove.motionChangePeriod = Array<float>({ 0.8f, 1.5f });
			tMove.axisLock = Array<bool>({ false, false, true });
			
			targets.append(tMove);

			TargetConfig tJump;
			tJump.id = "jumping";
			tJump.destSpace = "player";
			tJump.size = Array<float>({ 0.05f, 0.05f });
			tJump.speed = Array<float>({ 10.f, 10.f });
			tJump.motionChangePeriod = Array<float>({ 0.8f, 1.5f });
			tJump.jumpEnabled = true;
			tJump.jumpSpeed = Array<float>({ 10.f, 10.f });
			tJump.jumpPeriod = Array<float>({ 0.5f, 1.0f });
			tMove.axisLock = Array<bool>({ false, false, true });
			
			targets.append(tJump);
		}
		else {
			// Targets are present (make sure no 2 have the same ID)
			Array<String> ids;
			for (TargetConfig target : targets) { 
				if (!ids.contains(target.id)) { ids.append(target.id); }
				else {
					// This is a repeat entry, throw an exception
					throw format("Found duplicate target configuration for target: \"%s\"", target.id);
				}
			}
			
		}

		if(sessions.size() == 0 && addedTargets){
			SessionConfig sess60;
			sess60.id = "60Hz";
			sess60.description = "60Hz trials";
			sess60.render.frameRate = 60.0f;
			sess60.trials = Array<TrialCount>({ TrialCount(Array<String>({ "static", "moving", "jumping" }), 2) });

			sessions.append(sess60);

			SessionConfig sess30;
			sess30.id = "30Hz";
			sess30.description = "30Hz trials";
			sess30.render.frameRate = 30.0f;
			sess30.trials = Array<TrialCount>({ TrialCount(Array<String>({ "static", "moving", "jumping" }), 2) });
			
			sessions.append(sess30);
		}
	}

	/** Get an array of session IDs */
	void getSessionIds(Array<String>& ids) const {
        ids.fastClear();
		for (const SessionConfig& session : sessions) { ids.append(session.id); }
	}

	/** Get a session config based on its ID */
	shared_ptr<SessionConfig> getSessionConfigById(const String& id) const {
		for (const SessionConfig& session : sessions) {
			if (! session.id.compare(id)) {
                return SessionConfig::createShared<SessionConfig>(session);
            }
		}
		return nullptr;
	}

	/** Get the index of a session in the session array (by ID) */
	int getSessionIndex(const String& id) const {
        debugAssert(sessions.size() >= 0 && sessions.size() < 100000);
        for (int i = 0; i < sessions.size(); ++i) {
			if (! sessions[i].id.compare(id)) {
                return i;
            }
		}
		throw format("Could not find session:\"%s\"", id);
	}
	
	/** Get a pointer to a target config by ID */
	shared_ptr<TargetConfig> getTargetConfigById(const String& id) const {
        for (const TargetConfig& target : targets) { 
			if (!target.id.compare(id)) {
				return TargetConfig::createShared<TargetConfig>(target);
			}
		}
		return nullptr;
	}

	// Get target configs by trial (not recommended for repeated calls)
	Array<Array<shared_ptr<TargetConfig>>> getTargetsByTrial(const String& id) const {
		return getTargetsByTrial(getSessionIndex(id));
	}

	// Get target configs by trial (not recommended for repeated calls)
	Array<Array<shared_ptr<TargetConfig>>> getTargetsByTrial(int sessionIndex) const {
		Array<Array<shared_ptr<TargetConfig>>> trials;
		// Iterate through the trials
		for (int i = 0; i < sessions[sessionIndex].trials.size(); i++) {
			Array<shared_ptr<TargetConfig>> targets;
			for (String id : sessions[sessionIndex].trials[i].ids) {
				const shared_ptr<TargetConfig> t = getTargetConfigById(id);
				targets.append(t);
			}
			trials.append(targets);
		}
		return trials;
	}

	// Get all targets affiliated with a session (not recommended for repeated calls)
	Array<shared_ptr<TargetConfig>> getSessionTargets(const String& id) {
		const int idx = getSessionIndex(id);		// Get session index
		Array<shared_ptr<TargetConfig>> targets;
		Array<String> loggedIds;
		for (auto trial : sessions[idx].trials) {
			for (String id : trial.ids) {
				if (!loggedIds.contains(id)) {
					loggedIds.append(id);
					targets.append(getTargetConfigById(id));
				}
			}
		}
		return targets;
	}

	Any toAny(const bool forceAll = false) const {
		// Get the base any config
		Any a = FpsConfig::toAny(forceAll);
		SessionConfig def;
		// Write the experiment configuration-specific 
		if(forceAll || def.description != description)			a["description"] = description;
		if (forceAll || def.closeOnComplete != closeOnComplete) a["closeOnComplete"] = closeOnComplete;
		a["targets"] = targets;
		a["sessions"] = sessions;
		return a;
	}

	/** Get the experiment config from file */
	static ExperimentConfig load(const String& filename) {
        // if file not found, build a default
        if (!FileSystem::exists(System::findDataFile(filename, false))) {
            ExperimentConfig ex = ExperimentConfig();
			ex.toAny().save(filename);
			SessionConfig::defaultConfig = (FpsConfig)ex;
			return ex;
		}
		return Any::fromFile(System::findDataFile(filename));
	}

	/** Print the experiment config to the log */
	void printToLog() {
		logPrintf("\n-------------------\nExperiment Config\n-------------------\nappendingDescription = %s\nscene name = %s\nTrial Feedback Duration = %f\nPretrial Duration = %f\nMax Trial Task Duration = %f\nMax Clicks = %d\n",
			description.c_str(), scene.name.c_str(), timing.trialFeedbackDuration, timing.pretrialDuration, timing.maxTrialDuration, weapon.maxAmmo);
		// Iterate through sessions and print them
		for (int i = 0; i < sessions.size(); i++) {
			SessionConfig sess = sessions[i];
			logPrintf("\t-------------------\n\tSession Config\n\t-------------------\n\tID = %s\n\tFrame Rate = %f\n\tFrame Delay = %d\n",
				sess.id.c_str(), sess.render.frameRate, sess.render.frameDelay);
			// Now iterate through each run
			for (int j = 0; j < sess.trials.size(); j++) {
				String ids;
				for (String id : sess.trials[j].ids) { ids += format("%s, ", id.c_str()); }
				if(ids.length() > 2) ids = ids.substr(0, ids.length() - 2);
				logPrintf("\t\tTrial Run Config: IDs = [%s], Count = %d\n",
					ids.c_str(), sess.trials[j].count);
			}
		}
		// Iterate through trials and print them
		for (int i = 0; i < targets.size(); i++) {
			TargetConfig target = targets[i];
			logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tUpper Hemisphere Only = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n\tAxis Lock = [%s, %s, %s]\n",
				target.id.c_str(), target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.size[0], target.size[1], target.upperHemisphereOnly ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1],
				target.axisLock[0]?"true":"false", target.axisLock[1] ? "true" : "false", target.axisLock[2] ? "true" : "false");
		}
	}
};
