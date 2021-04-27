/** \file FPSciApp.cpp */
#include "FPSciApp.h"
#include "Dialogs.h"
#include "Logger.h"
#include "Session.h"
#include "PhysicsScene.h"
#include "WaypointManager.h"
#include <chrono>

// Storage for configuration static vars
FpsConfig SessionConfig::defaultConfig;
int TrialCount::defaultCount;
Array<String> UserSessionStatus::defaultSessionOrder;
bool UserSessionStatus::randomizeDefaults;

StartupConfig FPSciApp::startupConfig;

FPSciApp::FPSciApp(const GApp::Settings& settings) : GApp(settings) {}

/** Initialize the app */
void FPSciApp::onInit() {
	// Seed random based on the time
	Random::common().reset(uint32(time(0)));

	GApp::onInit();			// Initialize the G3D application (one time)
	startupConfig.validateExperiments();
	initExperiment();		// Initialize the experiment
}

void FPSciApp::initExperiment(){
	// Load config from files
	loadConfigs(startupConfig.experimentList[experimentIdx]);
	m_lastSavedUser = *currentUser();			// Copy over the startup user for saves

	// Get the size of the primary display
	displayRes = OSWindow::primaryDisplaySize();						

	// Setup the display mode
	setSubmitToDisplayMode(
		//SubmitToDisplayMode::EXPLICIT);
		SubmitToDisplayMode::MINIMIZE_LATENCY);
		//SubmitToDisplayMode::BALANCE);
	    //SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);

	// Set the initial simulation timestep to REAL_TIME. The desired timestep is set later.
	setFrameDuration(frameDuration(), REAL_TIME);
	m_lastOnSimulationRealTime = 0.0;

	// Setup/update waypoint manager
	if (startupConfig.developerMode && startupConfig.waypointEditorMode) {
		waypointManager = WaypointManager::create(this);
	}

	// Setup the scene
	setScene(PhysicsScene::create(m_ambientOcclusion));
	scene()->registerEntitySubclass("PlayerEntity", &PlayerEntity::create);			// Register the player entity for creation
	scene()->registerEntitySubclass("FlyingEntity", &FlyingEntity::create);			// Register the target entity for creation

	weapon = Weapon::create(&experimentConfig.weapon, scene(), activeCamera());
	weapon->setHitCallback(std::bind(&FPSciApp::hitTarget, this, std::placeholders::_1));
	weapon->setMissCallback(std::bind(&FPSciApp::missEvent, this));

	// Load models and set the reticle
	loadModels();
	setReticle(currentUser()->reticleIndex);

	// Load fonts and images
	outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	hudTextures.set("scoreBannerBackdrop", Texture::fromFile(System::findDataFile("gui/scoreBannerBackdrop.png")));

	// Setup the GUI
	showRenderingStats = false;
	makeGUI();

	updateMouseSensitivity();				// Update (apply) mouse sensitivity
	const Array<String> sessions = m_userSettingsWindow->updateSessionDropDown();	// Update the session drop down to remove already completed sessions
	updateSession(sessions[0], true);		// Update session to create results file/start collection
}

void FPSciApp::toggleUserSettingsMenu() {
	if (!m_userSettingsWindow->visible()) {
		openUserSettingsWindow();
	}
	else {
		closeUserSettingsWindow();
	}
	// Make sure any change to sensitivity are applied
	updateMouseSensitivity();
}

/** Handle the user settings window visibility */
void FPSciApp::openUserSettingsWindow() {
	// set focus so buttons properly highlight
	moveToCenter(m_userSettingsWindow);
    m_userSettingsWindow->setVisible(true);
	if (!dialog) {												// Don't allow the user menu to hide the mouse when a dialog box is open
		setMouseInputMode(MouseInputMode::MOUSE_CURSOR);		// Set mouse mode to cursor to allow pointer-based interaction
	}
	m_widgetManager->setFocusedWidget(m_userSettingsWindow);
}

/** Handle the user settings window visibility */
void FPSciApp::closeUserSettingsWindow() {
	if (sessConfig->menu.allowUserSettingsSave) {		// If the user could have saved their settings
		saveUserConfig(true);							// Save the user config (if it has changed) whenever this window is closed
	}
	if (!dialog) {										// Don't allow the user menu to hide the mouse when a dialog box is open
		setMouseInputMode(MouseInputMode::MOUSE_FPM);	// Set mouse mode to FPM to allow steering the view again
	}
	m_userSettingsWindow->setVisible(false);
}

void FPSciApp::saveUserConfig(bool onDiff) {
	// Check for save on diff, without mismatch
	if (onDiff && m_lastSavedUser == *currentUser()) return;
	if (notNull(sess->logger)) {
		sess->logger->logUserConfig(*currentUser(), sessConfig->id, sessConfig->player.turnScale);
	}
	userTable.save(startupConfig.experimentList[experimentIdx].userConfigFilename);
	m_lastSavedUser = *currentUser();			// Copy over this user
	logPrintf("User table saved.\n");			// Print message to log
}

void FPSciApp::saveUserStatus(void) {
	userStatusTable.save(startupConfig.experimentList[experimentIdx].userStatusFilename);
	logPrintf("User status saved.\n");
}

/** Update the mouse mode/sensitivity */
void FPSciApp::updateMouseSensitivity() {
	const shared_ptr<UserConfig> user = currentUser();
	// Converting from mouseDPI (dots/in) and sensitivity (cm/turn) into rad/dot which explains cm->in (2.54) and turn->rad (2*PI) factors
	// rad/dot = rad/cm * cm/dot = 2PI / (cm/turn) * 2.54 / (dots/in) = (2.54 * 2PI)/ (DPI * cm/360)
	const double cmp360 = 36.0 / user->mouseDegPerMm;
	const double radiansPerDot = 2.0 * pi() * 2.54 / (cmp360 * user->mouseDPI);
	const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());

	// Control player motion using the experiment config parameter
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
	if (notNull(player)) {
		player->m_cameraRadiansPerMouseDot = (float)radiansPerDot;
		player->turnScale = currentTurnScale();
	}

	m_userSettingsWindow->updateCmp360();
}

void FPSciApp::setMouseInputMode(MouseInputMode mode) {
	const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
	switch (mode) {
	case MouseInputMode::MOUSE_CURSOR:
		fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);	// Display cursor in right button mode (only holding right mouse rotates view)
		break;
	case MouseInputMode::MOUSE_DISABLED:								// Disabled case uses direct mode, but ignores view changes
	case MouseInputMode::MOUSE_FPM:										// FPM is "direct mode" wherein view is steered by mouse
		fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
		break;
	}
	m_mouseInputMode = mode;
}

void FPSciApp::loadConfigs(const ConfigFiles& configs) {
	// Load experiment setting from file
	experimentConfig = ExperimentConfig::load(configs.experimentConfigFilename);
	experimentConfig.printToLog();
	experimentConfig.validate(true);

	// Get hash for experimentconfig.Any file
	const size_t hash = HashTrait<String>::hashCode(experimentConfig.toAny().unparse());		// Hash the serialized Any (don't consider formatting)
	m_expConfigHash = format("%x", hash);														// Store the hash as a hex string
	logPrintf("Experiment hash: %s\r\n", m_expConfigHash);										// Write to log

	Array<String> sessionIds;
	experimentConfig.getSessionIds(sessionIds);

	// Load per user settings from file
	userTable = UserTable::load(configs.userConfigFilename);
	userTable.printToLog();

	// Load per experiment user settings from file and make sure they are valid
	userStatusTable = UserStatusTable::load(configs.userStatusFilename);
	userStatusTable.printToLog();
	userStatusTable.validate(sessionIds, userTable.getIds());
		
	// Get info about the system
	SystemInfo info = SystemInfo::get();
	info.printToLog();										// Print system info to log.txt

	// Get system configuration
	systemConfig = SystemConfig::load(configs.systemConfigFilename);
	systemConfig.printToLog();			// Print the latency logger config to log.txt	

	// Load the key binds
	keyMap = KeyMapping::load(configs.keymapConfigFilename);
	userInput->setKeyMapping(&keyMap.uiMap);
}

void FPSciApp::loadModels() {
	if ((experimentConfig.weapon.renderModel || startupConfig.developerMode) && !experimentConfig.weapon.modelSpec.filename.empty()) {
		// Load the model if we (might) need it
		weapon->loadModels();
	}

	// Add all the unqiue targets to this list
	Table<String, Any> targetsToBuild;
	Table<String, String> explosionsToBuild;
	Table<String, float> explosionScales;
	for (TargetConfig target : experimentConfig.targets) {
		targetsToBuild.set(target.id, target.modelSpec);
		explosionsToBuild.set(target.id, target.destroyDecal);
		explosionScales.set(target.id, target.destroyDecalScale);
	}
	// Append the basic model automatically (used for reference targets for now)
	targetsToBuild.set("reference", PARSE_ANY(ArticulatedModel::Specification{
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
	}));
	explosionsToBuild.set("reference", "explosion_01.png");
	explosionScales.set("reference", 1.0);

	// Scale the models into the m_targetModel table
	for (String id : targetsToBuild.getKeys()) {
		// Get the any specs
		Any tSpec = targetsToBuild.get(id);
		Any explosionSpec = Any::parse(format(
			"ArticulatedModel::Specification {\
				filename = \"ifs/square.ifs\";\
				preprocess = {\
					transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));\
					setMaterial(all(), UniversalMaterial::Specification{\
						lambertian = Texture::Specification {\
							filename = \"%s\";\
							encoding = Color3(1, 1, 1);\
						};\
					});\
				};\
			}", explosionsToBuild.get(id).c_str()));

		// Get the bounding box to scale to size rather than arbitrary factor
		shared_ptr<ArticulatedModel> size_model = ArticulatedModel::create(ArticulatedModel::Specification(tSpec));
		AABox bbox;
		size_model->getBoundingBox(bbox);
		Vector3 extent = bbox.extent();
		logPrintf("%20s bounding box: [%2.2f, %2.2f, %2.2f]\n", id.c_str(), extent[0], extent[1], extent[2]);
		const float default_scale = 1.0f / extent[0];					// Setup scale so that default model is 1m across

		// Create the target/explosion models for this target
		Array<shared_ptr<ArticulatedModel>> tModels, expModels;
		for (int i = 0; i <= TARGET_MODEL_SCALE_COUNT; ++i) {
			const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - TARGET_MODEL_ARRAY_OFFSET);
			tSpec.set("scale", scale*default_scale);
			explosionSpec.set("scale", (20.0 * scale * explosionScales.get(id)));
			tModels.push(ArticulatedModel::create(tSpec));
			expModels.push(ArticulatedModel::create(explosionSpec));
		}
		targetModels.set(id, tModels);
		m_explosionModels.set(id, expModels);
	}

	// Create a series of colored materials to choose from for target health
	m_materials.clear();
	for (int i = 0; i < m_MatTableSize; i++) {
		float complete = (float)i / m_MatTableSize;
		Color3 color = experimentConfig.targetView.healthColors[0] * complete + experimentConfig.targetView.healthColors[1] * (1.0f - complete);
		UniversalMaterial::Specification materialSpecification;
		materialSpecification.setLambertian(Texture::Specification(color));
		materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
		materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));
		m_materials.append(UniversalMaterial::create(materialSpecification));
	}
}

