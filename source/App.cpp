/** \file App.cpp */
#include "App.h"
#include "Dialogs.h"
#include "PlayerEntity.h"
#include "Logger.h"
#include "Session.h"
#include "PhysicsScene.h"
#include "WaypointManager.h"
#include <chrono>
#include <io.h>
#include <typeinfo>
#include <exception>

// Scale and offset for target
const float App::TARGET_MODEL_ARRAY_SCALING = 0.2f;
const float App::TARGET_MODEL_ARRAY_OFFSET = 20;

// Storage for configuration static vars
FpsConfig SessionConfig::defaultConfig;
int TrialCount::defaultCount;
Array<String> UserSessionStatus::defaultSessionOrder;

bool UserSessionStatus::randomizeDefaults;

/** global startup config - sets developer flags and experiment/user paths */
StartupConfig App::startupConfig;

App::App(const GApp::Settings& settings) : GApp(settings)
{
	for (auto& arg : m_settings.argArray) {
		if (arg == "--test") {
			printf("Starting in automated testing mode\n");
			m_testMode = true;
		}
	}
}

bool App::testModeRequested()
{
	return m_testMode;
}

void App::dumpNextFrame(String filename)
{
	m_dumpNextFrame = true;
	m_frameDumpFilename = filename;
}

/** Initialize the app */
void App::onInit() {
	// Seed random based on the time
	Random::common().reset(uint32(time(0)));

	// Initialize the app
	GApp::onInit();

	// Load experiment setting from file
	experimentConfig = ExperimentConfig::load(startupConfig.experimentConfig());
	experimentConfig.printToLog();

	Array<String> sessionIds;
	experimentConfig.getSessionIds(sessionIds);

	// Load per user settings from file
	userTable = UserTable::load(startupConfig.userConfig());
	userTable.printToLog();

	// Load per experiment user settings from file and make sure they are valid
	userStatusTable = UserStatusTable::load(startupConfig.userStatusConfig());
	userStatusTable.printToLog();
	userStatusTable.validate(sessionIds);
	
	// Get and save system configuration
	SystemConfig sysConfig = SystemConfig::load();
	sysConfig.printToLog();											// Print system info to log.txt
	sysConfig.toAny().save("systemconfig.Any");						// Update the any file here (new system info to write)

	// Load the key binds
	keyMap = KeyMapping::load();
	userInput->setKeyMapping(&keyMap.uiMap);

	// Setup/update waypoint manager
	waypointManager = WaypointManager::create(this);

	// Setup the display mode
	setSubmitToDisplayMode(
		//SubmitToDisplayMode::EXPLICIT);
		SubmitToDisplayMode::MINIMIZE_LATENCY);
		//SubmitToDisplayMode::BALANCE);
	    //SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);

	// Setup the scene
	setScene(PhysicsScene::create(m_ambientOcclusion));
	scene()->registerEntitySubclass("PlayerEntity", &PlayerEntity::create);			// Register the player entity for creation
	scene()->registerEntitySubclass("FlyingEntity", &FlyingEntity::create);			// Create a target

	// Setup the GUI
	showRenderingStats = false;
	makeGUI();
	   
	// Load fonts and images
	outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));

	// Load models and set the reticle
	loadModels();
	setReticle(userTable.getCurrentUser()->reticleIndex);

	m_noLatencyCamera = Camera::create();
	m_noLatencyCamera->copyParametersFrom(activeCamera());
    m_noLatencyCamera->filmSettings().setAntialiasingEnabled(false);
    m_noLatencyCamera->filmSettings().setTemporalAntialiasingEnabled(false);
    m_noLatencyCamera->filmSettings().setEffectsEnabled(false);

	updateMouseSensitivity();			// Update (apply) mouse sensitivity
	updateSessionDropDown();			// Update the session drop down to remove already completed sessions
	updateSessionPress();				// Update session to create results file/start collection
	
	if (m_testMode) {
		// Remove the menu
		m_userSettingsMode = false;
		m_userSettingsWindow->setVisible(m_userSettingsMode);
	}
}

/** Handle then user settings window visibility */
void App::openUserSettingsWindow() {
    m_userSettingsMode = true;
    m_userSettingsWindow->setVisible(m_userSettingsMode);
}

/** Update the mouse mode/sensitivity */
void App::updateMouseSensitivity() {
    // G3D expects mouse sensitivity in radians
    // we're converting from mouseDPI and centimeters/360 which explains
    // the screen resolution (dots), cm->in factor (2.54) and 2PI
	const double metersPerInch = 39.3701;
	double metersPer360 = userTable.getCurrentUser()->cmp360 / 100.0;
	double pixelsPerMeter = userTable.getCurrentUser()->mouseDPI * metersPerInch;
    double m_pixelsToRadians = (2.0 * pi()) / (pixelsPerMeter * metersPer360);
    const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
    if (m_userSettingsMode) {
        // set to 3rd person
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
    }
    else {
        // Force into FPS mode
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
    }
	// Control player motion using the experiment config parameter
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
	if (notNull(player)) {
		player->m_pixelsToRadians = (float)m_pixelsToRadians;
		player->turnScale = sessConfig->player.turnScale * userTable.getCurrentUser()->turnScale;
	}
}

/** Spawn a randomly parametrized target */
void App::spawnParameterizedRandomTarget(float motionDuration=4.0f, float motionDecisionPeriod=0.5f, float speed=2.0f, float radius=10.0f, float scale=2.0f) {
    Random& rng = Random::threadCommon();

    // Construct a reference frame
    // Remove the vertical component
    Vector3 Z = -activeCamera()->frame().lookVector();
    debugPrintf("lookatZ = [%.4f, %.4f, %.4f]\n", Z.x, Z.y, Z.z);
    debugPrintf("origin  = [%.4f, %.4f, %.4f]\n", activeCamera()->frame().translation.x, activeCamera()->frame().translation.y, activeCamera()->frame().translation.z);
    Z.y = 0.0f;
    Z = Z.direction();
    Vector3 Y = Vector3::unitY();
    Vector3 X = Y.cross(Z);

    // Make a random vector in front of the player in a narrow field of view
    Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.5f, 0.5f)).direction();

    // Ray from user/camera toward intended spawn location
    Ray ray = Ray::fromOriginAndDirection(activeCamera()->frame().translation, dir);

    //distance = rng.uniform(2.0f, distance - 1.0f);
    const shared_ptr<FlyingEntity>& target =
        spawnTarget(ray.origin() + ray.direction() * radius,
            scale, false,
            Color3::wheelRandom());

    // Choose some destination locations based on speed and motionDuration
    const Point3& center = ray.origin();
    Array<Point3> destinationArray;
    // [radians/s] = [m/s] / [m/radians]
    float angularSpeed = speed / radius;
    // [rad] = [rad/s] * [s] 
    float angleChange = angularSpeed * motionDecisionPeriod;

    destinationArray.push(target->frame().translation);
    int tempInt = 0;
    for (float motionTime = 0.0f; motionTime < motionDuration; motionTime += motionDecisionPeriod) {
        // TODO: make angle change randomize correction, should be placed on circle around previous point
        float pitch = 0.0f;
        float yaw = tempInt++ % 2 == 0 ? angleChange : -angleChange;
        //float yaw = rng.uniform(-angleChange, angleChange);
        //float pitch = rng.uniform(-angleChange, angleChange);
        const Vector3& dir = CFrame::fromXYZYPRRadians(0.0f, 0.0f, 0.0f, yaw, pitch, 0.0f).rotation * ray.direction();
        ray.set(ray.origin(), dir);
        destinationArray.push(center + dir * radius);
    }
    target->setSpeed(speed); // m/s
    // debugging prints
    for (Point3* p = destinationArray.begin(); p != destinationArray.end(); ++p) {
        debugPrintf("[%.2f, %.2f, %.2f]\n", p->x, p->y, p->z);
    }
    target->setDestinations(destinationArray, center);
}

/** Spawn a random non-parametrized target */
void App::spawnRandomTarget() {
	Random& rng = Random::threadCommon();

	bool done = false;
	int tries = 0;

	// Construct a reference frame
	// Remove the vertical component
	Vector3 Z = -activeCamera()->frame().lookVector();
	Z.y = 0.0f;
	Z = Z.direction();
	Vector3 Y = Vector3::unitY();
	Vector3 X = Y.cross(Z);

	do {
		// Make a random vector in front of the player in a narrow field of view
		Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.3f, 0.5f)).direction();

		// Make sure the spawn location is visible
		Ray ray = Ray::fromOriginAndDirection(activeCamera()->frame().translation, dir);
		float distance = finf();
		scene()->intersect(ray, distance);

		if ((distance > 2.0f) && (distance < finf())) {
            distance = rng.uniform(2.0f, distance - 1.0f);
			const shared_ptr<FlyingEntity>& target =
                spawnTarget(ray.origin() + ray.direction() * distance, 
                    rng.uniform(0.1f, 1.5f), rng.uniform() > 0.5f,
                    Color3::wheelRandom());

            // Choose some destination locations
            const Point3& center = ray.origin();
            Array<Point3> destinationArray;
            destinationArray.push(target->frame().translation);
            for (int i = 0; i < 20; ++i) {
        		const Vector3& dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.3f, 0.5f)).direction();
                destinationArray.push(center + dir * distance);
            }
            target->setSpeed(2.0f); // m/s
            target->setDestinations(destinationArray, center);

			done = true;
		}
		++tries;
	} while (!done && tries < 100);
}

/** Spawn a flying entity target */
shared_ptr<FlyingEntity> App::spawnTarget(const Point3& position, float scale, bool spinLeft, const Color3& color, String modelName) {
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);

	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(format("target%03d", ++m_lastUniqueID), scene().get(), m_targetModels[modelName][scaleIndex], CFrame());

	UniversalMaterial::Specification materialSpecification;
	materialSpecification.setLambertian(Texture::Specification(color));
	materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
	materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));

	const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
	amPose->materialTable.set("core/icosahedron_default", UniversalMaterial::create(materialSpecification));
	target->setPose(amPose);

	target->setFrame(position);
	/*
	// Don't set a track. We'll take care of the positioning after creation
	String animation = format("combine(orbit(0, %d), CFrame::fromXYZYPRDegrees(%f, %f, %f))", spinLeft ? 1 : -1, position.x, position.y, position.z);
	const shared_ptr<Entity::Track>& track = Entity::Track::create(target.get(), scene().get(), Any::parse(animation));
	target->setTrack(track);
	*/

	target->setShouldBeSaved(false);
	targetArray.append(target);
	scene()->insert(target);
	return target;
}

