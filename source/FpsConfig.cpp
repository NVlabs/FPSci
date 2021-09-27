#include "FpsConfig.h"

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

// Currently unused, allows getting scalar or vector from Any
template <class T>
static void getArrayFromAny(AnyTableReader reader, const String& name, Array<T>& output) {
	bool valid = true;
	T value;
	try {
		Array<T> arr;
		reader.get(name, arr);					// Try to read an array directly
		if (arr.size() < output.size()) {		// Check for under size (critical)
			throw format("\"%s\" array of length %d is underspecified (should be of length %d)!", name.c_str(), arr.size(), output.size());
		}
		else if (arr.size() != output.size()) {	// Check for over size (warn)
			logPrintf("WARNING: array specified for \"%s\" is of length %d, but should be %d (using first %d elements)...\n", name.c_str(), arr.size(), output.size(), output.size());
		}
		// Copy over array elements
		for (int i = 0; i < output.size(); i++) {
			output[i] = arr[i];
		}
	}
	catch(ParseError& e){
		// Failed to read array, try to duplicate a single value
		valid = reader.getIfPresent(name, value);
		if (valid) {
			for (int i = 0; i < output.size(); i++) {
				output[i] = value;
			}
		}
	}
}

// Currently unused, allows writing (single valued) vector as scalar to Any
template <class T>
static void arrayToAny(Any& a, const String& name, const Array<T>& arr) {
	bool allEqual = true;
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i] != arr[0]) {
			allEqual = false;
			break;
		}
	}
	if (allEqual) a[name] = arr[0];		// If array is constant just write a value
	else a[name] = arr;					// Otherwise write the array
}

static float tExpLambdaFromMean(float mean, float min, float max, float acceptableErr = 1e-6) {
	// Estimate lambda value from mean
	if (mean < min) throw "Error truncated exponential mean cannot be less than minimum!";
	else if (mean > max) throw "Error truncated exponential mean cannot be greater than maximum!";

	const float R = max - min;

	// Handle trivial cases (identity and uniform distributions)
	if (abs(mean - (min + R / 2)) < acceptableErr) return 0;
	else if (mean - min < acceptableErr) return 88.f;
	else if (max - mean < acceptableErr) return -88.f;

	// Iterative estimation of lambda
	float lambda = mean < min + R / 2 ? 1.f : -1.f;					// Initial guess (sign of lambda)
	float mu;
	for (int i = 0; i < 1000; i++) {							// Iterative estimation (can exit early)
		mu = 1 / lambda - R / (exp(R * lambda) - 1) + min;		// Calculate mean at this lamba value
		if (abs(mu - mean) < acceptableErr) break;				// Early exit condition
		lambda += mu - mean;									// Adjust lambda based on difference of means
	}
	return lambda;
}