void FPSciApp::updateControls(bool firstSession) {
	// Update the user settings window
	updateUserMenu = true;
	if(!firstSession) m_showUserMenu = sessConfig->menu.showMenuBetweenSessions;

	// Update the waypoint manager
	if (startupConfig.waypointEditorMode) { waypointManager->updateControls(); }

	// Update the player controls
	bool visible = false;
	Rect2D rect;
	if (notNull(m_playerControls)) {
		visible = m_playerControls->visible();
		rect = m_playerControls->rect();
		removeWidget(m_playerControls);
	}
	m_playerControls = PlayerControls::create(*sessConfig, std::bind(&FPSciApp::exportScene, this), theme);
	m_playerControls->setVisible(visible);
	if (!rect.isEmpty()) m_playerControls->setRect(rect);
	addWidget(m_playerControls);

	// Update the render controls
	visible = false;
	rect = Rect2D();
	if (notNull(m_renderControls)) {
		visible = m_renderControls->visible(); 
		rect = m_renderControls->rect();
		removeWidget(m_renderControls);
	}
	m_renderControls = RenderControls::create(this, *sessConfig, renderFPS, emergencyTurbo, numReticles, sceneBrightness, theme, MAX_HISTORY_TIMING_FRAMES);
	m_renderControls->setVisible(visible);
	if (!rect.isEmpty()) m_renderControls->setRect(rect);
	addWidget(m_renderControls);

	// Update the weapon controls
	visible = false;
	rect = Rect2D();
	if (notNull(m_weaponControls)) {
		visible = m_weaponControls->visible();
		rect = m_weaponControls->rect();
		removeWidget(m_weaponControls);
	}
	m_weaponControls = WeaponControls::create(sessConfig->weapon, theme);
	m_weaponControls->setVisible(visible);
	if (!rect.isEmpty()) m_weaponControls->setRect(rect);
	addWidget(m_weaponControls);
}

void FPSciApp::makeGUI() {

	theme = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"));
	debugWindow->setVisible(startupConfig.developerMode);

	if (startupConfig.developerMode) {
		developerWindow->cameraControlWindow->setVisible(startupConfig.developerMode);
		developerWindow->videoRecordDialog->setEnabled(true);
		developerWindow->videoRecordDialog->setCaptureGui(true);

		// Update the scene editor (for new PhysicsScene pointer, initially loaded in GApp)
		removeWidget(developerWindow->sceneEditorWindow);
		developerWindow->sceneEditorWindow = SceneEditorWindow::create(this, scene(), theme);
		developerWindow->sceneEditorWindow->moveTo(developerWindow->cameraControlWindow->rect().x0y1() + Vector2(0, 15));
		developerWindow->sceneEditorWindow->setVisible(startupConfig.developerMode);
	}

	// Open sub-window buttons here (menu-style)
	debugPane->removeAllChildren();
	debugPane->beginRow(); {
		debugPane->addButton("Render Controls [1]", this, &FPSciApp::showRenderControls);
		debugPane->addButton("Player Controls [2]", this, &FPSciApp::showPlayerControls);
		debugPane->addButton("Weapon Controls [3]", this, &FPSciApp::showWeaponControls);
		if(startupConfig.waypointEditorMode) debugPane->addButton("Waypoint Manager [4]", waypointManager, &WaypointManager::showWaypointWindow);
	}debugPane->endRow();

	// Create the user settings window
	if (notNull(m_userSettingsWindow)) { removeWidget(m_userSettingsWindow); }
	m_userSettingsWindow = UserMenu::create(this, userTable, userStatusTable, sessConfig->menu, theme, Rect2D::xywh(0.0f, 0.0f, 10.0f, 10.0f));
	addWidget(m_userSettingsWindow);
	openUserSettingsWindow();

	// Setup the debug window
	debugWindow->pack();
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->renderDevice()->viewport().width(), debugWindow->rect().height()));
	m_debugMenuHeight = startupConfig.developerMode ? debugWindow->rect().height() : 0.0f;

	// Add the control panes here
	updateControls();
	m_showUserMenu = experimentConfig.menu.showMenuOnStartup;
}

void FPSciApp::exportScene() {
	CFrame frame = scene()->typedEntity<PlayerEntity>("player")->frame();
	logPrintf("Player position is: [%f, %f, %f]\n", frame.translation.x, frame.translation.y, frame.translation.z);
	String filename = Scene::sceneNameToFilename(sessConfig->scene.name);
	scene()->toAny().save(filename);
}

void FPSciApp::showPlayerControls() {
	m_playerControls->setVisible(true);
}

void FPSciApp::showRenderControls() {
	m_renderControls->setVisible(true);
}

void FPSciApp::showWeaponControls() {
	m_weaponControls->setVisible(true);
}

void FPSciApp::presentQuestion(Question question) {
	if (notNull(dialog)) removeWidget(dialog);		// Remove the current dialog widget (if valid)
	currentQuestion = question;						// Store this for processing key-bound presses
	Array<String> options = question.options;		// Make a copy of the options (to add key binds if necessary)
	const Rect2D windowRect = window()->clientRect();
	const Point2 size = question.fullscreen ? Point2(windowRect.width(), windowRect.height()) : Point2(400, 200);
	switch (question.type) {
	case Question::Type::MultipleChoice:
		if (question.optionKeys.length() > 0) {		// Add key-bound option to the dialog
			for (int i = 0; i < options.length(); i++) { options[i] += format(" (%s)", question.optionKeys[i].toString()); }
		}
		dialog = SelectionDialog::create(question.prompt, options, theme, question.title, question.showCursor, 3, size, !question.fullscreen);
		break;
	case Question::Type::Entry:
		dialog = TextEntryDialog::create(question.prompt, theme, question.title, false, size, !question.fullscreen);
		break;
	case Question::Type::Rating:
		if (question.optionKeys.length() > 0) {		// Add key-bound option to the dialog
			for (int i = 0; i < options.length(); i++) { options[i] += format(" (%s)", question.optionKeys[i].toString()); }
		}
		dialog = RatingDialog::create(question.prompt, options, theme, question.title, question.showCursor, size, !question.fullscreen);
		break;
	default:
		throw "Unknown question type!";
		break;
	}

	moveToCenter(dialog);
	addWidget(dialog);
	setMouseInputMode(question.showCursor ? MouseInputMode::MOUSE_CURSOR : MouseInputMode::MOUSE_DISABLED);
}

void FPSciApp::markSessComplete(String sessId) {
	if (notNull(m_pyLogger)) {
		m_pyLogger->mergeLogToDb();
	}
	// Add the session id to completed session array and save the user status table
	userStatusTable.addCompletedSession(userStatusTable.currentUser, sessId);
	saveUserStatus();
	logPrintf("Marked session: %s complete for user %s.\n", sessId, userStatusTable.currentUser);

	// Update the session drop-down to remove this session
	m_userSettingsWindow->updateSessionDropDown();
}

void FPSciApp::updateParameters(int frameDelay, float frameRate) {
	// Apply frame lag
	displayLagFrames = frameDelay;
	lastSetFrameRate = frameRate;
	// Set a maximum *finite* frame rate
	float dt = 0;
	if (frameRate > 0) dt = 1.0f / frameRate;
	else dt = 1.0f / float(window()->settings().refreshRate);
	// Update the desired realtime framerate, leaving the simulation timestep as it were (likely REAL_TIME)
	setFrameDuration(dt, simStepDuration());
}

void FPSciApp::updateShaderBuffers() {
	// Parameters for update/resize of buffers
	const int width = renderDevice->width();
	const int height = renderDevice->height();
	const ImageFormat* framebufferFormat = m_framebuffer->texture(0)->format();

	// 2D buffers (input and output)
	if (sessConfig->render.split2DBuffer) {
		m_buffer2D = Framebuffer::create(Texture::createEmpty("FPSci::2DBuffer", width, height));
		if (!sessConfig->render.shader2D.empty()) {
			m_shader2DOutput = Framebuffer::create(Texture::createEmpty("FPSci::2DShaderPass::Output", width, height));
		}
	}

	// 3D shader output (use popped framebuffer as input)
	if (!sessConfig->render.shader3D.empty()) {
		m_shader3DOutput = Framebuffer::create(Texture::createEmpty("FPSci::3DShaderPass::Output", width, height, framebufferFormat));
	}

	// Composite buffer (input and output)
	if (!sessConfig->render.shaderComposite.empty()) {
		m_bufferComposite = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Input", width, height, framebufferFormat));
		m_shaderCompositeOutput = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Output", width, height, framebufferFormat));
	}
}