shared_ptr<TargetEntity> App::spawnDestTarget(const Point3 position, Array<Destination> dests, float scale, const Color3& color,
	 String id, int paramIdx, int respawns, String name, bool isLogged) {	
	// Create the target
	String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	const shared_ptr<TargetEntity>& target = TargetEntity::create(dests, nameStr, scene().get(), m_targetModels[id][scaleIndex], scaleIndex, CFrame(), paramIdx, position, respawns, isLogged);

	// Setup the texture
	UniversalMaterial::Specification materialSpecification;
	materialSpecification.setLambertian(Texture::Specification(color));
	materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
	materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));
	
	const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
	amPose->materialTable.set("core/icosahedron_default", UniversalMaterial::create(materialSpecification));

	// Apply texture/position to target
	target->setPose(amPose);
	target->setFrame(position);
	target->setShouldBeSaved(false);

	// Add target to array and scene
	targetArray.append(target);
	scene()->insert(target);

	return target;
}

shared_ptr<FlyingEntity> App::spawnFlyingTarget(
	const Point3& position,
	float scale,
	const Color3& color,
	const Vector2& speedRange,
	const Vector2& motionChangePeriodRange,
	bool upperHemisphereOnly,
	Point3 orbitCenter,
	String id,
	int paramIdx,
	Array<bool> axisLock,
	int respawns,
	String name,
	bool isLogged)
{
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(
		nameStr,
		scene().get(),
		m_targetModels[id][scaleIndex],
		scaleIndex,
		CFrame(),
		speedRange,
		motionChangePeriodRange,
		upperHemisphereOnly,
		orbitCenter,
		paramIdx,
		axisLock,
		respawns,
		isLogged
	);

	UniversalMaterial::Specification materialSpecification;
	materialSpecification.setLambertian(Texture::Specification(color));
	materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
	materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));

	const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
	amPose->materialTable.set("core/icosahedron_default", UniversalMaterial::create(materialSpecification));
	target->setPose(amPose);

	target->setFrame(position);

	target->setShouldBeSaved(false);
	targetArray.append(target);
	scene()->insert(target);
	return target;
}

shared_ptr<JumpingEntity> App::spawnJumpingTarget(
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
	String id,
	int paramIdx,
	Array<bool> axisLock,
	int respawns,
	String name,
	bool isLogged)
{
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_modelScaleCount - 1);
	String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const shared_ptr<JumpingEntity>& target = JumpingEntity::create(
		nameStr,
		scene().get(),
		m_targetModels[id][scaleIndex],
		scaleIndex,
		CFrame(),
		speedRange,
		motionChangePeriodRange,
		jumpPeriodRange,
		distanceRange,
		jumpSpeedRange,
		gravityRange,
		orbitCenter,
		targetDistance,
		paramIdx,
		axisLock,
		respawns,
		isLogged
	);

	UniversalMaterial::Specification materialSpecification;
	materialSpecification.setLambertian(Texture::Specification(color));
	materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
	materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));

	const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
	amPose->materialTable.set("core/icosahedron_default", UniversalMaterial::create(materialSpecification));
	target->setPose(amPose);

	target->setFrame(position);

	target->setShouldBeSaved(false);
	targetArray.append(target);
	scene()->insert(target);
	return target;
}

void App::loadDecals() {
	Any decalSpec = PARSE_ANY(ArticulatedModel::Specification{
	filename = "ifs/square.ifs";
	preprocess = {
		transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));
		setMaterial(all(), UniversalMaterial::Specification{
			lambertian = Texture::Specification {
				filename = "bullet-decal-256x256.png";
				encoding = Color3(1, 1, 1);
			};
		});
	};
		});
	decalSpec.set("scale", sessConfig->weapon.decalScale);
	m_decalModel = ArticulatedModel::create(decalSpec, "decalModel");
}

void App::loadModels() {
	if ((experimentConfig.weapon.renderModel || startupConfig.developerMode) && !experimentConfig.weapon.modelSpec.filename.empty()) {
		// Load the model if we (might) need it
		m_viewModel = ArticulatedModel::create(experimentConfig.weapon.modelSpec, "viewModel");
	}

	const static Any bulletSpec = PARSE_ANY(ArticulatedModel::Specification{
		filename = "ifs/d10.ifs";
		preprocess = {
			transformGeometry(all(), Matrix4::pitchDegrees(90));
			transformGeometry(all(), Matrix4::scale(0.05,0.05,2));
			setMaterial(all(), UniversalMaterial::Specification {
				lambertian = Color3(0);
			emissive = Power3(5,4,0);
			});
		}; });

	m_bulletModel = ArticulatedModel::create(bulletSpec, "bulletModel");

	// Add all the unqiue targets to this list
	Table<String, Any> toBuild;
	for (TargetConfig target : experimentConfig.targets) {
		toBuild.set(target.id, target.modelSpec);
	}
	// Append the basic model automatically (used for reference targets for now)
	toBuild.set("reference", PARSE_ANY(ArticulatedModel::Specification{
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
		scale = 0.2; // TODO: Ideally this should configurable and defined by Experiment class.
	}));

	// Setup the explosion specification
	Any explosionSpec = PARSE_ANY(ArticulatedModel::Specification{
		filename = "ifs/square.ifs";
		preprocess = {
			transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));
			//scaleAndOffsetTexCoord0(all(), 0.0769, 0);
			setMaterial(all(), UniversalMaterial::Specification{
				lambertian = Texture::Specification {
					//filename = "explosion_01_strip13.png";
					filename = "explosion_01.png";
					encoding = Color3(1, 1, 1);
				};
			});
		}; 
	});
	for (int i = 0; i < m_modelScaleCount; i++) {
		const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - TARGET_MODEL_ARRAY_OFFSET);
		explosionSpec.set("scale", scale*20.0f);
		m_explosionModels.push(ArticulatedModel::create(explosionSpec));
	}

	// Scale the models into the m_targetModel table
	for (String id : toBuild.getKeys()) {
		// Get the any spec
		Any spec = toBuild.get(id);
		// Get the bounding box to scale to size rather than arbitrary factor
		shared_ptr<ArticulatedModel> size_model = ArticulatedModel::create(ArticulatedModel::Specification(spec));
		AABox bbox;
		size_model->getBoundingBox(bbox);
		Vector3 extent = bbox.extent();
		logPrintf("%20s bounding box: [%2.2f, %2.2f, %2.2f]\n", id.c_str(), extent[0], extent[1], extent[2]);
		float default_scale = 1.0f / extent[0];					// Setup scale so that default model is 1m across

		Array<shared_ptr<ArticulatedModel>> models;
		for (int i = 0; i <= m_modelScaleCount; ++i) {
			const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - TARGET_MODEL_ARRAY_OFFSET);
			spec.set("scale", scale*default_scale);
			models.push(ArticulatedModel::create(spec));
		}
		m_targetModels.set(id, models);
	}

	// Create a series of colored materials to choose from for target health
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

void App::updateControls() {
	// Update the waypoint manager
	waypointManager->updateControls();

	// Setup the player control
	m_playerControls = PlayerControls::create(*sessConfig, std::bind(&App::exportScene, this), theme);
	m_playerControls->setVisible(false);
	this->addWidget(m_playerControls);

	// Setup the render control
	m_renderControls = RenderControls::create(*sessConfig, *(userTable.getCurrentUser()), renderFPS, emergencyTurbo, numReticles, sceneBrightness, theme, MAX_HISTORY_TIMING_FRAMES);
	m_renderControls->setVisible(false);
	this->addWidget(m_renderControls);

	m_weaponControls = WeaponControls::create(sessConfig->weapon, theme);
	m_weaponControls->setVisible(false);
	this->addWidget(m_weaponControls);
}