SceneConfig::SceneConfig(const Any& any) {
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

Any SceneConfig::toAny(const bool forceAll) const {
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

bool SceneConfig::operator!=(const SceneConfig& other) const {
	return name != other.name ||
		//gravity != other.gravity ||
		resetHeight != other.resetHeight ||
		spawnPosition != other.spawnPosition ||
		spawnHeading != other.spawnHeading;
}

void RenderConfig::load(AnyTableReader reader, int settingsVersion) {
	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("frameRate", frameRate);
		reader.getIfPresent("frameDelay", frameDelay);
		reader.getIfPresent("horizontalFieldOfView", hFoV);

		reader.getIfPresent("resolution2D", resolution2D);
		reader.getIfPresent("resolution3D", resolution3D);
		reader.getIfPresent("resolutionComposite", resolutionComposite);

		reader.getIfPresent("shader2D", shader2D);
		reader.getIfPresent("shader3D", shader3D);
		reader.getIfPresent("shaderComposite", shaderComposite);

		reader.getIfPresent("sampled2D", sampler2D);
		reader.getIfPresent("sampler2DOutput", sampler2DOutput);
		reader.getIfPresent("sampler3D", sampler3D);
		reader.getIfPresent("sampler3DOutput", sampler3DOutput);
		reader.getIfPresent("samplerPrecomposite", samplerPrecomposite);
		reader.getIfPresent("samplerComposite", samplerComposite);
		reader.getIfPresent("samplerFinal", samplerFinal);

		break;
	default:
		throw format("Did not recognize settings version: %d", settingsVersion);
		break;
	}
}

// Implement equality check for all Any serializable sampler fields (used below)
static bool operator!=(Sampler s1, Sampler s2) {
	return s1.interpolateMode != s2.interpolateMode ||
		s1.xWrapMode != s2.xWrapMode || s1.yWrapMode != s2.yWrapMode ||
		s1.depthReadMode != s2.depthReadMode || s1.maxAnisotropy != s1.maxAnisotropy ||
		s1.maxMipMap != s2.maxMipMap || s1.minMipMap != s2.minMipMap || s1.mipBias != s2.mipBias;
}

Any RenderConfig::addToAny(Any a, bool forceAll) const {
	RenderConfig def;
	if (forceAll || def.frameRate != frameRate)					a["frameRate"] = frameRate;
	if (forceAll || def.frameDelay != frameDelay)				a["frameDelay"] = frameDelay;
	if (forceAll || def.hFoV != hFoV)							a["horizontalFieldOfView"] = hFoV;

	if (forceAll || def.resolution2D != resolution2D)			a["resolution2D"] = resolution2D;
	if (forceAll || def.resolution3D != resolution3D)			a["resolution3D"] = resolution3D;
	if (forceAll || def.resolutionComposite != resolutionComposite)	a["resolutionComposite"] = resolutionComposite;

	if (forceAll || def.shader2D != shader2D)					a["shader2D"] = shader2D;
	if (forceAll || def.shader3D != shader3D)					a["shader3D"] = shader3D;
	if (forceAll || def.shaderComposite != shaderComposite)		a["shaderComposite"] = shaderComposite;	
	
	if (forceAll || def.sampler2D != sampler2D)					a["sampler2D"] = sampler2D;
	if (forceAll || def.sampler2DOutput != sampler2DOutput)		a["sampler2DOutput"] = sampler2DOutput;
	if (forceAll || def.sampler3D != sampler3D)					a["sampler3D"] = sampler3D;
	if (forceAll || def.sampler3DOutput != sampler3DOutput)		a["sampler3DOutput"] = sampler3DOutput;
	if (forceAll || def.samplerPrecomposite != samplerPrecomposite)	a["samplerPrecomposite"] = samplerPrecomposite;
	if (forceAll || def.samplerComposite != samplerComposite)	a["samplerComposite"] = samplerComposite;
	if (forceAll || def.samplerFinal != samplerFinal)			a["samplerFinal"] = samplerFinal;
	
	return a;
}

void PlayerConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any PlayerConfig::addToAny(Any a, bool forceAll) const {
	PlayerConfig def;
	if (forceAll || def.moveRate != moveRate)			a["moveRate"] = moveRate;
	if (forceAll || def.moveScale != moveScale)			a["moveScale"] = moveScale;
	if (forceAll || def.height != height)				a["playerHeight"] = height;
	if (forceAll || def.crouchHeight != crouchHeight)	a["crouchHeight"] = crouchHeight;
	if (forceAll || def.jumpVelocity != jumpVelocity)	a["jumpVelocity"] = jumpVelocity;
	if (forceAll || def.jumpInterval != jumpInterval)	a["jumpInterval"] = jumpInterval;
	if (forceAll || def.jumpTouch != jumpTouch)			a["jumpTouch"] = jumpTouch;
	if (forceAll || def.gravity != gravity)				a["playerGravity"] = gravity;
	if (forceAll || def.axisLock != axisLock)			a["playerAxisLock"] = axisLock;
	if (forceAll || def.stillBetweenTrials != stillBetweenTrials)		a["disablePlayerMotionBetweenTrials"] = stillBetweenTrials;
	if (forceAll || def.resetPositionPerTrial != resetPositionPerTrial)	a["resetPlayerPositionBetweenTrials"] = resetPositionPerTrial;
	return a;
}

StaticHudElement::StaticHudElement(const Any& any) {
	AnyTableReader reader(any);
	reader.get("filename", filename, "Must provide filename for all Static HUD elements!");
	reader.get("position", position, "Must provide position for all static HUD elements");
	reader.getIfPresent("scale", scale);
}

Any StaticHudElement::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	a["filename"] = filename;
	a["position"] = position;
	if (forceAll || scale != Vector2(1.0, 1.0))  a["scale"] = scale;
	return a;
}