void FPSciApp::initPlayer() {
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
	shared_ptr<PhysicsScene> pscene = typedScene<PhysicsScene>();

	// Set gravity and camera field of view
	Vector3 grav = experimentConfig.player.gravity;
	float FoV = experimentConfig.render.hFoV;
	if (notNull(sessConfig)) {
		grav = sessConfig->player.gravity;
		FoV = sessConfig->render.hFoV;
	}
	pscene->setGravity(grav);
	playerCamera->setFieldOfView(FoV * units::degrees(), FOVDirection::HORIZONTAL);

	// For now make the player invisible (prevent issues w/ seeing model from inside)
	player->setVisible(false);

	// Set the reset height
	float resetHeight = sessConfig->scene.resetHeight;
	if (isnan(resetHeight)) {
		float resetHeight = pscene->resetHeight();
		if (isnan(resetHeight)) {
			resetHeight = -1e6;
		}
	}
	player->setRespawnHeight(resetHeight);

	// Set player respawn location
	Point3 spawnPosition = sessConfig->scene.spawnPosition;
	if (isnan(spawnPosition.x)) {
		spawnPosition = player->frame().translation;
	}
	player->setRespawnPosition(spawnPosition);

	// Set player values from session config
	player->moveRate = &sessConfig->player.moveRate;
	player->moveScale = &sessConfig->player.moveScale;
	player->axisLock = &sessConfig->player.axisLock;
	player->jumpVelocity = &sessConfig->player.jumpVelocity;
	player->jumpInterval = &sessConfig->player.jumpInterval;
	player->jumpTouch = &sessConfig->player.jumpTouch;
	player->height = &sessConfig->player.height;
	player->crouchHeight = &sessConfig->player.crouchHeight;

	// Respawn player
	player->respawn();
	updateMouseSensitivity();

	// Set initial heading for session
	sess->initialHeadingRadians = player->heading();
}

void FPSciApp::updateSession(const String& id, bool forceReload) {
	// Check for a valid ID (non-emtpy and 
	Array<String> ids;
	experimentConfig.getSessionIds(ids);
	if (!id.empty() && ids.contains(id)) {
		// Load the session config specified by the id
		sessConfig = experimentConfig.getSessionConfigById(id);
		logPrintf("User selected session: %s. Updating now...\n", id);
		m_userSettingsWindow->setSelectedSession(id);
		// Create the session based on the loaded config
		sess = Session::create(this, sessConfig);
	}
	else {
		// Create an empty session
		sessConfig = SessionConfig::create();
		sess = Session::create(this);
	}

	// Update the controls for this session
	updateControls(m_firstSession);				// If first session consider showing the menu

	// Update the frame rate/delay
	updateParameters(sessConfig->render.frameDelay, sessConfig->render.frameRate);

	// Handle buffer setup here
	updateShaderBuffers();

	// Update shader parameters
	m_startTime = System::time();
	m_last2DTime = m_startTime;
	m_last3DTime = m_startTime;
	m_lastCompositeTime = m_startTime;
	m_frameNumber = 0;

	// Load (session dependent) fonts
	hudFont = GFont::fromFile(System::findDataFile(sessConfig->hud.hudFont));
	m_combatFont = GFont::fromFile(System::findDataFile(sessConfig->targetView.combatTextFont));

	// Handle clearing the targets here (clear any remaining targets before loading a new scene)
	if (notNull(scene())) sess->clearTargets();

	// Load the experiment scene if we haven't already (target only)
	if (sessConfig->scene.name.empty()) {
		// No scene specified, load default scene
		if (m_loadedScene.name.empty() || forceReload) {
			loadScene(m_defaultSceneName);
			m_loadedScene.name = m_defaultSceneName;
		}
		// Otherwise let the loaded scene persist
	}
	else if (sessConfig->scene != m_loadedScene || forceReload) {
		loadScene(sessConfig->scene.name);
		m_loadedScene = sessConfig->scene;
	}

	// Check for play mode specific parameters
	if (notNull(weapon)) weapon->clearDecals();
	weapon->setConfig(&sessConfig->weapon);
	weapon->setScene(scene());
	weapon->setCamera(activeCamera());

	// Update weapon model (if drawn) and sounds
	weapon->loadModels();
	weapon->loadSounds();
	if (!sessConfig->audio.sceneHitSound.empty()) {
		m_sceneHitSound = Sound::create(System::findDataFile(sessConfig->audio.sceneHitSound));
	}

	// Update shader table
	m_shaderTable.clear();
	if (!sessConfig->render.shader3D.empty()) {
		m_shaderTable.set(sessConfig->render.shader3D, G3D::Shader::getShaderFromPattern(sessConfig->render.shader3D));
	}
	if (!sessConfig->render.shader2D.empty()) {
		m_shaderTable.set(sessConfig->render.shader2D, G3D::Shader::getShaderFromPattern(sessConfig->render.shader2D));
	}
	if (!sessConfig->render.shaderComposite.empty()) {
		m_shaderTable.set(sessConfig->render.shaderComposite, G3D::Shader::getShaderFromPattern(sessConfig->render.shaderComposite));
	}

	// Load static HUD textures
	for (StaticHudElement element : sessConfig->hud.staticElements) {
		hudTextures.set(element.filename, Texture::fromFile(System::findDataFile(element.filename)));
	}

	// Create a series of colored materials to choose from for target health
	m_materials.clear();
	for (int i = 0; i < m_MatTableSize; i++) {
		float complete = (float)i / m_MatTableSize;
		Color3 color = sessConfig->targetView.healthColors[0] * complete + sessConfig->targetView.healthColors[1] * (1.0f - complete);
		UniversalMaterial::Specification materialSpecification;
		materialSpecification.setLambertian(Texture::Specification(color));
		materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
		materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));
		m_materials.append(UniversalMaterial::create(materialSpecification));
	}

	// Player parameters
	initPlayer();

	const String resultsDirPath = startupConfig.experimentList[experimentIdx].resultsDirPath;

	// Check for need to start latency logging and if so run the logger now
	if (!FileSystem::isDirectory(resultsDirPath)) {
		FileSystem::createDirectory(resultsDirPath);
	}

	const String logName = sessConfig->logger.logToSingleDb ? 
		resultsDirPath + experimentConfig.description + "_" + userStatusTable.currentUser + "_" + m_expConfigHash :
		resultsDirPath + id + "_" + userStatusTable.currentUser + "_" + String(FPSciLogger::genFileTimestamp());

	if (systemConfig.hasLogger) {
		if (!sessConfig->clickToPhoton.enabled) {
			logPrintf("WARNING: Using a click-to-photon logger without the click-to-photon region enabled!\n\n");
		}
		if (m_pyLogger == nullptr) {
			m_pyLogger = PythonLogger::create(systemConfig.loggerComPort, systemConfig.hasSync, systemConfig.syncComPort);
		}
		else {
			// Handle running logger if we need to (terminate then merge results)
			m_pyLogger->mergeLogToDb();
		}
		// Run a new logger if we need to (include the mode to run in here...)
		m_pyLogger->run(logName, sessConfig->clickToPhoton.mode);
	}

	// Initialize the experiment (this creates the results file)
	sess->onInit(logName+".db", experimentConfig.description + "/" + sessConfig->description);

	// Don't create a results file for a user w/ no sessions left
	if (m_userSettingsWindow->sessionsForSelectedUser() == 0) {
		logPrintf("No sessions remaining for selected user.\n");
	}
	else {
		logPrintf("Created results file: %s.db\n", logName.c_str());
	}

	if (m_firstSession) {
		m_firstSession = false;
	}
}

void FPSciApp::quitRequest() {
	// End session logging
	if (notNull(sess)) {
		sess->endLogging();
	}
	// Merge Python log into session log (if logging)
	if (notNull(m_pyLogger)) {
		m_pyLogger->mergeLogToDb(true);
	}
    setExitCode(0);
}

void FPSciApp::onAfterLoadScene(const Any& any, const String& sceneName) {

	// Make sure the scene has a "player" entity
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");

	// Add a player if one isn't present in the scene
	if (isNull(player)) {
		logPrintf("WARNING: Didn't find a \"player\" specified in \"%s\"! Adding one at the origin.", sceneName);
		shared_ptr<Entity> newPlayer = PlayerEntity::create("player", scene().get(), CFrame(), nullptr);
		scene()->insert(newPlayer);
	}

	// Set the active camera to the player
	const String pcamName = sessConfig->scene.playerCamera;
	playerCamera = pcamName.empty() ? scene()->defaultCamera() : scene()->typedEntity<Camera>(sessConfig->scene.playerCamera);
	alwaysAssertM(notNull(playerCamera), format("Scene %s does not contain a camera named \"%s\"!", sessConfig->scene.name, sessConfig->scene.playerCamera));
	setActiveCamera(playerCamera);

	initPlayer();

	if (weapon) {
		weapon->setScene(scene());
		weapon->setCamera(playerCamera);
	}
}

void FPSciApp::onAI() {
	GApp::onAI();
	// Add non-simulation game logic and AI code here
}


void FPSciApp::onNetwork() {
	GApp::onNetwork();
	// Poll net messages here
}


void FPSciApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) {

    if (displayLagFrames > 0) {
		// Need one more frame in the queue than we have frames of delay, to hold the current frame
		if (m_ldrDelayBufferQueue.size() <= displayLagFrames) {
			// Allocate new textures
			for (int i = displayLagFrames - m_ldrDelayBufferQueue.size(); i >= 0; --i) {
				m_ldrDelayBufferQueue.push(Framebuffer::create(Texture::createEmpty(format("Delay buffer %d", m_ldrDelayBufferQueue.size()), rd->width(), rd->height(), ImageFormat::RGB8())));
			}
			debugAssert(m_ldrDelayBufferQueue.size() == displayLagFrames + 1);
		}

		// When the display lag changes, we must be sure to be within range
		m_currentDelayBufferIndex = min(displayLagFrames, m_currentDelayBufferIndex);

		rd->pushState(m_ldrDelayBufferQueue[m_currentDelayBufferIndex]);
	}

	scene()->lightingEnvironment().ambientOcclusionSettings.enabled = !emergencyTurbo;
	playerCamera->filmSettings().setAntialiasingEnabled(!emergencyTurbo);
	playerCamera->filmSettings().setBloomStrength(emergencyTurbo ? 0.0f : 0.5f);

	GApp::onGraphics3D(rd, surface);

	if (displayLagFrames > 0) {
		// Display the delayed frame
		rd->popState();
		// Render into the frame buffer or the composite shader input buffer (if provided)
		rd->push2D(); {
			// Advance the pointer to the next, which is also the oldest frame
			m_currentDelayBufferIndex = (m_currentDelayBufferIndex + 1) % (displayLagFrames + 1);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrDelayBufferQueue[m_currentDelayBufferIndex]->texture(0), Sampler::buffer());
		} rd->pop2D();
	}
}

void FPSciApp::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// TODO: this should eventually probably use sdt instead of rdt
	RealTime currentRealTime;
	if (m_lastOnSimulationRealTime == 0) {
		m_lastOnSimulationRealTime = System::time();			// Grab the current system time if uninitialized
		currentRealTime = m_lastOnSimulationRealTime;			// Set this equal to the current system time
	}
	else {
		currentRealTime = m_lastOnSimulationRealTime + rdt;		// Increment the time by the current real time delta
	}

	bool stateCanFire = sess->currentState == PresentationState::trialTask && !m_userSettingsWindow->visible();

	// These variables will be used to fire after the various weapon styles populate them below
	int numShots = 0;
	float damagePerShot = weapon->damagePerShot();
	RealTime newLastFireTime = currentRealTime;

	if (shootButtonJustPressed && stateCanFire && !weapon->canFire(currentRealTime)) {
		// Invalid click since the weapon isn't ready to fire
		sess->accumulatePlayerAction(PlayerActionType::Invalid);
	}
	else if ((shootButtonJustPressed || !shootButtonUp) && !stateCanFire) {
		// Non-task state but button pressed
		sess->accumulatePlayerAction(PlayerActionType::Nontask);
	}
	else if (shootButtonJustPressed && !weapon->config()->autoFire && weapon->canFire(currentRealTime) && stateCanFire) {
		// Discrete weapon fires a single shot with normal damage at the current time
		numShots = 1;
		// These copy the above defaults, but are here for clarity
		damagePerShot = weapon->damagePerShot();
		newLastFireTime = currentRealTime;
	}
	else if (weapon->config()->autoFire && !weapon->config()->isContinuous() && !shootButtonUp && stateCanFire) {
		// Autofire weapon should create shots until currentRealTime with normal damage
		if (shootButtonJustPressed) {
			// If the button was just pressed, fire one bullet half way through
			weapon->setLastFireTime(m_lastOnSimulationRealTime + rdt * 0.5f);
			numShots = 1;
		}
		// Add on bullets until the frame time
		int newShots = weapon->numShotsUntil(currentRealTime);
		numShots += newShots;
		newLastFireTime = weapon->lastFireTime() + (float)(newShots) * weapon->config()->firePeriod;
		// This copies the above default, but are here for clarity
		damagePerShot = weapon->damagePerShot();
	}
	else if (weapon->config()->isContinuous() && (!shootButtonUp || shootButtonJustReleased) && stateCanFire) {
		// Continuous weapon should have been firing continuously, but since we do sampled simulation
		// this approximates continuous fire by releasing a single "megabullet"
		// with power that matches the elapsed time at the current
		numShots = 1;

		// If the button was just pressed, assume the duration should begin half way through
		if (shootButtonJustPressed) {
			weapon->setLastFireTime(m_lastOnSimulationRealTime + rdt * 0.5f);
		}
		// If the shoot button just released, assume the fire ended half way through
		newLastFireTime = shootButtonJustReleased ? m_lastOnSimulationRealTime + rdt * 0.5f : currentRealTime;
		RealTime fireDuration = weapon->fireDurationUntil(newLastFireTime);
		damagePerShot = (float)fireDuration * weapon->config()->damagePerSecond;
	}

	// Actually shoot here
	m_currentWeaponDamage = damagePerShot; // pass this to the callback where weapon damage is applied
	bool shotFired = false;
	for (int shotId = 0; shotId < numShots; shotId++) {
		Array<shared_ptr<Entity>> dontHit;
		dontHit.append(m_explosions);
		dontHit.append(sess->unhittableTargets());
		Model::HitInfo info;
		float hitDist = finf();
		int hitIdx = -1;

		shared_ptr<TargetEntity> target = weapon->fire(sess->hittableTargets(), hitIdx, hitDist, info, dontHit, false);			// Fire the weapon
		if (isNull(target)) // Miss case
		{
			// Play scene hit sound
			if (!weapon->config()->isContinuous() && notNull(m_sceneHitSound)) {
				m_sceneHitSound->play(sessConfig->audio.sceneHitSoundVol);
			}
		}
		shotFired = true;
	}
	if (shotFired) {
		weapon->setLastFireTime(newLastFireTime);
	}
	weapon->playSound(shotFired, shootButtonUp);

	// TODO (or NOTTODO): The following can be cleared at the cost of one more level of inheritance.
	sess->onSimulation(rdt, sdt, idt);

	// These are all we need from GApp::onSimulation() for walk mode
	m_widgetManager->onSimulation(rdt, sdt, idt);
	if (scene()) { scene()->onSimulation(sdt); }

	// make sure mouse sensitivity is set right
	if (m_userSettingsWindow->visible()) {
		updateMouseSensitivity();
	}

	// Simulate the projectiles
	weapon->simulateProjectiles(sdt, sess->hittableTargets());

	// explosion animation
	for (int i = 0; i < m_explosions.size(); i++) {
		shared_ptr<VisibleEntity> explosion = m_explosions[i];
		m_explosionRemainingTimes[i] -= sdt;
		if (m_explosionRemainingTimes[i] <= 0) {
			scene()->remove(explosion);
			m_explosions.fastRemove(i);
			m_explosionRemainingTimes.fastRemove(i);
			i--;
		}
		else {
			// could update animation here...
		}
	}

	// Move the player
	const shared_ptr<PlayerEntity>& p = scene()->typedEntity<PlayerEntity>("player");
	playerCamera->setFrame(p->getCameraFrame());
	
	// Handle developer mode features here
	if (startupConfig.developerMode) {
		// If the debug camera is selected, update it's position from the FPM
		if (activeCamera() == m_debugCamera) {
			m_debugCamera->setFrame(m_cameraManipulator->frame());
		}

		// Handle frame rate/delay updates here
		if (sessConfig->render.frameRate != lastSetFrameRate || displayLagFrames != sessConfig->render.frameDelay) {
			updateParameters(sessConfig->render.frameDelay, sessConfig->render.frameRate);
		}

		if (startupConfig.waypointEditorMode) {
			// Handle highlighting for selected target
			waypointManager->updateSelected();
			// Handle player motion recording here
			waypointManager->updatePlayerPosition(p->getCameraFrame().translation);
		}

		// Example GUI dynamic layout code.  Resize the debugWindow to fill
		// the screen horizontally.
		debugWindow->setRect(Rect2D::xywh(0.0f, 0.0f, (float)window()->width(), debugWindow->rect().height()));
	}
	   
	// Check for completed session
	if (sess->moveOn) {
		// Get the next session for the current user
		updateSession(userStatusTable.getNextSession());
	}

	// Update time at which this simulation finished
	m_lastOnSimulationRealTime = m_lastOnSimulationRealTime + rdt;
	m_lastOnSimulationSimTime = m_lastOnSimulationSimTime + sdt;
	m_lastOnSimulationIdealSimTime = m_lastOnSimulationIdealSimTime + idt;

	// Clear button press state
	shootButtonJustPressed = false;
	shootButtonJustReleased = false;
}