void App::makeGUI() {
	debugWindow->setVisible(startupConfig.developerMode);
	developerWindow->setVisible(startupConfig.developerMode);
	developerWindow->sceneEditorWindow->setVisible(startupConfig.developerMode);
	developerWindow->cameraControlWindow->setVisible(startupConfig.developerMode);
	developerWindow->videoRecordDialog->setEnabled(true);
	developerWindow->videoRecordDialog->setCaptureGui(true);

	theme = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"));

	// Add the control panes here
	updateControls();

	// Open sub-window panes here...
	debugPane->beginRow(); {
		debugPane->addButton("Render Controls [1]", this, &App::showRenderControls);
		debugPane->addButton("Player Controls [2]", this, &App::showPlayerControls);
		debugPane->addButton("Weapon Controls [3]", this, &App::showWeaponControls);
		if(startupConfig.waypointEditorMode) debugPane->addButton("Waypoint Manager [4]", waypointManager, &WaypointManager::showWaypointWindow);
	}debugPane->endRow();

    // set up user settings window
    m_userSettingsWindow = GuiWindow::create("User Settings", nullptr, Rect2D::xywh(0.0f, 0.0f, 10.0f, 10.0f));
    GuiPane* p = m_userSettingsWindow->pane();
    GuiTabPane* tabPane = p->addTabPane();
    {
        m_currentUserPane = tabPane->addTab("Current User Settings");
        updateUserGUI();

        m_ddCurrentUser = userTable.getCurrentUserIndex();


        GuiPane* p2 = m_currentUserPane->addPane("Experiment Settings");
        p2->beginRow();
        m_userDropDown = p2->addDropDownList("User", userTable.getIds(), &m_ddCurrentUser);
        p2->addButton("Select User", this, &App::updateUser);
        p2->endRow();
        p2->beginRow();
        m_sessDropDown = p2->addDropDownList("Session", Array<String>({}), &m_ddCurrentSession);
        updateSessionDropDown();
        p2->addButton("Select Session", this, &App::updateSessionPress);
        p2->endRow();
        m_currentUserPane->pack();
    }
    {
        GuiPane* gp = tabPane->addTab("Late Warp Settings");
        gp->addNumberBox("Framerate",
            Pointer<float>(
                [&]() { return 1.0f / float(realTimeTargetDuration()); },
                [&](float f) {
            setFramerate(f);
        }),
            "Hz", GuiTheme::LOG_SLIDER, 30.0f, 5000.0f);

        gp->addNumberBox("Display Lag",
            Pointer<int>(
                [&]() { return m_simulationLagFrames; },
                [&](int i) {
            setLatencyFrames(i);
        }),
            "f", GuiTheme::LINEAR_SLIDER, 0, 60);

        GuiControl* b = gp->addDropDownList("Latewarp", m_warpCompensation, &m_selectedWarpCompensation, nullptr);
        GuiControl* c = nullptr;
        c = gp->addDropDownList("Method", m_warpMethodList, &m_selectedMethod, nullptr); c->moveRightOf(b, 10.0f);
        //gp->addLabel("Hit decision");
        GuiPane* gp2 = gp->addPane("Hit decision");
        c = gp2->addDropDownList("Input", m_inputTimingList, &m_selectedInputTiming, nullptr); c->moveBy(50.0f, 0.0f);
        c = gp2->addDropDownList("Game State", m_gameStateTimingList, &m_selectedGameStateTiming, nullptr); c->moveBy(50.0f, 0.0f);
        gp2->addCheckBox("Show SOL", &m_showSOL);
        GuiControl* d = gp->addCheckBox("Visualize Latency", &m_latencyVisualizeEnable);
        gp->pack();
    }

    p->addButton("Quit", this, &App::quitRequest);

	m_userSettingsWindow->pack();
	float scale = 0.5f / m_userSettingsWindow->pixelScale();
	Vector2 pos = Vector2(renderDevice->viewport().width()*scale - m_userSettingsWindow->bounds().width() / 2.0f,
		renderDevice->viewport().height()*scale - m_userSettingsWindow->bounds().height() / 2.0f);
	m_userSettingsWindow->moveTo(pos);
	m_userSettingsWindow->setVisible(m_userSettingsMode);
	addWidget(m_userSettingsWindow);

	debugWindow->pack();
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->renderDevice()->viewport().width(), debugWindow->rect().height()));
	m_debugMenuHeight = startupConfig.developerMode ? debugWindow->rect().height() : 0.0f;
}

void App::exportScene() {
	CFrame frame = scene()->typedEntity<PlayerEntity>("player")->frame();
	logPrintf("Player position is: [%f, %f, %f]\n", frame.translation.x, frame.translation.y, frame.translation.z);
	String filename = Scene::sceneNameToFilename(sessConfig->sceneName);
	scene()->toAny().save(filename);
}

void App::showPlayerControls() {
	m_playerControls->setVisible(true);
}

void App::showRenderControls() {
	m_renderControls->setVisible(true);
}

void App::showWeaponControls() {
	m_weaponControls->setVisible(true);
}

void App::userSaveButtonPress(void) {
	// Save the any file
	Any a = Any(userTable);
	a.save(startupConfig.userConfig());
	logPrintf("User table saved.\n");			// Print message to log
}	

void App::updateUser(void){
	// Update the user if needed
	if (m_lastSeenUser != m_ddCurrentUser) {
		// This creates a new results file...
		if(m_sessDropDown->numElements() > 0) updateSession(updateSessionDropDown()[0]);
		String id = getDropDownUserId();
		m_lastSeenUser = m_ddCurrentUser;

        userTable.currentUser = id;
        updateUserGUI();
	}
	// Get new session list for (new) user
	updateSessionDropDown();
}

void App::updateUserGUI() {
    m_currentUserPane->removeAllChildren();
	UserConfig *user = userTable.getCurrentUser();
    m_currentUserPane->addLabel(format("Current User: %s", userTable.currentUser));
    m_mouseDPILabel = m_currentUserPane->addLabel(format("Mouse DPI: %f", user->mouseDPI));
    m_currentUserPane->addNumberBox("Mouse 360", &(user->cmp360), "cm", GuiTheme::LINEAR_SLIDER, 0.2, 100.0, 0.2);
	m_currentUserPane->addNumberBox("Turn Scale X", &(user->turnScale.x), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f);
	m_currentUserPane->addNumberBox("Turn Scale Y", &(user->turnScale.y), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f);
    m_currentUserPane->addButton("Save settings", this, &App::userSaveButtonPress);
}

Array<String> App::updateSessionDropDown(void) {
	// Create updated session list
    String userId = userTable.getCurrentUser()->id;
	shared_ptr<UserSessionStatus> userStatus = userStatusTable.getUserStatus(userId);
	// If we have a user that doesn't have specified sessions
	if (userStatus == nullptr) {
		// Create a new user session status w/ no progress and default order
		logPrintf("User %s not found. Creating a new user w/ default session ordering.\n", userId);
		UserSessionStatus newStatus = UserSessionStatus();
		newStatus.id = userId;
		experimentConfig.getSessionIds(newStatus.sessionOrder);
		userStatusTable.userInfo.append(newStatus);
		userStatus = userStatusTable.getUserStatus(userId);
		userStatusTable.toAny().save("userstatus.Any");
	}

	Array<String> remainingSess = {};
	if (userStatusTable.allowRepeat) {
		remainingSess = userStatus->sessionOrder;
		for (int i = 0; i < userStatus->completedSessions.size(); i++) {
			if (remainingSess.contains(userStatus->completedSessions[i])) {
				int idx = remainingSess.findIndex(userStatus->completedSessions[i]);
				remainingSess.remove(idx, 1);
			}
		}
	}
	else{
		for (int i = 0; i < userStatus->sessionOrder.size(); i++) {
			if(!userStatus->completedSessions.contains(userStatus->sessionOrder[i])) {
				// user hasn't (ever) completed this session
				remainingSess.append(userStatus->sessionOrder[i]);
			}
		}
	}
	m_sessDropDown->setList(remainingSess);

	// Print message to log
	logPrintf("Updated %s's session drop down to:\n", userId);
	for (String id : remainingSess) {
		logPrintf("\t%s\n", id);
	}

	return remainingSess;
}

String App::getDropDownSessId(void) {
	if (m_sessDropDown->numElements() == 0) return "";
	return m_sessDropDown->get(m_ddCurrentSession);
}

String App::getDropDownUserId(void) {
	return m_userDropDown->get(m_ddCurrentUser);
}

void App::presentQuestion(Question question) {
	switch (question.type) {
	case Question::Type::MultipleChoice:
		dialog = SelectionDialog::create(question.prompt, question.options, theme, question.title);
		break;
	case Question::Type::Entry:
		dialog = TextEntryDialog::create(question.prompt, theme, question.title);
		break;
	case Question::Type::Rating:
		dialog = RatingDialog::create(question.prompt, question.options, theme, question.title);
		break;
	default:
		throw "Unknown question type!";
		break;
	}
	this->addWidget(dialog);
	openUserSettingsWindow();
}

void App::markSessComplete(String sessId) {
	if (m_pyLogger != nullptr) {
		m_pyLogger->mergeLogToDb();
	}
	// Add the session id to completed session array
	userStatusTable.addCompletedSession(userTable.currentUser, sessId);
	// Save the file to any
	userStatusTable.toAny().save("userstatus.Any");
	logPrintf("Marked session: %s complete for user %s.\n", sessId, userTable.currentUser);
}

shared_ptr<UserConfig> App::getCurrUser(void) {
	return userTable.getUserById(getDropDownUserId());
}

void App::updateSessionPress(void) {
	updateSession(getDropDownSessId());
}

void App::updateParameters() {
	// Set a maximum *finite* frame rate
	float frameRate = lastSetFrameRate > 1e-5f ? lastSetFrameRate : float(window()->settings().refreshRate);
	float dt = 1.0f / frameRate;
	const float current = (float)realTimeTargetDuration();
	if (abs(dt - current) > 1e-5f) {
		// Only set when there is a change, otherwise the simulation's deltas are confused.
		setFrameDuration(dt, GApp::REAL_TIME);
	}
}

void App::setFramerate(float frameRate)
{
	if (frameRate < 10.f) {
		frameRate = 10.f; // minimum frame rate is 10 hz
	}
	if (frameRate > 360.f) {
		frameRate = 360.f; // maximum frame rate is 360 hz
	}
	lastSetFrameRate = frameRate;
	updateParameters();
}

void App::setLatencyFrames(int frameDelay)
{
	if (frameDelay < 0) {
		frameDelay = 0;
	}
	m_simulationLagFrames = frameDelay;
	updateParameters();
}