bool StaticHudElement::operator!=(const StaticHudElement& other) const {
	return filename != other.filename ||
		position != other.position ||
		scale != other.scale;
}


void HudConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any HudConfig::addToAny(Any a, bool forceAll) const {
	HudConfig def;
	if (forceAll || def.enable != enable)											a["showHUD"] = enable;
	if (forceAll || def.showBanner != showBanner)									a["showBanner"] = showBanner;
	if (forceAll || def.hudFont != hudFont)											a["hudFont"] = hudFont;
	if (forceAll || def.showPlayerHealthBar != showPlayerHealthBar)					a["showPlayerHealthBar"] = showPlayerHealthBar;
	if (forceAll || def.playerHealthBarSize != playerHealthBarSize)					a["playerHealthBarSize"] = playerHealthBarSize;
	if (forceAll || def.playerHealthBarPos != playerHealthBarPos)					a["playerHealthBarPosition"] = playerHealthBarPos;
	if (forceAll || def.playerHealthBarBorderSize != playerHealthBarBorderSize)		a["playerHealthBarBorderSize"] = playerHealthBarBorderSize;
	if (forceAll || def.playerHealthBarBorderColor != playerHealthBarBorderColor)	a["playerHealthBarBorderColor"] = playerHealthBarBorderColor;
	if (forceAll || def.playerHealthBarColors != playerHealthBarColors)				a["playerHealthBarColors"] = playerHealthBarColors;
	if (forceAll || def.showAmmo != showAmmo)										a["showAmmo"] = showAmmo;
	if (forceAll || def.ammoPosition != ammoPosition)								a["ammoPosition"] = ammoPosition;
	if (forceAll || def.ammoSize != ammoSize)										a["ammoSize"] = ammoSize;
	if (forceAll || def.ammoColor != ammoColor)										a["ammoColor"] = ammoColor;
	if (forceAll || def.ammoOutlineColor != ammoOutlineColor)						a["ammoOutlineColor"] = ammoOutlineColor;
	if (forceAll || def.renderWeaponStatus != renderWeaponStatus)					a["renderWeaponStatus"] = renderWeaponStatus;
	if (forceAll || def.weaponStatusSide != weaponStatusSide)						a["weaponStatusSide"] = weaponStatusSide;
	if (forceAll || def.cooldownMode != cooldownMode)								a["cooldownMode"] = cooldownMode;
	if (forceAll || def.cooldownInnerRadius != cooldownInnerRadius)					a["cooldownInnerRadius"] = cooldownInnerRadius;
	if (forceAll || def.cooldownThickness != cooldownThickness)						a["cooldownThickness"] = cooldownThickness;
	if (forceAll || def.cooldownSubdivisions != cooldownSubdivisions)				a["cooldownSubdivisions"] = cooldownSubdivisions;
	if (forceAll || def.cooldownColor != cooldownColor)								a["cooldownColor"] = cooldownColor;
	if (forceAll || def.staticElements != staticElements)							a["staticHUDElements"] = staticElements;
	return a;
}

void AudioConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any AudioConfig::addToAny(Any a, bool forceAll) const {
	AudioConfig def;
	if (forceAll || def.sceneHitSound != sceneHitSound)			a["sceneHitSound"] = sceneHitSound;
	if (forceAll || def.sceneHitSoundVol != sceneHitSoundVol)	a["sceneHitSoundVol"] = sceneHitSoundVol;
	return a;
}