bool FPSciApp::onEvent(const GEvent& event) {
	GKey ksym = event.key.keysym.sym;
	bool foundKey = false;

	// Handle developer mode key-bound shortcuts here...
	if (startupConfig.developerMode) {
		if (event.type == GEventType::KEY_DOWN) {
			// Window display toggle
			if (keyMap.map["toggleRenderWindow"].contains(ksym)) {
				m_renderControls->setVisible(!m_renderControls->visible());
				foundKey = true;
			} else if (keyMap.map["togglePlayerWindow"].contains(ksym)) {
				m_playerControls->setVisible(!m_playerControls->visible());
				foundKey = true;
			} else if (keyMap.map["toggleWeaponWindow"].contains(ksym)) {
				m_weaponControls->setVisible(!m_weaponControls->visible());
				foundKey = true;
			}
			else if (keyMap.map["reloadConfigs"].contains(ksym)) {
				loadConfigs(startupConfig.experimentList[experimentIdx]);					// (Re)load the configs
				// Update session from the reloaded configs
				m_userSettingsWindow->updateSessionDropDown();
				updateSession(m_userSettingsWindow->selectedSession());
				// Do not set foundKey = true to allow shader reloading from GApp::onEvent()
			}
			// Waypoint editor only keys
			else if (startupConfig.waypointEditorMode) {
				if (keyMap.map["toggleWaypointWindow"].contains(ksym)) {
					waypointManager->toggleWaypointWindow();
					foundKey = true;
				} else if (keyMap.map["toggleRecording"].contains(ksym)) {
					waypointManager->recordMotion = !waypointManager->recordMotion;
					foundKey = true;
				}
				// Waypoint movement controls
				else if (keyMap.map["dropWaypoint"].contains(ksym)) {
					waypointManager->dropWaypoint();
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointUp"].contains(ksym)) {
					waypointManager->moveMask += Vector3(0.0f, 1.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointDown"].contains(ksym)) {
					waypointManager->moveMask += Vector3(0.0f, -1.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointIn"].contains(ksym)) {
					waypointManager->moveMask += Vector3(0.0f, 0.0f, 1.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointOut"].contains(ksym)) {
					waypointManager->moveMask += Vector3(0.0f, 0.0f, -1.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointRight"].contains(ksym)) {
					waypointManager->moveMask += Vector3(1.0f, 0.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointLeft"].contains(ksym)) {
					waypointManager->moveMask += Vector3(-1.0f, 0.0f, 0.0f);
					foundKey = true;
				}
			}
		}
		else if (event.type == GEventType::KEY_UP) {
			if (startupConfig.waypointEditorMode) {
				if (keyMap.map["moveWaypointUp"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(0.0f, 1.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointDown"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(0.0f, -1.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointIn"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(0.0f, 0.0f, 1.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointOut"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(0.0f, 0.0f, -1.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointRight"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(1.0f, 0.0f, 0.0f);
					foundKey = true;
				}
				else if (keyMap.map["moveWaypointLeft"].contains(ksym)) {
					waypointManager->moveMask -= Vector3(-1.0f, 0.0f, 0.0f);
					foundKey = true;
				}
			}
		}
	}
	// Handle key strokes explicitly for non-developer mode
	else {
		if (event.type == GEventType::KEY_DOWN) {
			// Remove F8 as it isn't masked by useDeveloperTools = False
			const Array<GKey> player_masked = { GKey::F8 };
			if (player_masked.contains(ksym)) {
				foundKey = true;
			}
		}
	}
	
	// Handle normal keypresses
	if (event.type == GEventType::KEY_DOWN) {
		if (keyMap.map["openMenu"].contains(ksym)) {
			toggleUserSettingsMenu();
			foundKey = true;
		}
		else if (keyMap.map["quit"].contains(ksym)) {
			quitRequest();
			return true;
		}
		else if (notNull(dialog) && !dialog->complete) {		// If we have an open, incomplete dialog, check for key bound question responses
			// Handle key presses to redirect towards dialog
			for (int i = 0; i < currentQuestion.optionKeys.length(); i++) {
				if (currentQuestion.optionKeys[i] == ksym) {
					dialog->result = currentQuestion.options[i];
					dialog->complete = true;
					dialog->setVisible(false);
					foundKey = true;
				}
			}
		}
		else if (activeCamera() == playerCamera && !foundKey) {
			// Override 'q', 'z', 'c', and 'e' keys (unused)
			// THIS IS A PROBLEM IF THESE ARE KEY MAPPED!!!
			const Array<GKey> unused = { (GKey)'e', (GKey)'z', (GKey)'c', (GKey)'q' };
			if (unused.contains(ksym)) {
				foundKey = true;
			}
			else if (keyMap.map["crouch"].contains(ksym)) {
				scene()->typedEntity<PlayerEntity>("player")->setCrouched(true);
				foundKey = true;
			}
			else if (keyMap.map["jump"].contains(ksym)) {
				scene()->typedEntity<PlayerEntity>("player")->setJumpPressed(true);
				foundKey = true;
			}
		}
	}
	else if ((event.type == GEventType::KEY_UP)) {
		if (activeCamera() == playerCamera) {
			if (keyMap.map["crouch"].contains(ksym)) {
				scene()->typedEntity<PlayerEntity>("player")->setCrouched(false);
				foundKey = true;
			}
		}
	}
	if (foundKey) {
		return true;
	}

	// Handle window resize here
	if (event.type == GEventType::VIDEO_RESIZE) {
		moveToCenter(m_userSettingsWindow);
	}

	// Handle window-based close ("X" button)
	if (event.type == GEventType::QUIT) {
		quitRequest();
		return true;
	}

	// Handle resize event here
	if (event.type == GEventType::VIDEO_RESIZE) {
		// Resize the shader buffers here
		updateShaderBuffers();
	}

	// Handle super-class events
	return GApp::onEvent(event);
}

void FPSciApp::onAfterEvents() {
	if (updateUserMenu) {
		// Remove the old settings window
		removeWidget(m_userSettingsWindow);

		// Re-create the settings window
		String selSess = m_userSettingsWindow->selectedSession();
		m_userSettingsWindow = UserMenu::create(this, userTable, userStatusTable, sessConfig->menu, theme, Rect2D::xywh(0.0f, 0.0f, 10.0f, 10.0f));
		m_userSettingsWindow->setSelectedSession(selSess);
		moveToCenter(m_userSettingsWindow);
		m_userSettingsWindow->setVisible(m_showUserMenu);
		setMouseInputMode(m_showUserMenu ? MouseInputMode::MOUSE_CURSOR : MouseInputMode::MOUSE_FPM);

		// Add the new settings window and clear the semaphore
		addWidget(m_userSettingsWindow);
		updateUserMenu = false;
	}

	if (reinitExperiment) {			// Check for experiment reinitialization (developer-mode only)
		m_widgetManager->clear();
		addWidget(debugWindow);
		addWidget(developerWindow);
		initExperiment();
		reinitExperiment = false;
	}

	GApp::onAfterEvents();
}

void FPSciApp::onPostProcessHDR3DEffects(RenderDevice* rd) {
	// Put elements that should be delayed along w/ 3D here
	if (sessConfig->render.split2DBuffer) {
		// If rendering into a split (offscreen) 2D buffer, then clear it/render to it here
		m_buffer2D->texture(0)->clear();
		rd->push2D(m_buffer2D);
	}
	else {
		// Otherwise render to the framebuffer
		rd->push2D();
	}
	{
		rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
		const float scale = rd->viewport().width() / 1920.0f;

		// Draw target health bars
		if (sessConfig->targetView.showHealthBars) {
			for (auto const& target : sess->targetArray()) {
				target->drawHealthBar(rd, *activeCamera(), *m_framebuffer,
					sessConfig->targetView.healthBarSize,
					sessConfig->targetView.healthBarOffset,
					sessConfig->targetView.healthBarBorderSize,
					sessConfig->targetView.healthBarColors,
					sessConfig->targetView.healthBarBorderColor);
			}
		}

		// Draw the combat text
		if (sessConfig->targetView.showCombatText) {
			Array<int> toRemove;
			for (int i = 0; i < m_combatTextList.size(); i++) {
				bool remove = !m_combatTextList[i]->draw(rd, *playerCamera, *m_framebuffer);
				if (remove) m_combatTextList[i] = nullptr;		// Null pointers to remove
			}
			// Remove the expired elements here
			m_combatTextList.removeNulls();
		}

		if (sessConfig->clickToPhoton.enabled && sessConfig->clickToPhoton.mode == "total") {
			drawClickIndicator(rd, "total");
		}

		// Draw the HUD here
		if (sessConfig->hud.enable) {
			drawHUD(rd);
		}
	}rd->pop2D();

	if (!sessConfig->render.shader3D.empty() && m_shaderTable.containsKey(sessConfig->render.shader3D)) {
		BEGIN_PROFILER_EVENT_WITH_HINT("3D Shader Pass", "Time to run the post-3D shader pass");

		rd->push2D(m_shader3DOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_framebuffer->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_lastTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shader3D], args);
			m_lastTime = iTime;
		} rd->pop2D();

		// Copy the shader output buffer into the framebuffer
		sessConfig->render.shaderComposite.empty() ? rd->push2D() : rd->push2D(m_bufferComposite); {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shader3DOutput->texture(0), Sampler::buffer());
		} rd->pop2D();
		END_PROFILER_EVENT();
	}

	GApp::onPostProcessHDR3DEffects(rd);
}

void FPSciApp::drawClickIndicator(RenderDevice *rd, String mode) {
	// Click to photon latency measuring corner box
	if (sessConfig->clickToPhoton.enabled) {
		float boxLeft = 0.0f;
		// Paint both sides by the width of latency measuring box.
		Point2 latencyRect = sessConfig->clickToPhoton.size * Point2((float)rd->viewport().width(), (float)rd->viewport().height());
		float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		if (guardband) {
			boxLeft += guardband;
		}
		float boxTop = rd->viewport().height()*sessConfig->clickToPhoton.vertPos - latencyRect.y / 2;
		if (sessConfig->clickToPhoton.mode == "both") {
			boxTop = (mode == "minimum") ? boxTop - latencyRect.y : boxTop + latencyRect.y;
		}
		if (sessConfig->clickToPhoton.side == "right") {
			boxLeft = (float)rd->viewport().width() - (guardband + latencyRect.x);
		}
		// Draw the "active" box
		Color3 boxColor;
		if (sessConfig->clickToPhoton.mode == "frameRate") {
			boxColor = (frameToggle) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
			frameToggle = !frameToggle;
		}
		else boxColor = (shootButtonUp) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
		Draw::rect2D(Rect2D::xywh(boxLeft, boxTop, latencyRect.x, latencyRect.y), rd, boxColor);
	}
}

void FPSciApp::drawHUD(RenderDevice *rd) {
	// Scale is used to position/resize the "score banner" when the window changes size in "windowed" mode (always 1 in fullscreen mode).
	const Vector2 scale = rd->viewport().wh() / displayRes;

	RealTime now = m_lastOnSimulationRealTime;

	// Weapon ready status (cooldown indicator)
	if (sessConfig->hud.renderWeaponStatus) {
		// Draw the "active" cooldown box
		if (sessConfig->hud.cooldownMode == "box") {
			float boxLeft = (float)rd->viewport().width() * 0.0f;
			if (sessConfig->hud.weaponStatusSide == "right") {
				// swap side
				boxLeft = (float)rd->viewport().width() * (1.0f - sessConfig->clickToPhoton.size.x);
			}
			Draw::rect2D(
				Rect2D::xywh(
					boxLeft,
					(float)rd->viewport().height() * (float)(weapon->cooldownRatio(now)),
					(float)rd->viewport().width() * sessConfig->clickToPhoton.size.x,
					(float)rd->viewport().height() * (float)(1.0 - weapon->cooldownRatio(now))
				), rd, Color3::white() * 0.8f
			);
		}
		else if (sessConfig->hud.cooldownMode == "ring") {
			// Draw cooldown "ring" instead of box
			const float iRad = sessConfig->hud.cooldownInnerRadius;
			const float oRad = iRad + sessConfig->hud.cooldownThickness;
			const int segments = sessConfig->hud.cooldownSubdivisions;
			int segsToLight = static_cast<int>(ceilf((1 - weapon->cooldownRatio(now))*segments));
			// Create the segments
			for (int i = 0; i < segsToLight; i++) {
				const float inc = static_cast<float>(2 * pi() / segments);
				const float theta = -i * inc;
				Vector2 center = Vector2(rd->viewport().width() / 2.0f, rd->viewport().height() / 2.0f);
				Array<Vector2> verts = {
					center + Vector2(oRad*sin(theta), -oRad * cos(theta)),
					center + Vector2(oRad*sin(theta + inc), -oRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta + inc), -iRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta), -iRad * cos(theta))
				};
				Draw::poly2D(verts, rd, sessConfig->hud.cooldownColor);
			}
		}
	}

	// Draw the player health bar
	if (sessConfig->hud.showPlayerHealthBar) {
		const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		const float health = scene()->typedEntity<PlayerEntity>("player")->health();
		const Point2 location = Point2(sessConfig->hud.playerHealthBarPos.x, sessConfig->hud.playerHealthBarPos.y + m_debugMenuHeight) + Point2(guardband, guardband);
		const Point2 size = sessConfig->hud.playerHealthBarSize;
		const Point2 border = sessConfig->hud.playerHealthBarBorderSize;
		const Color4 borderColor = sessConfig->hud.playerHealthBarBorderColor;
		const Color4 color = sessConfig->hud.playerHealthBarColors[1] * (1.0f - health) + sessConfig->hud.playerHealthBarColors[0] * health;

		Draw::rect2D(Rect2D::xywh(location - border, size + border + border), rd, borderColor);
		Draw::rect2D(Rect2D::xywh(location, size*Point2(health, 1.0f)), rd, color);
	}
	// Draw the ammo indicator
	if (sessConfig->hud.showAmmo) {
		const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		Point2 lowerRight = Point2(static_cast<float>(rd->viewport().width()), static_cast<float>(rd->viewport().height())) - Point2(guardband, guardband);
		hudFont->draw2D(rd,
			format("%d/%d", weapon->remainingAmmo(), sessConfig->weapon.maxAmmo),
			lowerRight - sessConfig->hud.ammoPosition,
			sessConfig->hud.ammoSize,
			sessConfig->hud.ammoColor,
			sessConfig->hud.ammoOutlineColor,
			GFont::XALIGN_RIGHT,
			GFont::YALIGN_BOTTOM
		);
	}

	if (sessConfig->hud.showBanner && !emergencyTurbo) {
		const shared_ptr<Texture> scoreBannerTexture = hudTextures["scoreBannerBackdrop"];
		const Point2 hudCenter(rd->viewport().width() / 2.0f, sessConfig->hud.bannerVertVisible*scoreBannerTexture->height() * scale.y + debugMenuHeight());
		Draw::rect2D((scoreBannerTexture->rect2DBounds() * scale - scoreBannerTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), scoreBannerTexture);

		// Create strings for time remaining, progress in sessions, and score
		float remainingTime = sess->getRemainingTrialTime();
		float printTime = remainingTime > 0 ? remainingTime : 0.0f;
		String time_string = format("%0.2f", printTime);
		float prog = sess->getProgress();
		String prog_string = "";
		if (!isnan(prog)) {
			prog_string = format("%d", (int)(100.0f*prog)) + "%";
		}
		String score_string = format("%d", (int)(10 * sess->getScore()));

		hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale.x, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale.x * sessConfig->hud.bannerLargeFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}

	// Draw any static HUD elements
	for (StaticHudElement element : sessConfig->hud.staticElements) {
		if (!hudTextures.containsKey(element.filename)) continue;						// Skip any items we haven't loaded
		const shared_ptr<Texture> texture = hudTextures[element.filename];				// Get the loaded texture for this element
		const Vector2 size = element.scale * scale * texture->vector2Bounds();			// Get the final size of the image
		const Vector2 pos = (element.position * rd->viewport().wh()) - size/2.0;		// Compute position (center image on provided position)
		Draw::rect2D(Rect2D::xywh(pos, size), rd, Color3::white(), texture);			// Draw the rect
	}
}

Vector2 FPSciApp::currentTurnScale() {
	const shared_ptr<UserConfig> user = currentUser();
	Vector2 baseTurnScale = sessConfig->player.turnScale * user->turnScale;
	// Apply y-invert here
	if (user->invertY) baseTurnScale.y = -baseTurnScale.y;
	// If we're not scoped just return the normal user turn scale
	if (!weapon || !weapon->scoped()) return baseTurnScale;
	// Otherwise create scaled turn scale for the scoped state
	if (user->scopeTurnScale.length() > 0) {
		// User scoped turn scale specified, don't perform default scaling
		return baseTurnScale * user->scopeTurnScale;
	}
	else {
		// Otherwise scale the scope turn scalue using the ratio of FoV
		return playerCamera->fieldOfViewAngleDegrees() / sessConfig->render.hFoV * baseTurnScale;
	}
}

void FPSciApp::setScopeView(bool scoped) {
	// Get player entity and calculate scope FoV
	const shared_ptr<PlayerEntity>& player = scene()->typedEntity<PlayerEntity>("player");
	const float scopeFoV = sessConfig->weapon.scopeFoV > 0 ? sessConfig->weapon.scopeFoV : sessConfig->render.hFoV;
	weapon->setScoped(scoped);														// Update the weapon state		
	const float FoV = (scoped ? scopeFoV : sessConfig->render.hFoV);					// Get new FoV in degrees (depending on scope state)
	playerCamera->setFieldOfView(FoV * pif() / 180.f, FOVDirection::HORIZONTAL);		// Set the camera FoV
	player->turnScale = currentTurnScale();												// Scale sensitivity based on the field of view change here
}

void FPSciApp::hitTarget(shared_ptr<TargetEntity> target) {
	// Damage the target
	float damage = m_currentWeaponDamage;
	target->doDamage(damage);
	target->playHitSound();

	// Check if we need to add combat text for this damage
	if (sessConfig->targetView.showCombatText) {
		m_combatTextList.append(FloatingCombatText::create(
			format("%2.0f", 100.f * damage),
			m_combatFont,
			sessConfig->targetView.combatTextSize,
			sessConfig->targetView.combatTextColor,
			sessConfig->targetView.combatTextOutline,
			sessConfig->targetView.combatTextOffset,
			sessConfig->targetView.combatTextVelocity,
			sessConfig->targetView.combatTextFade,
			sessConfig->targetView.combatTextTimeout));
		m_combatTextList.last()->setFrame(target->frame());
	}

	// Check for "kill" condition
	bool respawned = false;
	bool destroyedTarget = false;
	if (target->name() == "reference") {
		// Handle reference target here
		sess->destroyTarget(target);
		destroyedTarget = true;
		sess->accumulatePlayerAction(PlayerActionType::Nontask, target->name());

	}
	else if (target->health() <= 0) {
		// Position explosion
		CFrame explosionFrame = target->frame();
		explosionFrame.rotation = playerCamera->frame().rotation;
		// Create the explosion
		const shared_ptr<VisibleEntity> newExplosion = VisibleEntity::create(
			format("explosion%d", m_explosionIdx), 
			scene().get(), 
			m_explosionModels.get(target->id())[target->scaleIndex()], 
			explosionFrame
		);
		m_explosionIdx++;
		m_explosionIdx %= m_maxExplosions;
		scene()->insert(newExplosion);
		m_explosions.push(newExplosion);
		m_explosionRemainingTimes.push(experimentConfig.getTargetConfigById(target->id())->destroyDecalDuration); // Schedule end of explosion
		target->playDestroySound();

		sess->countDestroy();
		respawned = target->tryRespawn();
		// check for respawn
		if (!respawned) {
			// This is the final respawn
			//destroyTarget(hitIdx);
			sess->destroyTarget(target);
			destroyedTarget = true;
		}
		// Target eliminated, must be 'destroy'.
		sess->accumulatePlayerAction(PlayerActionType::Destroy, target->name());
	}
	else {
		// Target 'hit', but still alive.
		sess->accumulatePlayerAction(PlayerActionType::Hit, target->name());
	}
	if (!destroyedTarget || respawned) {
		if (respawned) {
			sess->randomizePosition(target);
		}
		// Update the target color based on it's health
		updateTargetColor(target);
	}
}

void FPSciApp::updateTargetColor(const shared_ptr<TargetEntity>& target) {
	BEGIN_PROFILER_EVENT("updateTargetColor/changeColor");
	BEGIN_PROFILER_EVENT("updateTargetColor/clone");
	shared_ptr<ArticulatedModel::Pose> pose = dynamic_pointer_cast<ArticulatedModel::Pose>(target->pose()->clone());
	END_PROFILER_EVENT();
	BEGIN_PROFILER_EVENT("updateTargetColor/materialSet");
	shared_ptr<UniversalMaterial> mat = m_materials[min((int)(target->health() * m_MatTableSize), m_MatTableSize - 1)];
	pose->materialTable.set("core/icosahedron_default", mat);
	END_PROFILER_EVENT();
	BEGIN_PROFILER_EVENT("updateTargetColor/setPose");
	target->setPose(pose);
	END_PROFILER_EVENT();
	END_PROFILER_EVENT();
}

void FPSciApp::missEvent() {
	if (sess) {
		sess->accumulatePlayerAction(PlayerActionType::Miss);		// Declare this shot a miss here
	}
}

/** Handle user input here */
void FPSciApp::onUserInput(UserInput* ui) {
	BEGIN_PROFILER_EVENT("onUserInput");

	GApp::onUserInput(ui);

	const shared_ptr<PlayerEntity>& player = scene()->typedEntity<PlayerEntity>("player");
	if (m_mouseInputMode == MouseInputMode::MOUSE_FPM && activeCamera() == playerCamera && notNull(player)) {
		player->updateFromInput(ui);		// Only update the player if the mouse input mode is FPM and the active camera is the player view camera
	}
	else if (notNull(player)) {	// Zero the player velocity and rotation when in the setting menu
		player->setDesiredOSVelocity(Vector3::zero());
		player->setDesiredAngularVelocity(0.0, 0.0);
	}

	// Handle scope behavior
	for (GKey scopeButton : keyMap.map["scope"]) {
		if (ui->keyPressed(scopeButton)) {
			// Are we using scope toggling?
			if (sessConfig->weapon.scopeToggle) {
				setScopeView(!weapon->scoped());
			}
			// Otherwise just set scope based on the state of the scope button
			else {
				setScopeView(true);
			}
		}
		if (ui->keyReleased(scopeButton) && !sessConfig->weapon.scopeToggle) {
			setScopeView(false);
		}
	}

	// Record button state changes
	// These will be evaluated and reset on the next onSimulation()
	for (GKey shootButton : keyMap.map["shoot"]) {
		if (ui->keyReleased(shootButton)) {
			shootButtonJustReleased = true;
			shootButtonUp = true;
		}
		if (ui->keyPressed(shootButton)) {
			shootButtonJustPressed = true;
		}
		if (ui->keyDown(shootButton)) {
			shootButtonUp = false;
		}
	}

	for (GKey selectButton : keyMap.map["selectWaypoint"]) {
		// Check for developer mode editing here, if so set selected waypoint using the camera
		if (ui->keyDown(selectButton) && startupConfig.developerMode && startupConfig.waypointEditorMode) {
			waypointManager->aimSelectWaypoint(activeCamera());
		}
	}
	
	for (GKey dummyShoot : keyMap.map["dummyShoot"]) {
		if (ui->keyPressed(dummyShoot) && (sess->currentState == PresentationState::trialFeedback) && !m_userSettingsWindow->visible()) {
			Array<shared_ptr<Entity>> dontHit;
			dontHit.append(m_explosions);
			dontHit.append(sess->unhittableTargets());
			Model::HitInfo info;
			float hitDist = finf();
			int hitIdx = -1;
			shared_ptr<TargetEntity> target = weapon->fire(sess->hittableTargets(), hitIdx, hitDist, info, dontHit, true);			// Fire the weapon
		}
	}

	if (m_lastReticleLoaded != currentUser()->reticleIndex || m_userSettingsWindow->visible()) {
		// Slider was used to change the reticle
		setReticle(currentUser()->reticleIndex);
		m_userSettingsWindow->updateReticlePreview();
	}

	playerCamera->filmSettings().setSensitivity(sceneBrightness);
    END_PROFILER_EVENT();
}

void FPSciApp::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
	GApp::onPose(surface, surface2D);

	typedScene<PhysicsScene>()->poseExceptExcluded(surface, "player");

	if (weapon) { weapon->onPose(surface); }
}

void FPSciApp::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
	// Track the instantaneous frame duration (no smoothing) in a circular queue
	if (m_frameDurationQueue.length() > MAX_HISTORY_TIMING_FRAMES) {
		m_frameDurationQueue.dequeue();
	}
	{
		const float f = rd->stats().frameRate;
		const float t = 1.0f / f;
		m_frameDurationQueue.enqueue(t);
	}

	float recentMin = finf();
	float recentMax = -finf();
	for (int i = 0; i < m_frameDurationQueue.length(); ++i) {
		const float t = m_frameDurationQueue[i];
		recentMin = min(recentMin, t);
		recentMax = max(recentMax, t);
	}

	// Set render buffer depending on whether we are rendering to a sperate 2D buffer
	sessConfig->render.split2DBuffer ? rd->push2D(m_buffer2D) : rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
		const float scale = rd->viewport().width() / 1920.0f;

		// FPS display (faster than the full stats widget)
		if (renderFPS) {
			String msg;

			if (window()->settings().refreshRate > 0) {
				msg = format("%d measured / %d requested fps",
					iRound(renderDevice->stats().smoothFrameRate),
					window()->settings().refreshRate);
			}
			else {
				msg = format("%d fps", iRound(renderDevice->stats().smoothFrameRate));
			}

			msg += format(" | %.1f min/%.1f avg/%.1f max ms", recentMin * 1000.0f, 1000.0f / renderDevice->stats().smoothFrameRate, 1000.0f * recentMax);
			outputFont->draw2D(rd, msg, Point2(rd->viewport().width()*0.75f, rd->viewport().height()*0.05f).floor(), floor(20.0f*scale), Color3::yellow());
		}

		// Handle recording indicator
		if (startupConfig.waypointEditorMode && waypointManager->recordMotion) {
			Draw::point(Point2(rd->viewport().width()*0.9f - 15.0f, 20.0f+m_debugMenuHeight*scale), rd, Color3::red(), 10.0f);
			outputFont->draw2D(rd, "Recording Position", Point2(rd->viewport().width() - 200.0f , m_debugMenuHeight*scale), 20.0f, Color3::red());
		}

		// Click-to-photon mouse event indicator
		if (sessConfig->clickToPhoton.enabled && sessConfig->clickToPhoton.mode != "total") {
			drawClickIndicator(rd, sessConfig->clickToPhoton.mode);
		}

		// Player camera only indicators
		if (activeCamera() == playerCamera) {
			// Reticle
			const shared_ptr<UserConfig> user = currentUser();
			float tscale = weapon->cooldownRatio(m_lastOnSimulationRealTime, user->reticleChangeTimeS);
			float rScale = tscale * user->reticleScale[0] + (1.0f - tscale)*user->reticleScale[1];
			Color4 rColor = user->reticleColor[1] * (1.0f - tscale) + user->reticleColor[0] * tscale;
			Draw::rect2D(((reticleTexture->rect2DBounds() - reticleTexture->vector2Bounds() / 2.0f))*rScale / 2.0f + rd->viewport().wh() / 2.0f, rd, rColor, reticleTexture);

			// Handle the feedback message
			String message = sess->getFeedbackMessage();
			const float centerHeight = rd->viewport().height() * 0.4f;
			const float scaledFontSize = floor(sessConfig->feedback.fontSize * scale);
			if (!message.empty()) {
				String currLine;
				Array<String> lines = stringSplit(message, '\n');
				float vertPos = centerHeight - (scaledFontSize * 1.5f * lines.length()/ 2.0f);
				// Draw a "back plate"
				Draw::rect2D(Rect2D::xywh(0.0f, 
					vertPos - 1.5f * scaledFontSize,
					rd->viewport().width(), 
					scaledFontSize * (lines.length()+1) * 1.5f),
					rd, sessConfig->feedback.backgroundColor);
				for (String line : lines) {
					outputFont->draw2D(rd, line.c_str(),
						(Point2(rd->viewport().width() * 0.5f, vertPos)).floor(),
						scaledFontSize,
						sessConfig->feedback.color,
						sessConfig->feedback.outlineColor,
						GFont::XALIGN_CENTER, GFont::YALIGN_CENTER
					);
					vertPos += scaledFontSize * 1.5f;
				}
			}
		}

	} rd->pop2D();

	// Handle 2D-only shader here (requires split 2D framebuffer)
	if (!sessConfig->render.shader2D.empty() && m_shaderTable.containsKey(sessConfig->render.shader2D)) {
		if (!sessConfig->render.split2DBuffer) {
			throw "Cannot warp non-split 2D buffer, use \"shaderComposite\" to warp combined buffer instead!";
		}
		BEGIN_PROFILER_EVENT_WITH_HINT("2D Shader Pass", "Time to run the post-2D shader pass");

		rd->push2D(m_shader2DOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_buffer2D->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_last2DTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shader2D], args);
			m_last2DTime = iTime;
		} rd->pop2D();

		// Direct shader output to the display or composite shader input (if specified)
		sessConfig->render.shaderComposite.empty() ? rd->push2D() : rd->push2D(m_bufferComposite); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shader2DOutput->texture(0), Sampler::buffer());
		} rd->pop2D();

		END_PROFILER_EVENT();
	}

	// Composite the 2D buffer here (if not given a 2D shader)
	else if (sessConfig->render.split2DBuffer) {
		// Copy the 2D (split) output buffer into the framebuffer or composite shader input (if specified)
		sessConfig->render.shaderComposite.empty() ? rd->push2D() : rd->push2D(m_bufferComposite); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_buffer2D->texture(0), Sampler::buffer());
		} rd->pop2D();
	}

	//  Handle post-2D composite shader here
	if (!sessConfig->render.shaderComposite.empty() && m_shaderTable.containsKey(sessConfig->render.shaderComposite)) {
		BEGIN_PROFILER_EVENT_WITH_HINT("Composite Shader Pass", "Time to run the composite shader pass");

		// If we haven't run a shader into the composite buffer then blit the framebuffer here now
		if (sessConfig->render.shader2D.empty() && sessConfig->render.shader3D.empty()) {
			rd->readFramebuffer()->blitTo(rd, m_bufferComposite, true, false, false, false);
		}

		rd->push2D(m_shaderCompositeOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_bufferComposite->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_lastCompositeTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shaderComposite], args);
			m_lastCompositeTime = iTime;
		} rd->pop2D();

		// Copy the shader output buffer into the framebuffer
		rd->push2D(); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shaderCompositeOutput->texture(0), Sampler::buffer());
		} rd->pop2D();

		END_PROFILER_EVENT();
	}

	// Might not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}