void App::updateSession(const String& id) {
	// Check for a valid ID (non-emtpy and 
	Array<String> ids;
	experimentConfig.getSessionIds(ids);
	if (!id.empty() && ids.contains(id)) {
		sessConfig = experimentConfig.getSessionConfigById(id);						// Get the new session config
		logPrintf("User selected session: %s. Updating now...\n", id);				// Print message to log
		m_sessDropDown->setSelectedValue(id);										// Update session drop-down selection
		sess = Session::create(this, sessConfig);									// Create the session
	} else {
		sessConfig = SessionConfig::create();										// Create an empty session
		sess = Session::create(this, sessConfig);											
	}

	// Update the controls for this session
	updateControls();

	// Update the frame rate/delay
	setFramerate(sessConfig->render.frameRate);
	setLatencyFrames(sessConfig->render.frameDelay);

	// Update latewarp configuration
	if (sessConfig->render.warpMethod == "none") {
		m_selectedWarpCompensation = 0;
        m_selectedMethod = 0;
        m_selectedInputTiming = 0;
        m_selectedGameStateTiming = 0;
	}
	else if (sessConfig->render.warpMethod == "RIW") {
		m_selectedWarpCompensation = 1;
		m_selectedMethod = 0;
		m_selectedInputTiming = 1;
		m_selectedGameStateTiming = 1;
	}
    else if (sessConfig->render.warpMethod == "RCW") {
        m_selectedWarpCompensation = 1;
        m_selectedMethod = 1;
        m_selectedInputTiming = 1;
        m_selectedGameStateTiming = 1;
    }
    else if (sessConfig->render.warpMethod == "FCW") {
        m_selectedWarpCompensation = 2;
        m_selectedMethod = 1;
        m_selectedInputTiming = 1;
        m_selectedGameStateTiming = 1;
    }

	// Load (session dependent) fonts
	hudFont = GFont::fromFile(System::findDataFile(sessConfig->hud.hudFont));
	m_combatFont = GFont::fromFile(System::findDataFile(sessConfig->targetView.combatTextFont));

	// Handle clearing the targets here (clear any remaining targets before loading a new scene)
	if (notNull(scene())) clearTargets();

	// Load the experiment scene if we haven't already (target only)
	if (sessConfig->sceneName.empty()) {
		if (m_loadedScene.empty()) {		// No scene specified
			loadScene(m_defaultScene);		// Use this as the default
			m_loadedScene = m_defaultScene;
		}
		// Otherwise just let the loaded scene persist
	}
	else if (sessConfig->sceneName != m_loadedScene) {
		loadScene(sessConfig->sceneName);
		m_loadedScene = sessConfig->sceneName;
	}

	loadDecals();

	// Check for play mode specific parameters
	m_fireSound = Sound::create(System::findDataFile(sessConfig->weapon.fireSound));
	m_sceneHitSound = Sound::create(System::findDataFile(sessConfig->audio.sceneHitSound));

	// Update weapon model (if drawn)
	if (sessConfig->weapon.renderModel) {
		m_viewModel = ArticulatedModel::create(sessConfig->weapon.modelSpec, "viewModel");
	}

	// Create a series of colored materials to choose from for target health
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
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
	sess->initialHeadingRadians = player->heading();

	player->m_pixelsToRadians = 0.0f;
	player->turnScale = Vector2::zero();
	updateMouseSensitivity();
	player->moveRate =		&sessConfig->player.moveRate;
	player->moveScale =		&sessConfig->player.moveScale;
	player->axisLock =		&sessConfig->player.axisLock;
	player->jumpVelocity =	&sessConfig->player.jumpVelocity;
	player->jumpInterval =	&sessConfig->player.jumpInterval;
	player->jumpTouch =		&sessConfig->player.jumpTouch;
	player->height =		&sessConfig->player.height;
	player->crouchHeight =	&sessConfig->player.crouchHeight;

	// Make sure all targets are cleared
	clearTargets();

	// Check for need to start latency logging and if so run the logger now
	SystemConfig sysConfig = SystemConfig::load();
	String logName = "../results/" + id + "_" + userTable.currentUser + "_" + String(Logger::genFileTimestamp());
	if (sysConfig.hasLogger) {
		if (!sessConfig->clickToPhoton.enabled) {
			logPrintf("WARNING: Using a click-to-photon logger without the click-to-photon region enabled!\n\n");
		}
		if (m_pyLogger == nullptr) {
			m_pyLogger = PythonLogger::create(sysConfig.loggerComPort, sysConfig.hasSync, sysConfig.syncComPort);
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
	if (m_sessDropDown->numElements() == 0) {
		logPrintf("No sessions remaining for selected user.\n");
	}
	else {
		logPrintf("Created results file: %s.db\n", logName.c_str());
	}
}

void App::quitRequest() {
	// End session logging
	if (sess != nullptr) {
		sess->endLogging();
	}
	// Merge Python log into session log (if logging)
	if (m_pyLogger != nullptr) {
		// Merge (blocking here to keep from killing merge...)
		m_pyLogger->mergeLogToDb(true);
	}
    setExitCode(0);
}

void App::onAfterLoadScene(const Any& any, const String& sceneName) {
	// Pick between experiment and session settings
	Vector3 grav = experimentConfig.player.gravity;
	float FoV = experimentConfig.render.hFoV;
	if (sessConfig != nullptr) {
		grav = sessConfig->player.gravity;
		FoV = sessConfig->render.hFoV;
	}
	// Set the active camera to the player
	setActiveCamera(scene()->typedEntity<Camera>("camera"));

    // make sure the scene has a "player" entity
    if (isNull(scene()->typedEntity<PlayerEntity>("player"))) {
        shared_ptr<Entity> newPlayer = PlayerEntity::create("player", scene().get(), CFrame(), nullptr);
        scene()->insert(newPlayer);
    }

	// Get the reset height
	shared_ptr<PhysicsScene> pscene = typedScene<PhysicsScene>();
	pscene->setGravity(grav);
	float resetHeight = pscene->resetHeight();
	if (isnan(resetHeight)) {
		resetHeight = -1e6;
	}

	// For now make the player invisible (prevent issues w/ seeing model from inside)
	shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
	player->setVisible(false);
	player->setRespawnHeight(resetHeight);
	player->setRespawnPosition(player->frame().translation);
	activeCamera()->setFieldOfView(FoV * units::degrees(), FOVDirection::HORIZONTAL);
}

void App::onAI() {
	GApp::onAI();
	// Add non-simulation game logic and AI code here
}


void App::onNetwork() {
	GApp::onNetwork();
	// Poll net messages here
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) {
	BEGIN_PROFILER_EVENT("App::onGraphics3D");

	CFrame camraToWorldFrame = activeCamera()->frame();
	Projection projection = activeCamera()->projection();
	shared_ptr<Texture> noLatencyFrame; // points to a m_ldrDelayBufferQueue element, if used
	
	rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

	if (m_displayLagFrames > 0) {
		// Should only be delaying real frames when latency simulation is done with a frame queue
		assert(m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_FRAMES);

		// Need one more frame in the queue than we have frames of delay, to hold the current frame
		if (m_ldrDelayBufferQueue.size() < m_displayLagFrames) {
			// Allocate new textures
			int framesToAdd = m_displayLagFrames - m_ldrDelayBufferQueue.size();
			for (int i = 0; i < framesToAdd; ++i) {
				DelayedFrame frame;
				frame.fb = Framebuffer::create(Texture::createEmpty(format("Delay buffer %d", m_ldrDelayBufferQueue.size()), rd->width(), rd->height(), ImageFormat::RGB8()));
				frame.mouseState = true;
				m_ldrDelayBufferQueue.push(frame);
			}
			debugAssert(m_ldrDelayBufferQueue.size() == m_displayLagFrames);
		}

		// Advance the pointer to the next, which is also the oldest frame
		m_currentDelayBufferIndex = (m_currentDelayBufferIndex + 1) % m_displayLagFrames;
		auto& texture = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].fb->texture(0);
		m_mouseStateDelayed = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].mouseState;
		
		BEGIN_PROFILER_EVENT("Draw delay frames");
		rd->push2D();
		if (m_latencyConfig.latewarpMethod != LatencyConfig::LATEWARP_NONE &&
			m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_FRAMES) {

			if (m_latencyConfig.latewarpMethod == LatencyConfig::LATEWARP_FULLTRANSFORM) {
				debugPrintf("Warning: LATEWARP_FULLTRANSFORM not implemented with LATENCY_DELAY_FRAMES\n");
			}

			// Set the on-screen view to the one we're late-warping to
			m_displayedCameraFrame = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].view;
			m_displayedCameraFrame.rotation = camraToWorldFrame.rotation;

			// Get view information for reprojection
			auto oldCameraToWorld = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].view.rotation;
			auto currentWorldToCamera = camraToWorldFrame.rotation.transpose();
			Matrix3 oldToCurrentCamera = currentWorldToCamera * oldCameraToWorld;

			// Fix the projection as it includes the guardband
			float hfov = projection.fieldOfViewAngle();
			hfov = 2.0f * atan(tan(hfov * 0.5f) * rd->viewport().width() / m_gbuffer->colorRect().width());
			projection.setFieldOfViewAngle(hfov);

			// Build the camera's projection matrix
			Matrix4 projectionMatrix;
			projection.getProjectUnitMatrix(rd->viewport(), projectionMatrix);
			projectionMatrix = projectionMatrix * Matrix4::scale(1, -1, 1); // Because getProjectUnitMatrix says to

			// Build reprojection matrix - the delta between two images
			auto transform = projectionMatrix * Matrix4(oldToCurrentCamera) * projectionMatrix.inverse();

			Args args;
			args.setUniform("sourceTexture", texture, Sampler::buffer());
			args.setUniform("transform", transform);
			//args.setUniform("viewDelta", viewDelta);
			//args.setUniform("projection", projection);
			//args.setUniform("projectionInv", projection.inverse());
			args.setRect(rd->viewport());
			LAUNCH_SHADER("warp-3dof.*", args);
		} else {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), texture, Sampler::buffer());

			// Set the on-screen view to the one just copied over
			m_displayedCameraFrame = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].view;
		}
	
		if (m_dumpNextFrame) {
			const shared_ptr<Image>& screenshot = rd->screenshotPic();
			String delay = "late";
			screenshot->save(format("screenshots/%s_%s_%s.png", m_frameDumpFilename.c_str(), delay.c_str(), m_latencyConfig.getConfigShortName().c_str()));
		}

		rd->pop2D();
		END_PROFILER_EVENT();

		// Prepare to render into the current position of the delay queue
		rd->pushState(m_ldrDelayBufferQueue[m_currentDelayBufferIndex].fb);
		m_ldrDelayBufferQueue[m_currentDelayBufferIndex].view = camraToWorldFrame;
		m_ldrDelayBufferQueue[m_currentDelayBufferIndex].mouseState = m_buttonUp;
		noLatencyFrame = m_ldrDelayBufferQueue[m_currentDelayBufferIndex].fb->texture(0);
	} else {
		// Record the view we used to render the scene
		m_displayedCameraFrame = camraToWorldFrame;
	}

	/*
	scene()->lightingEnvironment().ambientOcclusionSettings.enabled = !emergencyTurbo;
	activeCamera()->filmSettings().setAntialiasingEnabled(!emergencyTurbo);
	activeCamera()->filmSettings().setBloomStrength(emergencyTurbo ? 0.0f : 0.5f);
	*/

	GApp::onGraphics3D(rd, surface);
	
	if (m_dumpNextFrame) {
		const shared_ptr<Image>& screenshot = rd->screenshotPic();
		String delay = m_displayLagFrames > 0 ? "real" : "late";
		screenshot->save(format("screenshots/%s_%s_%s.png", m_frameDumpFilename.c_str(), delay.c_str(), m_latencyConfig.getConfigShortName().c_str()));
	}

	if (m_displayLagFrames > 0) {
		rd->popState();
	}

	// Render again, creating a view without latency added
	if (m_latencyVisualizeEnable) {
		BEGIN_PROFILER_EVENT("Visualize latency");
		if (m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_POSES) {
			if (!m_latencyVisualizeFB) {
				m_latencyVisualizeFB = Framebuffer::create(Texture::createEmpty("Latency visualization", rd->width(), rd->height(), ImageFormat::RGB8()));
			}

			rd->pushState(m_latencyVisualizeFB);

			// A separate gbuffer is needed or rendering twice takes a huge perf hit.
			if (!m_latencyVisualizeGbuffer) {
				m_latencyVisualizeGbuffer = GBuffer::create(m_gbufferSpecification);
			}
			auto gbBackup = m_gbuffer;
			m_gbuffer = m_latencyVisualizeGbuffer;

			auto oldCamera = activeCamera();
			setActiveCamera(m_noLatencyCamera);
			scene()->lightingEnvironment().ambientOcclusionSettings.enabled = false;
            
			if (m_stateQueue.size() > 0) {
				State& currentState = m_stateQueue.last();
#if 1
				GApp::onGraphics3D(rd, currentState.surfaceArray);
#else
				m_renderer->render(rd, m_lateWarpCamera, m_latencyVisualizeFB, nullptr, scene()->lightingEnvironment(), m_gbuffer, currentState.surfaceArray);
#endif
			}

			// Restore rendering state
			setActiveCamera(oldCamera);
			m_gbuffer = gbBackup;

			rd->popState();
			noLatencyFrame = m_latencyVisualizeFB->texture(0);
		}

		if (noLatencyFrame) {
			rd->push2D();
			rd->setBlendFunc(Framebuffer::COLOR0, RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA, RenderDevice::BLENDEQ_ADD);
			Draw::rect2D(rd->viewport(), rd, Color4(1.0f, 1.0f, 1.0f, 0.2f), noLatencyFrame, Sampler::buffer());
			rd->pop2D();
		}

		END_PROFILER_EVENT(); // Visualize latency
	}
    
	END_PROFILER_EVENT(); // onGraphics3D
}