void TimingConfig::load(AnyTableReader reader, int settingsVersion) {
	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("pretrialDuration", pretrialDuration);
		// Get pretrialDurationRange if present (indiciates a truncated exponential randomized pretrial duration)
		if (reader.getIfPresent("pretrialDurationRange", pretrialDurationRange)) {
			if (pretrialDurationRange.size() < 2) {
				throw "Must provide 2 values for \"pretrialDurationRange\"!";
			}
			else if (pretrialDurationRange[0] > pretrialDurationRange[1]) {
				throw format("pretrialDurationRange[0] (%.3f) must be less than (or equal to) pretrialDurationRange[1] (%.3f). Please re-order!", 
					pretrialDurationRange[0], pretrialDurationRange[1]).c_str();
			}
			else if (pretrialDurationRange.size() > 2) {
				logPrintf("WARNING: \"pretrialDurationRange\" should be specified as a 2-element array but has length %d (ignoring last %d values)!",
					pretrialDurationRange.size(), pretrialDurationRange.size() - 2);
			}
			if (pretrialDuration < pretrialDurationRange[0] || pretrialDuration > pretrialDurationRange[1]) {
				throw format("\"pretrialDuration\" (%.3f) must be within \"pretrialDurationRange\" ([%.3f, %.3f])!",
					pretrialDuration, pretrialDurationRange[0], pretrialDurationRange[1]).c_str();
			}
			pretrialDurationLambda = tExpLambdaFromMean(pretrialDuration, pretrialDurationRange[0], pretrialDurationRange[1]);
		}
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

Any TimingConfig::addToAny(Any a, bool forceAll) const {
	TimingConfig def;
	if (forceAll || def.pretrialDuration != pretrialDuration)				a["pretrialDuration"] = pretrialDuration;
	if (forceAll || def.pretrialDurationRange != pretrialDurationRange)		a["pretrialDurationRange"] = pretrialDurationRange;
	if (forceAll || def.maxTrialDuration != maxTrialDuration)				a["maxTrialDuration"] = maxTrialDuration;
	if (forceAll || def.trialFeedbackDuration != trialFeedbackDuration)		a["trialFeedbackDuration"] = trialFeedbackDuration;
	if (forceAll || def.sessionFeedbackDuration != sessionFeedbackDuration)	a["sessionFeedbackDuration"] = sessionFeedbackDuration;
	if (forceAll || def.clickToStart != clickToStart)						a["clickToStart"] = clickToStart;
	if (forceAll || def.sessionFeedbackRequireClick != sessionFeedbackRequireClick) a["sessionFeedbackRequireClick"] = sessionFeedbackRequireClick;
	if (forceAll || def.defaultTrialCount != defaultTrialCount)				a["defaultTrialCount"] = defaultTrialCount;
	return a;
}

void FeedbackConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any FeedbackConfig::addToAny(Any a, bool forceAll) const {
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

void TargetViewConfig::load(AnyTableReader reader, int settingsVersion) {
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
		reader.getIfPresent("clearMissDecalsWithReference", clearDecalsWithRef);
		reader.getIfPresent("showPreviewTargetsWithReference", previewWithRef);
		reader.getIfPresent("showReferenceTargetMissDecals", showRefDecals);
		reader.getIfPresent("previewTargetColor", previewColor);
		break;
	default:
		throw format("Did not recognize settings version: %d", settingsVersion);
		break;
	}
}

Any TargetViewConfig::addToAny(Any a, bool forceAll) const {
	TargetViewConfig def;
	if (forceAll || def.showHealthBars != showHealthBars)				a["showTargetHealthBars"] = showHealthBars;
	if (forceAll || def.healthBarSize != healthBarSize)					a["targetHealthBarSize"] = healthBarSize;
	if (forceAll || def.healthBarOffset != healthBarOffset)				a["targetHealthBarOffset"] = healthBarOffset;
	if (forceAll || def.healthBarBorderSize != healthBarBorderSize)		a["targetHealthBarBorderSize"] = healthBarBorderSize;
	if (forceAll || def.healthBarBorderColor != healthBarBorderColor)	a["targetHealthBarBorderColor"] = healthBarBorderColor;
	if (forceAll || def.healthColors != healthColors)					a["targetHealthColors"] = healthColors;
	if (forceAll || def.healthBarColors != healthBarColors)				a["targetHealthBarColors"] = healthBarColors;
	if (forceAll || def.showCombatText != showCombatText)				a["showFloatingCombatText"] = showCombatText;
	if (forceAll || def.combatTextSize != combatTextSize)				a["floatingCombatTextSize"] = combatTextSize;
	if (forceAll || def.combatTextFont != combatTextFont)				a["floatingCombatTextFont"] = combatTextFont;
	if (forceAll || def.combatTextColor != combatTextColor)				a["floatingCombatTextColor"] = combatTextColor;
	if (forceAll || def.combatTextOutline != combatTextOutline)			a["floatingCombatTextOutlineColor"] = combatTextOutline;
	if (forceAll || def.combatTextOffset != combatTextOffset)			a["floatingCombatTextOffset"] = combatTextOffset;
	if (forceAll || def.combatTextVelocity != combatTextVelocity)		a["floatingCombatTextVelocity"] = combatTextVelocity;
	if (forceAll || def.combatTextFade != combatTextFade)				a["floatingCombatTextFade"] = combatTextFade;
	if (forceAll || def.combatTextTimeout != combatTextTimeout)			a["floatingCombatTextTimeout"] = combatTextTimeout;
	if (forceAll || def.showRefTarget != showRefTarget)					a["showRefTarget"] = showRefTarget;
	if (forceAll || def.refTargetSize != refTargetSize)					a["referenceTargetSize"] = refTargetSize;
	if (forceAll || def.refTargetColor != refTargetColor)				a["referenceTargetColor"] = refTargetColor;
	if (forceAll || def.clearDecalsWithRef != clearDecalsWithRef)		a["clearMissDecalsWithReference"] = clearDecalsWithRef;
	if (forceAll || def.previewWithRef != previewWithRef)				a["showPreviewTargetsWithReference"] = previewWithRef;
	if (forceAll || def.showRefDecals != showRefDecals)					a["showReferenceTargetMissDecals"] = showRefDecals;
	if (forceAll || def.previewColor != previewColor)					a["previewTargetColor"] = previewColor;
	return a;
}

void ClickToPhotonConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any ClickToPhotonConfig::addToAny(Any a, bool forceAll) const {
	ClickToPhotonConfig def;
	if (forceAll || def.enabled != enabled)		a["renderClickPhoton"] = enabled;
	if (forceAll || def.side != side)			a["clickPhotonSide"] = side;
	if (forceAll || def.size != size)			a["clickPhotonSize"] = size;
	if (forceAll || def.vertPos != vertPos)		a["clickPhotonVertPos"] = vertPos;
	if (forceAll || def.colors != colors)		a["clickPhotonColors"] = colors;
	return a;
}

void LoggerConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any LoggerConfig::addToAny(Any a, bool forceAll) const {
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

CommandSpec::CommandSpec(const Any& any) {
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

Any CommandSpec::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	CommandSpec def;
	a["command"] = cmdStr;
	if (forceAll || def.foreground != foreground)	a["foreground"] = foreground;
	if (forceAll || def.blocking != blocking)		a["blocking"] = blocking;
	return a;
}

void CommandConfig::load(AnyTableReader reader, int settingsVersion) {
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

Any CommandConfig::addToAny(Any a, const bool forceAll) const {
	if (forceAll || sessionStartCmds.size() > 0)		a["commandsOnSessionStart"] = sessionStartCmds;
	if (forceAll || sessionEndCmds.size() > 0)			a["commandsOnSessionEnd"] = sessionEndCmds;
	if (forceAll || trialStartCmds.size() > 0)			a["commandsOnTrialStart"] = trialStartCmds;
	if (forceAll || trialEndCmds.size() > 0)			a["commandsOnTrialEnd"] = trialEndCmds;
	return a;
}

Question::Question(const Any& any) {
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
		reader.getIfPresent("randomOrder", randomOrder);

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