/** Set the currently reticle by index */
void FPSciApp::setReticle(const int r) {
	int idx = clamp(0, r, numReticles);
	if(idx == m_lastReticleLoaded) return;	// Nothing to do here, setting current reticle
	if (r < numReticles) {
		reticleTexture = Texture::fromFile(System::findDataFile(format("gui/reticle/reticle-%03d.png", idx)));
	}
	else {
		// This special case is added to allow a custom reticle not in the gui/reticle/reticle-[x].png format
		reticleTexture = Texture::fromFile(System::findDataFile("gui/reticle.png"));
	}
	m_lastReticleLoaded = idx;
}

void FPSciApp::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
}

/** Overridden (optimized) oneFrame() function to improve latency */
void FPSciApp::oneFrame() {
	// Count this frame (for shaders)
	m_frameNumber++;

    // Wait
    // Note: we might end up spending all of our time inside of
    // RenderDevice::beginFrame.  Waiting here isn't double waiting,
    // though, because while we're sleeping the CPU the GPU is working
    // to catch up.    
    if ((submitToDisplayMode() == SubmitToDisplayMode::MINIMIZE_LATENCY)) {
        BEGIN_PROFILER_EVENT("Wait");
        m_waitWatch.tick(); {
            RealTime nowAfterLoop = System::time();

            // Compute accumulated time
            RealTime cumulativeTime = nowAfterLoop - m_lastWaitTime;

            debugAssert(m_wallClockTargetDuration < finf());
            // Perform wait for actual time needed
            RealTime duration = m_wallClockTargetDuration;
            if (!window()->hasFocus() && m_lowerFrameRateInBackground) {
                // Lower frame rate to 4fps
                duration = 1.0 / 4.0;
            }
            RealTime desiredWaitTime = max(0.0, duration - cumulativeTime);
            onWait(max(0.0, desiredWaitTime - m_lastFrameOverWait) * 0.97);

            // Update wait timers
            m_lastWaitTime = System::time();
            RealTime actualWaitTime = m_lastWaitTime - nowAfterLoop;

            // Learn how much onWait appears to overshoot by and compensate
            double thisOverWait = actualWaitTime - desiredWaitTime;
            if (G3D::abs(thisOverWait - m_lastFrameOverWait) / max(G3D::abs(m_lastFrameOverWait), G3D::abs(thisOverWait)) > 0.4) {
                // Abruptly change our estimate
                m_lastFrameOverWait = thisOverWait;
            }
            else {
                // Smoothly change our estimate
                m_lastFrameOverWait = lerp(m_lastFrameOverWait, thisOverWait, 0.1);
            }
        }  m_waitWatch.tock();
        END_PROFILER_EVENT();
    }

    for (int repeat = 0; repeat < max(1, m_renderPeriod); ++repeat) {
        Profiler::nextFrame();
        m_lastTime = m_now;
        m_now = System::time();
        RealTime timeStep = m_now - m_lastTime;

        // User input
        m_userInputWatch.tick();
        if (manageUserInput) {
            processGEventQueue();
        }
        onAfterEvents();
        onUserInput(userInput);
        m_userInputWatch.tock();

        // Network
        BEGIN_PROFILER_EVENT("GApp::onNetwork");
        m_networkWatch.tick();
        onNetwork();
        m_networkWatch.tock();
        END_PROFILER_EVENT();

        // Logic
        m_logicWatch.tick();
        {
            onAI();
        }
        m_logicWatch.tock();

        // Simulation
        m_simulationWatch.tick();
        BEGIN_PROFILER_EVENT("Simulation");
        {
            RealTime rdt = timeStep;

            SimTime sdt = m_simTimeStep;
            if (sdt == MATCH_REAL_TIME_TARGET) {
                sdt = m_wallClockTargetDuration;
            }
            else if (sdt == REAL_TIME) {
                sdt = float(timeStep);
            }
            sdt *= m_simTimeScale;

            SimTime idt = m_wallClockTargetDuration;

            onBeforeSimulation(rdt, sdt, idt);
            onSimulation(rdt, sdt, idt);
            onAfterSimulation(rdt, sdt, idt);

            m_previousSimTimeStep = float(sdt);
            m_previousRealTimeStep = float(rdt);
            setRealTime(realTime() + rdt);
            setSimTime(simTime() + sdt);
        }
        m_simulationWatch.tock();
        END_PROFILER_EVENT();
    }


    // Pose
    BEGIN_PROFILER_EVENT("Pose");
    m_poseWatch.tick(); {
        m_posed3D.fastClear();
        m_posed2D.fastClear();
        onPose(m_posed3D, m_posed2D);

        // The debug camera is not in the scene, so we have
        // to explicitly pose it. This actually does nothing, but
        // it allows us to trigger the TAA code.
		playerCamera->onPose(m_posed3D);
    } m_poseWatch.tock();
    END_PROFILER_EVENT();

    // Wait
    // Note: we might end up spending all of our time inside of
    // RenderDevice::beginFrame.  Waiting here isn't double waiting,
    // though, because while we're sleeping the CPU the GPU is working
    // to catch up.    
    if ((submitToDisplayMode() != SubmitToDisplayMode::MINIMIZE_LATENCY)) {
        BEGIN_PROFILER_EVENT("Wait");
        m_waitWatch.tick(); {
            RealTime nowAfterLoop = System::time();

            // Compute accumulated time
            RealTime cumulativeTime = nowAfterLoop - m_lastWaitTime;

            debugAssert(m_wallClockTargetDuration < finf());
            // Perform wait for actual time needed
            RealTime duration = m_wallClockTargetDuration;
            if (!window()->hasFocus() && m_lowerFrameRateInBackground) {
                // Lower frame rate to 4fps
                duration = 1.0 / 4.0;
            }
            RealTime desiredWaitTime = max(0.0, duration - cumulativeTime);
            onWait(max(0.0, desiredWaitTime - m_lastFrameOverWait) * 0.97);

            // Update wait timers
            m_lastWaitTime = System::time();
            RealTime actualWaitTime = m_lastWaitTime - nowAfterLoop;

            // Learn how much onWait appears to overshoot by and compensate
            double thisOverWait = actualWaitTime - desiredWaitTime;
            if (G3D::abs(thisOverWait - m_lastFrameOverWait) / max(G3D::abs(m_lastFrameOverWait), G3D::abs(thisOverWait)) > 0.4) {
                // Abruptly change our estimate
                m_lastFrameOverWait = thisOverWait;
            }
            else {
                // Smoothly change our estimate
                m_lastFrameOverWait = lerp(m_lastFrameOverWait, thisOverWait, 0.1);
            }
        }  m_waitWatch.tock();
        END_PROFILER_EVENT();
    }

    // Graphics
    debugAssertGLOk();
    if ((submitToDisplayMode() == SubmitToDisplayMode::BALANCE) && (!renderDevice->swapBuffersAutomatically())) {
        swapBuffers();
    }

    if (notNull(m_gazeTracker)) {
        BEGIN_PROFILER_EVENT("Gaze Tracker");
        sampleGazeTrackerData();
        END_PROFILER_EVENT();
    }

    BEGIN_PROFILER_EVENT("Graphics");
    renderDevice->beginFrame();
    m_widgetManager->onBeforeGraphics();
    m_graphicsWatch.tick(); {
        debugAssertGLOk();
        renderDevice->pushState(); {
            debugAssertGLOk();
            onGraphics(renderDevice, m_posed3D, m_posed2D);
        } renderDevice->popState();
    }  m_graphicsWatch.tock();
    renderDevice->endFrame();
    if ((submitToDisplayMode() == SubmitToDisplayMode::MINIMIZE_LATENCY) && (!renderDevice->swapBuffersAutomatically())) {
        swapBuffers();
    }
    END_PROFILER_EVENT();

    // Remove all expired debug shapes
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        if (debugShapeArray[i].endTime <= m_now) {
            debugShapeArray.fastRemove(i);
            --i;
        }
    }

    for (int i = 0; i < debugLabelArray.size(); ++i) {
        if (debugLabelArray[i].endTime <= m_now) {
            debugLabelArray.fastRemove(i);
            --i;
        }
    }

    debugText.fastClear();

    m_posed3D.fastClear();
    m_posed2D.fastClear();

    if (m_endProgram && window()->requiresMainLoop()) {
        window()->popLoopBody();
    }
}

FPSciApp::Settings::Settings(const StartupConfig& startupConfig, int argc, const char* argv[])
{
	if (startupConfig.fullscreen) {
		// Use the primary 
		window.width = (int)OSWindow::primaryDisplaySize().x;
		window.height = (int)OSWindow::primaryDisplaySize().y;
	}
	else {
		window.width = (int)startupConfig.windowSize.x;
		window.height = (int)startupConfig.windowSize.y;
	}
	window.fullScreen = startupConfig.fullscreen;
	window.resizable = !window.fullScreen;

	// V-sync off always
	window.asynchronous = true;
	window.caption = "First Person Science";
	window.refreshRate = -1;
	window.defaultIconFilename = "icon.png";

	useDeveloperTools = startupConfig.developerMode;

	hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
	hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
	dataDir = FileSystem::currentDirectory();
	screenCapture.includeAppRevision = false;
	screenCapture.includeG3DRevision = false;
	screenCapture.outputDirectory = ""; // "../journal/"
	screenCapture.filenamePrefix = "_";

	renderer.deferredShading = true;
	renderer.orderIndependentTransparency = false;
}