Point2 App::getViewDirection()
{   // returns (azimuth, elevation), where azimuth is 0 deg when straightahead and + for right, - for left.
	Point3 view_cartesian = activeCamera()->frame().lookVector();
	float az = atan2(-view_cartesian.z, -view_cartesian.x) * 180 / pif();
	float el = atan2(view_cartesian.y, sqrtf(view_cartesian.x * view_cartesian.x + view_cartesian.z * view_cartesian.z)) * 180 / pif();
	return Point2(az, el);
}

Point3 App::getPlayerLocation()
{
	return activeCamera()->frame().translation;
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// Called manually because session is not registered with G3D
	sess->onSimulation(rdt, sdt, idt);

	// These are all we need from GApp::onSimulation() for walk mode
	m_widgetManager->onSimulation(rdt, sdt, idt);
	if (scene()) { scene()->onSimulation(sdt); }

	// make sure mouse sensitivity is set right
	if (m_userSettingsMode) {
		updateMouseSensitivity();
        // Make sure window stays coherent w/ user settings mode
		m_userSettingsWindow->setVisible(m_userSettingsMode);
	}

	const RealTime now = System::time();
	for (int p = 0; p < projectileArray.size(); ++p) {
		const Projectile& projectile = projectileArray[p];

		if (!m_hitScan) {
			// Check for collisions
		}

		if (projectile.endTime < now) {
			// Expire
			scene()->removeEntity(projectile.entity->name());
			projectileArray.fastRemove(p);
			--p;
		} else {
			// Animate
			projectile.entity->setFrame(projectile.entity->frame() + projectile.entity->frame().lookVector() * sessConfig->weapon.bulletSpeed);
		}
	}

    // TODO: Make an explosion entity array?
	// explosion animation
	if (notNull(m_explosion) && (m_explosionEndTime < now)) {
		scene()->remove(m_explosion);
		m_explosion = nullptr;
	} else {
		// could update animation here...
	}

	// Move the player
	const shared_ptr<PlayerEntity>& p = scene()->typedEntity<PlayerEntity>("player");
	activeCamera()->setFrame(p->getCameraFrame());

	// Handle developer mode features here
	if (startupConfig.developerMode) {
		// Handle frame rate/delay updates here
		if (sessConfig->render.frameRate != lastSetFrameRate || m_displayLagFrames != sessConfig->render.frameDelay) {
            setFramerate(sessConfig->render.frameRate);
            setLatencyFrames(sessConfig->render.frameDelay);
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
		const String& nextSess = userStatusTable.getNextSession(userTable.currentUser);
		updateSession(nextSess);
	}

    // Duplicate the camera frame with zero latency before we add latency to it
    m_noLatencyCamera->copyParametersFrom(activeCamera());
	m_noLatencyCamera->setFrame(activeCamera()->frame());
    
    // If we're faking frame latency by just delaying poses, update the camera here using
    // a delay queue. This is to delay everything we wouldn't normally be able to late-warp away.
    // Otherwise, latency will be simulated with a queue of rendered frames
	BEGIN_PROFILER_EVENT("Camera Enqueue");
    if ((m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_POSES) && (m_cameraDelayFrames > 0)) {
        if (m_cameraPoseQueue.size() == 0) {
            m_delayedCameraFrame = activeCamera()->frame();
        }

        // Since we're messing with the camera behind G3D's back,
        // the previous frame needs to be set too for motion vectors.
        activeCamera()->setPreviousFrame(m_delayedCameraFrame);

        // Update the queue to maintain a constant m_cameraDelayFrames number
        // of frames. Structure the code to allow the the m_cameraDelayFrames "constant"
        // to change suddenly due to GUI interaction, however.
        m_cameraPoseQueue.enqueue(activeCamera()->frame());
        while (m_cameraPoseQueue.size() > m_cameraDelayFrames) {
            m_delayedCameraFrame = m_cameraPoseQueue.dequeue();
        }
		
		// Add the mouse event. This must be kept in sync with the above pose queue.
		// Remove the excess, leaving the current mouse state at the front of the queue.
		m_mouseStatusDelayQueue.enqueue(m_buttonUp);
		while (m_mouseStatusDelayQueue.size() > m_cameraDelayFrames) {
			m_mouseStateDelayed = m_mouseStatusDelayQueue.dequeue();
		}

		if (m_latencyConfig.latewarpMethod == LatencyConfig::LATEWARP_ROTATION) {
			// If late-warping is enabled, copy over the more up-to-date rotation
			m_delayedCameraFrame.rotation = activeCamera()->frame().rotation;
		} else if (m_latencyConfig.latewarpMethod == LatencyConfig::LATEWARP_FULLTRANSFORM) {
			// Copy over the entire up to date transform
			m_delayedCameraFrame = activeCamera()->frame();
		}

        // Use the delayed camera frame instead and don't replace the previous frame.
        activeCamera()->setFrame(m_delayedCameraFrame, false);
    }
	END_PROFILER_EVENT();
}

bool App::onEvent(const GEvent& event) {
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
	
	// Handle normal keypresses
	if (event.type == GEventType::KEY_DOWN) {
		if (keyMap.map["openMenu"].contains(ksym)) {
			m_userSettingsMode = !m_userSettingsMode;
			m_userSettingsWindow->setVisible(m_userSettingsMode);
			if (m_userSettingsMode) {
				// set focus so buttons properly highlight
				m_widgetManager->setFocusedWidget(m_userSettingsWindow);
			}
			// switch to first or 3rd person mode
			updateMouseSensitivity();
			foundKey = true;
		}

		// Override 'q', 'z', 'c', and 'e' keys (unused)
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
		else if (keyMap.map["quit"].contains(ksym)) {
			quitRequest();
			return true;
		}
	}
	else if ((event.type == GEventType::KEY_UP)){
		if (keyMap.map["crouch"].contains(ksym)) {
			scene()->typedEntity<PlayerEntity>("player")->setCrouched(false);
			foundKey = true;
		}
	}
	if (foundKey) {
		return true;
	}

	// Handle window-based close ("X" button)
	if (event.type == GEventType::QUIT) {
		quitRequest();
		return true;
	}

	// Handle super-class events
	return GApp::onEvent(event);
}

void App::drawHittingEffect(RenderDevice *rd) {
	Color3 redColor = Color3::red();
	if (!gamerMoving && (hittingRenderCount <= 15 || hittingRenderCount > 30))
	{
		renderTheEffect = 1;
		hittingRenderCount++;
		//printf("Rendering! %d\n", hittingRenderCount);
	}
	else if (!gamerMoving && hittingRenderCount > 15 && hittingRenderCount < 30)
	{
		renderTheEffect = 0;
		hittingRenderCount++;
		//printf("Not Rendering! %d\n", hittingRenderCount);
	}
	else
	{
		hittingRenderCount = 0;
		renderTheEffect = 0;
		//printf("All Good! %d %.1f %d\n", gamerMoving, hittingRenderCount, renderTheEffect);
	}


	Draw::rect2D(
		Rect2D::xywh(
			(float)m_framebuffer->width() * 0.0f,
			(float)m_framebuffer->height() * 0.02f,
			(float)m_framebuffer->width() *  renderTheEffect,
			(float)m_framebuffer->height() * renderTheEffect * 0.04f
		), rd, redColor
	);
	Draw::rect2D(
		Rect2D::xywh(
			(float)m_framebuffer->width() * 0.0f,
			(float)m_framebuffer->height() * 0.02f,
			(float)m_framebuffer->width() *  renderTheEffect * 0.04f,
			(float)m_framebuffer->height() * renderTheEffect
		), rd, redColor
	);

	Draw::rect2D(
		Rect2D::xywh(
			(float)m_framebuffer->width() * 0.0f,
			(float)m_framebuffer->height() * 0.92f,
			(float)m_framebuffer->width() *  renderTheEffect,
			(float)m_framebuffer->height() * renderTheEffect * 0.04f
		), rd, redColor
	);

}

void App::drawClickIndicator(RenderDevice *rd, bool mouseStatus, String mode) {
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
		Color3 cornerColor = (mouseStatus) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
		Draw::rect2D(Rect2D::xywh(boxLeft, boxTop, latencyRect.x, latencyRect.y),  rd, cornerColor);
	}
}

void App::drawHUD(RenderDevice *rd) {
	// Draw the HUD elements
	const Vector2 scale = Vector2(rd->viewport().width()/1920.0f, rd->viewport().height()/1080.0f);

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
					(float)rd->viewport().height() * (float)(sess->weaponCooldownPercent()),
					(float)rd->viewport().width() * sessConfig->clickToPhoton.size.x,
					(float)rd->viewport().height() * (float)(1.0 - sess->weaponCooldownPercent())
				), rd, Color3::white() * 0.8f
			);
		}
		else if (sessConfig->hud.cooldownMode == "ring") {
			// Draw cooldown "ring" instead of box
			const float iRad = sessConfig->hud.cooldownInnerRadius;
			const float oRad = iRad + sessConfig->hud.cooldownThickness;
			const int segments = sessConfig->hud.cooldownSubdivisions;
			int segsToLight = static_cast<int>(ceilf((1 - sess->weaponCooldownPercent())*segments));
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
		const float health = scene()->typedEntity<PlayerEntity>("player")->health();
		const Point2 location = Point2(sessConfig->hud.playerHealthBarPos.x, sessConfig->hud.playerHealthBarPos.y + m_debugMenuHeight);
		const Point2 size = sessConfig->hud.playerHealthBarSize;
		const Point2 border = sessConfig->hud.playerHealthBarBorderSize;
		const Color4 borderColor = sessConfig->hud.playerHealthBarBorderColor;
		const Color4 color = sessConfig->hud.playerHealthBarColors[1] * (1.0f - health) + sessConfig->hud.playerHealthBarColors[0] * health;

		Draw::rect2D(Rect2D::xywh(location - border, size + border + border), rd, borderColor);
		Draw::rect2D(Rect2D::xywh(location, size*Point2(health, 1.0f)), rd, color);
	}
	// Draw the ammo indicator
	if (sessConfig->hud.showAmmo) {
		Point2 lowerRight = Point2(static_cast<float>(rd->viewport().width()), static_cast<float>(rd->viewport().height()));
		hudFont->draw2D(rd,
			format("%d/%d", sess->remainingAmmo(), sessConfig->weapon.maxAmmo),
			lowerRight - sessConfig->hud.ammoPosition,
			sessConfig->hud.ammoSize,
			sessConfig->hud.ammoColor,
			sessConfig->hud.ammoOutlineColor,
			GFont::XALIGN_RIGHT,
			GFont::YALIGN_BOTTOM
		);
	}

	if (sessConfig->hud.showBanner && !emergencyTurbo) {
		const Point2 hudCenter(rd->viewport().width() / 2.0f, sessConfig->hud.bannerVertVisible*hudTexture->height() * scale.y + m_debugMenuHeight);
		Draw::rect2D((hudTexture->rect2DBounds() * scale - hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), hudTexture);

		// Create strings for time remaining, progress in sessions, and score
		float remainingTime = sess->getRemainingTrialTime();
		float printTime = remainingTime > 0 ? remainingTime : 0.0f;
        String time_string;
        if (sess->presentationState == PresentationState::task) {
            time_string = format("%0.2f", printTime);
        }
        else {
            time_string = format("%0.2f", 0.f);
        }
		float prog = sess->getProgress();
		String prog_string = "";
		if (!isnan(prog)) {
            prog_string = format("%d", (int)(100.0f*prog)) + "%";
		}
		String score_string = format("%0.1f", (sess->getScore()));

		hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale.x, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale.x * sessConfig->hud.bannerLargeFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}
}

void App::onPostProcessHDR3DEffects(RenderDevice *rd) {
	GApp::onPostProcessHDR3DEffects(rd);
}

/** Method for handling weapon fire */
shared_ptr<TargetEntity> App::fire(bool destroyImmediately) {
    BEGIN_PROFILER_EVENT("fire");
    
    Ray ray;
    CFrame rayFrame;
	if (m_latencyConfig.hitDetectionInput == LatencyConfig::HitDetectionInput::LATEST) {
        rayFrame = m_noLatencyCamera->frame();
	} else {
		rayFrame = m_displayedCameraFrame;
	}
	ray = rayFrame.lookRay();

	Ray m_lastFireGamestateRay = m_noLatencyCamera->frame().lookRay();
	Ray m_lastFireVisualRay = m_displayedCameraFrame.lookRay();

	bool destroyedTarget = false;
	static bool hitTarget = false;
	bool hitScene = false;
	static RealTime lastTime;
	shared_ptr<TargetEntity> target = nullptr;
    Vector3 hitPoint;

	if (m_hitScan) {
        // build the list of objects in the scene to ignore
        Array<shared_ptr<Entity>> dontHit = { m_explosion, m_lastDecal, m_firstDecal };
        for (auto projectile : projectileArray) {
            dontHit.append(projectile.entity);
        }
        for (auto target : targetArray) {
            dontHit.append(target);
        }

        // initialize hit record and closest ray hit
        float closest = finf();
        int closestIndex = -1;

        if (m_latencyConfig.hitDetectionTime == LatencyConfig::HIT_TIMESTAMP_VISUAL && m_poseLagCount > 0) {
            // Test for hits against old target positions
            State& oldState = m_delayedGameState;
            assert(oldState.targets.size() == oldState.targetFrames.size());

            scene()->intersect(ray, closest, false, dontHit);
            for (size_t i = 0; i < oldState.targetFrames.size(); ++i) {
                // Temporarily set the target frame to the old one before testing for an intersection
                const CFrame backupFrame = oldState.targets[i]->frame();
#if 0
                oldState.targets[i]->setFrame(oldState.targetFrames[i], false);
			    if (oldState.targets[i]->intersect(ray, closest)) {
#else
                // Compute the new ray for this alternative translation and rotation
                Matrix4 delta = oldState.targets[i]->frame().toMatrix4() * oldState.targetFrames[i].toMatrix4().inverse();
                Ray tmpRay((delta * Vector4(ray.origin(), 1.0)).xyz(), (delta * Vector4(ray.direction(), 0.0f)).xyz());
                // Intersect the scene first
			    if (oldState.targets[i]->intersect(tmpRay, closest)) {
#endif
				    // Find the index of the shot target in the target array
                    for (int t = 0; t < targetArray.size(); ++t) {
                        if (oldState.targets[i].get() == targetArray[t].get()) {
				            closestIndex = t;
                            break;
                        }
                    }
			    }
                oldState.targets[i]->setFrame(backupFrame, false);
            }
            if ((closestIndex != -1)&&(closest != finf())) {
                hitScene = true;
            }
        } else {
            // Intersect the scene first
            scene()->intersect(ray, closest, false, dontHit);
            // Find hits with current target positions
		    for (int t = 0; t < targetArray.size(); ++t) {
			    if (targetArray[t]->intersect(ray, closest)) {
				    closestIndex = t;
			    }
		    }
        }

		// Hit logic
		if (closestIndex >= 0) {
			target = targetArray[closestIndex];			// Assign the target pointer here (not null indicates the hit)

			// Damage the target
			float damage;
			if (destroyImmediately || sessConfig->weapon.instantKill) {
				damage = target->health();
			}
			else if (sessConfig->weapon.firePeriod == 0.0f && hitTarget) {		// Check if we are in "laser" mode hit the target last time
				float dt = max(previousSimTimeStep(),0.0f);
				damage = sessConfig->weapon.damagePerSecond * dt;
			}
			else {																// If we're not in "laser" mode then damage/shot is just damage/second * second/shot
				damage = sessConfig->weapon.damagePerSecond * sessConfig->weapon.firePeriod;
			}
			hitTarget = true;
            hitPoint = ray.origin() + ray.direction() * closest;

			// Check if we need to add combat text for this damage
			if (sessConfig->targetView.showCombatText) {
				m_combatTextList.append(FloatingCombatText::create(
					format("%2.0f", 100*damage),
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

			destroyedTarget = target->doDamage(damage);
			bool respawned = false;
			if (destroyedTarget) {
				// create explosion animation
				CFrame explosionFrame = targetArray[closestIndex]->frame();
				explosionFrame.rotation = activeCamera()->frame().rotation;
				static size_t explosionIndex = 0;
				const shared_ptr<VisibleEntity> newExplosion = VisibleEntity::create("explosion" + (explosionIndex++), scene().get(), m_explosionModels[target->scaleIndex()], explosionFrame);
				scene()->insert(newExplosion);
				m_explosion = newExplosion;
				m_explosionEndTime = System::time() + 0.1f; // make explosion end in 0.5 seconds
				sess->countDestroy();
				respawned = target->tryRespawn();
				// check for respawn
				if (!respawned) {
					// This is the final respawn
					destroyTarget(closestIndex);
				}
			}
			if(!destroyedTarget || respawned)  {
				// Handle randomizing position of non-destination targets here
				if (target->destinations().size() == 0 && respawned) {
					sess->randomizePosition(target);
				}
                BEGIN_PROFILER_EVENT("fire/changeColor");
                    BEGIN_PROFILER_EVENT("fire/clone");
				        shared_ptr<ArticulatedModel::Pose> pose = dynamic_pointer_cast<ArticulatedModel::Pose>(targetArray[closestIndex]->pose()->clone());
                    END_PROFILER_EVENT();
                    BEGIN_PROFILER_EVENT("fire/materialSet");
						shared_ptr<UniversalMaterial> mat = m_materials[min((int)(target->health()*m_MatTableSize), m_MatTableSize-1)];
				        pose->materialTable.set("core/icosahedron_default", mat);
                    END_PROFILER_EVENT();
                    BEGIN_PROFILER_EVENT("fire/setPose");
				        targetArray[closestIndex]->setPose(pose);
                    END_PROFILER_EVENT();
                END_PROFILER_EVENT();
			}
		}
		else hitTarget = false;
	}

	// Create the bullet
	if (sessConfig->weapon.renderBullets) {
#if 1
		// Create the bullet start frame from the weapon frame plus muzzle offset
		CFrame bulletStartFrame = m_weaponFrame;
		bulletStartFrame.translation += sessConfig->weapon.muzzleOffset;
#else
        // Bullets start from the camera and match the hit scan exactly
		CFrame bulletStartFrame = CFrame(ray.origin());
#endif
        
		// Angle the bullet start frame towards the aim point
		bulletStartFrame.lookAt(hitTarget ? hitPoint : ray.origin() + ray.direction() * 1000.0f);

		bulletStartFrame.translation += bulletStartFrame.lookVector() * 2.0f;
		const shared_ptr<VisibleEntity>& bullet = VisibleEntity::create(format("bullet%03d", ++m_lastUniqueID), scene().get(), m_bulletModel, bulletStartFrame);
		bullet->setShouldBeSaved(false);
		bullet->setCanCauseCollisions(false);
		bullet->setCastsShadows(false);

		projectileArray.push(Projectile(bullet, System::time() + 1.0f));
		scene()->insert(bullet);
	}

    // play sounds
    if (destroyedTarget) {
		target->playDestroySound();
	}
	else if (hitTarget) {
		target->playHitSound();
	}
	else if (hitScene) {
		m_sceneHitSound->play(sessConfig->audio.sceneHitSoundVol);
	}
	
	if(sessConfig->weapon.firePeriod > 0.0f || !sessConfig->weapon.autoFire) {
		m_fireSound->play(sessConfig->weapon.fireSoundVol);
		//m_fireSound->play(activeCamera()->frame().translation, activeCamera()->frame().lookVector() * 2.0f, 0.5f);
	}

	if (sessConfig->weapon.renderDecals && sessConfig->weapon.firePeriod > 0.0f && !hitTarget) {
		// compute world intersection
		float hitDist = finf();
		Array<shared_ptr<Entity>> dontHit = { m_explosion, m_lastDecal, m_firstDecal };
		for (auto projectile : projectileArray) {
			dontHit.append(projectile.entity);
		}
		for (auto target : targetArray) {
			dontHit.append(target);
		}

		// Cast a ray against the scene to get the decal location/normal
		Model::HitInfo info;
		scene()->intersect(ray, hitDist, false, dontHit, info);
		// Find where to put the decal
		CFrame decalFrame = rayFrame;
		decalFrame.translation += ray.direction() * (hitDist - 0.01f);
		// Set the decal rotation to match the normal here
		decalFrame.lookAt(decalFrame.translation - info.normal);

		// Only allow 1 miss decal at a time (remove last decal if present)
		if (notNull(m_lastDecal)) {
			scene()->remove(m_lastDecal);
		}

		// Add the new decal to the scene
		const shared_ptr<VisibleEntity>& newDecal = VisibleEntity::create(format("decal%03d", ++m_lastUniqueID), scene().get(), m_decalModel, decalFrame);
		newDecal->setCastsShadows(false);
		scene()->insert(newDecal);
		m_lastDecal = m_firstDecal;
		m_firstDecal = newDecal;
	}
    END_PROFILER_EVENT();
	return target;
}

/** Clear all targets one by one */
void App::clearTargets() {
	while (targetArray.size() > 0) {
		destroyTarget(0);
	}
	
	// Remove any explosions when targets are reset.
	// This is not so important, but this avoids confusion
	// in test screenshots from old explosions.
	if (m_explosion) {
		scene()->remove(m_explosion);
		m_explosion.reset();
	}
}

/** Handle user input here */
void App::onUserInput(UserInput* ui) {
	BEGIN_PROFILER_EVENT("onUserInput");
	GApp::onUserInput(ui);
	(void)ui;

	const shared_ptr<PlayerEntity>& player = scene()->typedEntity<PlayerEntity>("player");
	if (!m_userSettingsMode && notNull(player)) {
		player->updateFromInput(ui);
	}
	else {	// Zero the player velocity and rotation when in the setting menu
		player->setDesiredOSVelocity(Vector3::zero());
		player->setDesiredAngularVelocity(0.0, 0.0);
	}

	// Holds the firing state for the frame. This is set here because
	// there may be multiple fire buttons mapped. Initially true and
	// if any fire button is down or was pressed, this is set to false.
	m_buttonUp = true;

	// Handle fire up/down events
	for (GKey shootButton : keyMap.map["shoot"]) {
		// Record whether we clicked or are still holding the button down
		m_buttonUp = m_buttonUp && !ui->keyPressed(shootButton) && !ui->keyDown(shootButton);

		// Handle shoot down (fire) event here
		bool firing = ui->keyPressed(shootButton);
		if (sessConfig->weapon.autoFire)
			firing = firing || ui->keyDown(shootButton);

		if (firing) {
			// check for hit, add graphics, update target state
			if ((sess->presentationState == PresentationState::task) && !m_userSettingsMode) {
				if (sess->canFire()) {
					sess->countClick();						        // Count clicks
					shared_ptr<TargetEntity> t = fire();			// Fire the weapon
					if (notNull(t)) {								// Check if we hit anything
						if (t->health() <= 0) {
							// Target eliminated, must be 'destroy'.
							sess->accumulatePlayerAction(PlayerActionType::Destroy, t->name());
						}
						else {
							// Target 'hit', but still alive.
							sess->accumulatePlayerAction(PlayerActionType::Hit, t->name());
						}
					}
					else {
						// Target still present, must be 'miss'.
						sess->accumulatePlayerAction(PlayerActionType::Miss);
					}
				}
				// Avoid accumulating invalid clicks during holds...
				else {
					// Invalid click since the trial isn't ready for response
					sess->accumulatePlayerAction(PlayerActionType::Invalid);
				}
			}
		}
		else {
			sess->accumulatePlayerAction(PlayerActionType::Nontask); // not happening in task state.
		}

		// Check for developer mode editing here, if so set selected waypoint using the camera
		if (startupConfig.developerMode && startupConfig.waypointEditorMode) {
			waypointManager->aimSelectWaypoint(activeCamera());
		}

	}
	
	for (GKey dummyShoot : keyMap.map["dummyShoot"]) {
		if (ui->keyPressed(dummyShoot) && (sess->presentationState == PresentationState::feedback)) {
			fire(true); // Fire at dummy target here...
		}
	}

	if (m_lastReticleLoaded != userTable.getCurrentUser()->reticleIndex) {
		// Slider was used to change the reticle
		setReticle(userTable.getCurrentUser()->reticleIndex);
	}

	// Hot keys for late warp demo
	if (ui->keyPressed(GKey::KP0)) {
		m_selectedMethod++;
		if (m_selectedMethod == m_warpMethodList.size()) {
			m_selectedMethod = 0;
		}
	}
	if (ui->keyPressed(GKey::KP1)) {
		m_selectedWarpCompensation++;
		if (m_selectedWarpCompensation == m_warpCompensation.size()) {
			m_selectedWarpCompensation = 0;
		}
	}
	if (ui->keyPressed(GKey::KP2)) {
		m_selectedInputTiming++;
		if (m_selectedInputTiming == m_inputTimingList.size()) {
			m_selectedInputTiming = 0;
		}
	}
	if (ui->keyPressed(GKey::KP3)) {
		m_selectedGameStateTiming++;
		if (m_selectedGameStateTiming == m_gameStateTimingList.size()) {
			m_selectedGameStateTiming = 0;
		}
	}
	if (ui->keyPressed(GKey::KP7)) {
		setLatencyFrames(m_simulationLagFrames + 1);
	}
	if (ui->keyPressed(GKey::KP4)) {
		setLatencyFrames(m_simulationLagFrames - 1);
	}

    if (ui->keyPressed(GKey::KP5)) {
        m_latencyVisualizeEnable = !m_latencyVisualizeEnable;
    }

	if (ui->keyPressed(GKey::KP6)) {
		setFramerate(getFrameRate() - 10.0f);
	}

	if (ui->keyPressed(GKey::KP9)) {
		setFramerate(getFrameRate() + 10.0f);
	}

	activeCamera()->filmSettings().setSensitivity(sceneBrightness);
    END_PROFILER_EVENT();
}

void App::destroyTarget(int index) {
	// Not a reference because we're about to manipulate the array
	const shared_ptr<VisibleEntity> target = targetArray[index];
	// Remove the target from the target array
	targetArray.fastRemove(index);
	// Remove the target from the scene
	scene()->removeEntity(target->name());
}

void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
	GApp::onPose(surface, surface2D);

	typedScene<PhysicsScene>()->poseExceptExcluded(surface, "player");

	if (sessConfig->weapon.renderModel || sessConfig->weapon.renderBullets || sessConfig->weapon.renderMuzzleFlash) {
		// Update the weapon frame for all of these cases
		const float yScale = -0.12f;
		const float zScale = -yScale * 0.5f;
		const float lookY = activeCamera()->frame().lookVector().y;
		m_weaponFrame = activeCamera()->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
		// Pose the view model (weapon) for render here
		if (sessConfig->weapon.renderModel) {
			const float prevLookY = activeCamera()->previousFrame().lookVector().y;
			const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
			m_viewModel->pose(surface, m_weaponFrame, activeCamera()->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
		}
	}
    
    if (m_poseLagCount > 0 ) {
        BEGIN_PROFILER_EVENT("Pose enqueue");
        State next;
        next.surfaceArray.append(surface);
        next.surface2DArray.append(surface2D);
        for (auto& target : targetArray) {
            next.targets.append(target);
            next.targetFrames.append(target->frame());
        }
        m_stateQueue.enqueue(next);
        END_PROFILER_EVENT();
        
        if (m_stateQueue.size() < m_poseLagCount) { // if we haven't accumulated m_stateQueue enough, don't draw anything
            Array<shared_ptr<Surface>> s;
            surface = s;
            Array<shared_ptr<Surface2D>> s2;
            surface2D = s2;
        }
        while (m_stateQueue.size() > m_poseLagCount) {
            BEGIN_PROFILER_EVENT("Pose dequeue");
            m_delayedGameState = m_stateQueue.dequeue();
            if (m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_POSES) {
                surface = m_delayedGameState.surfaceArray;
                surface2D = m_delayedGameState.surface2DArray;
            }
            END_PROFILER_EVENT();
        }
    }
}

void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
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

	rd->push2D(); {
		const float scale = rd->viewport().width() / 1920.0f;
		rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

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
		if (waypointManager->recordMotion) {
			Draw::point(Point2(rd->viewport().width()*0.9f - 15.0f, 20.0f+m_debugMenuHeight*scale), rd, Color3::red(), 10.0f);
			outputFont->draw2D(rd, "Recording Position", Point2(rd->viewport().width() - 200.0f , m_debugMenuHeight*scale), 20.0f, Color3::red());
		}

		// Draw target health bars
		if (sessConfig->targetView.showHealthBars) {
			for (auto const& target : targetArray) {
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
				bool remove = !m_combatTextList[i]->draw(rd, *activeCamera(), *m_framebuffer);
				if (remove) m_combatTextList[i] = nullptr;		// Null pointers to remove
			}
			// Remove the expired elements here
			m_combatTextList.removeNulls();
		}

		// Draw the click indicator
		if (sessConfig->clickToPhoton.enabled) {
			bool status = m_buttonUp;
			if (sessConfig->clickToPhoton.mode == "total") {
				status = m_mouseStateDelayed;
			}

			drawClickIndicator(rd, status, sessConfig->clickToPhoton.mode);
		}

		// Draw the HUD here
		if (sessConfig->hud.enable) {
			drawHUD(rd);
		}

		// Reticle
		UserConfig* user = userTable.getCurrentUser();
		float tscale = max(min(((float)(System::time() - sess->lastFireTime()) / user->reticleShrinkTimeS), 1.0f), 0.0f);
		float rScale = tscale * user->reticleScale[0] + (1.0f - tscale)*user->reticleScale[1];
		Color4 rColor = user->reticleColor[1] * (1.0f - tscale) + user->reticleColor[0] * tscale;
		Draw::rect2D(((reticleTexture->rect2DBounds() - reticleTexture->vector2Bounds() / 2.0f))*rScale / 2.0f + rd->viewport().wh() / 2.0f, rd, rColor, reticleTexture);

		// Handle the feedback message
		String message = sess->getFeedbackMessage();
		if (!message.empty()) {
			outputFont->draw2D(rd, message.c_str(),
				(Point2(rd->viewport().width()*0.5f, rd->viewport().height()*0.4f)).floor(), floor(20.0f * scale), Color3::yellow(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		}

	} rd->pop2D();

	if (sessConfig->render.shader != "") {
		// Copy the post-VFX HDR framebuffer
		static shared_ptr<Framebuffer> temp = Framebuffer::create(Texture::createEmpty("temp distortion source", 256, 256, m_framebuffer->texture(0)->format()));
		temp->resize(m_framebuffer->width(), m_framebuffer->height());
		m_framebuffer->blitTo(rd, temp, false, false, false, false, true);

		rd->push2D(m_framebuffer); {
			Args args;
			args.setUniform("sourceTexture", temp->texture(0), Sampler::video());
			args.setRect(rd->viewport());
			LAUNCH_SHADER(sessConfig->render.shader, args);
		} rd->pop2D();
	}

	// Might not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}

/** Set the currently reticle by index */
void App::setReticle(const int r) {
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

void App::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
}

void App::setLatewarpConfig(LatencyConfig config)
{
	// This could break things if set during oneFrame(). Better to double buffer 'config'.
	// Currently it's only used by UnitTests but the caller needs to be aware.
	m_latencyConfig = config;
	
	// The definitive state is currently coming from m_warpMethodList etc and not m_latencyConfig. This need cleaning up. Adding a pending config might help.
	if (m_latencyConfig.latewarpMethod == LatencyConfig::LatewarpMethod::LATEWARP_NONE) {
		m_selectedWarpCompensation = 0;
	}
	else if (m_latencyConfig.latewarpMethod == LatencyConfig::LatewarpMethod::LATEWARP_ROTATION) {
		m_selectedWarpCompensation = 1;
	}
	else if (m_latencyConfig.latewarpMethod == LatencyConfig::LatewarpMethod::LATEWARP_FULLTRANSFORM) {
		m_selectedWarpCompensation = 2;
	}
	if (m_latencyConfig.latencySimulation == LatencyConfig::LatencySimulation::LATENCY_DELAY_FRAMES) {
		m_selectedMethod = 0;
	}
	else if (m_latencyConfig.latencySimulation == LatencyConfig::LatencySimulation::LATENCY_DELAY_POSES) {
		m_selectedMethod = 1;
	}
	if (m_latencyConfig.hitDetectionInput == LatencyConfig::HitDetectionInput::LATEST) {
		m_selectedInputTiming = 0;
	}
	else if (m_latencyConfig.hitDetectionInput == LatencyConfig::HitDetectionInput::VISUAL) {
		m_selectedInputTiming = 1;
	}
	if (m_latencyConfig.hitDetectionTime == LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_CLICK) {
		m_selectedGameStateTiming = 0;
	}
	else if (m_latencyConfig.hitDetectionTime == LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_VISUAL) {
		m_selectedGameStateTiming = 1;
	}
}

void App::configureLateWarp() {
	// enable/disable late warp
	if (m_warpCompensation[m_selectedWarpCompensation] == "None") {
		m_latencyConfig.latewarpMethod = LatencyConfig::LatewarpMethod::LATEWARP_NONE;
	}
	else if (m_warpCompensation[m_selectedWarpCompensation] == "Rotation") {
		m_latencyConfig.latewarpMethod = LatencyConfig::LatewarpMethod::LATEWARP_ROTATION;
	}
	else if (m_warpCompensation[m_selectedWarpCompensation] == "Full Transform") {
		m_latencyConfig.latewarpMethod = LatencyConfig::LatewarpMethod::LATEWARP_FULLTRANSFORM;
	}

	// late warp simulation method
	if (m_warpMethodList[m_selectedMethod] == "Image Operation") {
		m_latencyConfig.latencySimulation = LatencyConfig::LatencySimulation::LATENCY_DELAY_FRAMES;
	}
	else if (m_warpMethodList[m_selectedMethod] == "Virtual Camera Steering") {
		m_latencyConfig.latencySimulation = LatencyConfig::LatencySimulation::LATENCY_DELAY_POSES;
	}

	// input timing
	if (m_inputTimingList[m_selectedInputTiming] == "Latest") {
		m_latencyConfig.hitDetectionInput = LatencyConfig::HitDetectionInput::LATEST;
	}
	else if (m_inputTimingList[m_selectedInputTiming] == "On Visual") {
		m_latencyConfig.hitDetectionInput = LatencyConfig::HitDetectionInput::VISUAL;
	}

	// game state timing
	if (m_gameStateTimingList[m_selectedGameStateTiming] == "On Click") {
		m_latencyConfig.hitDetectionTime = LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_CLICK;
	}
	else if (m_gameStateTimingList[m_selectedGameStateTiming] == "On Visual") {
		m_latencyConfig.hitDetectionTime = LatencyConfig::HitDetectionTime::HIT_TIMESTAMP_VISUAL;
	}
    
	// Set delay queue lengths based on the above settings.
    if (m_latencyConfig.latencySimulation == LatencyConfig::LATENCY_DELAY_FRAMES) {
		if (m_displayLagFrames > m_simulationLagFrames) {
			// We are truncating the delay frames. Rotate the latency queue so that the delay
			// is immediately correct. After the rotate, all the frames that are too old will
			// be at the end of the array and will be skipped.
			int newDelayStart = (m_currentDelayBufferIndex + m_displayLagFrames - m_simulationLagFrames) % m_displayLagFrames;
			std::rotate(m_ldrDelayBufferQueue.begin(), m_ldrDelayBufferQueue.begin() + newDelayStart, m_ldrDelayBufferQueue.begin() + m_displayLagFrames);
			m_currentDelayBufferIndex = std::max(0, m_simulationLagFrames - 1);
		}

        m_displayLagFrames = m_simulationLagFrames;
        m_cameraDelayFrames = 0;

        // In this case we use the pose queue for hit detection.
        m_poseLagCount = m_simulationLagFrames;
    } else {
        m_displayLagFrames = 0;
        m_cameraDelayFrames = m_simulationLagFrames;
        m_poseLagCount = m_simulationLagFrames;
    }
}

/** Overridden (optimized) oneFrame() function to improve latency */
void App::oneFrame() {
	configureLateWarp();

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
		activeCamera()->onPose(m_posed3D);
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

	m_dumpNextFrame = false;
}


// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int AbortReportHook(int reportType, char *message, int *returnValue)
{
	const char* typeStr;
	switch (reportType) {
	case _CRT_WARN: typeStr = "Warning"; break;
	case _CRT_ERROR: typeStr = "Error"; break;
	case _CRT_ASSERT: typeStr = "Assertion"; break;
	default: typeStr = "<invalid report type>"; break;
	}
	printf("Abort (%s): %s\n", typeStr, message);
	fflush(stdout);
	*returnValue = 1;
	return true; // no popup!
}

int main(int argc, const char* argv[]) {
    if (FileSystem::exists("startupconfig.Any")) {
        App::startupConfig = Any::fromFile("startupconfig.Any");
    }
    else {
        // autogenerate if it wasn't there
        App::startupConfig.toAny().save("startupconfig.Any");
    }

	{
		G3DSpecification spec;
        spec.audio = App::startupConfig.audioEnable;
		initGLG3D(spec);
	}

	(void)argc; (void)argv;
	GApp::Settings settings(argc, argv);

	if (App::startupConfig.fullscreen) {
		settings.window.width = 1920;
		settings.window.height = 1080;
	}
	else {
		settings.window.width = (int)App::startupConfig.windowSize.x;
		settings.window.height = (int)App::startupConfig.windowSize.y;
	}
	settings.window.fullScreen = App::startupConfig.fullscreen;
	settings.window.resizable = !settings.window.fullScreen;

    // V-sync off always
	settings.window.asynchronous = true;
	settings.window.caption = "First Person Science";
	settings.window.refreshRate = -1;
	settings.window.defaultIconFilename = "icon.png";

	settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(128, 128);
	settings.hdrFramebuffer.colorGuardBandThickness = settings.hdrFramebuffer.depthGuardBandThickness;
	settings.dataDir = FileSystem::currentDirectory();
	settings.screenCapture.includeAppRevision = false;
	settings.screenCapture.includeG3DRevision = false;
	settings.screenCapture.outputDirectory = ""; // "../journal/"
	settings.screenCapture.filenamePrefix = "_";

	settings.renderer.deferredShading = true;
	settings.renderer.orderIndependentTransparency = false;

	App app(settings);

	return app.run();
}

