/** \file App.cpp */
#include "App.h"
#include "TargetEntity.h"

// Set to false when just editing content
//static const bool playMode = true;

// Enable this to see maximum CPU/GPU rate when not limited
// by the monitor. (true = target infinite frame rate)
static const bool  unlockFramerate = false;
static const bool  useSerialPort = false;

// Set to true if the monitor has G-SYNC/Adaptive VSync/FreeSync, 
// which allows the application to submit asynchronously with vsync
// without tearing.
static const bool  variableRefreshRate = true;

const float App::TARGET_MODEL_ARRAY_SCALING = 0.2f;
const float App::TARGET_MODEL_ARRAY_OFFSET = 40;

//static const float verticalFieldOfViewDegrees = 90; // deg
static const float horizontalFieldOfViewDegrees = 103; // deg

static const bool measureClickPhotonLatency = true;
static const bool testCustomProjection = false;

// JBS: TODO: Refactor these as experiment variables
//========================================================================
// variables related to experimental condition and record.
//static const float targetFrameRate = 360; // hz
//const int numFrameDelay = 0;
//static const std::string expMode = "training"; // training or real
//static const std::string taskType = "reaction"; // reaction or target
//static const std::string appendingDescription = "ver1";
//========================================================================

App::App(const GApp::Settings& settings) : GApp(settings) {
	// TODO: make method that changes definition of ex, and have constructor call that
	// method to set default experiment
	// JBS: moved experiment definition to `onInit()`
}

void App::onInit() {
	// feed a random seed based on time.
	Random::common().reset(uint32(time(0)));

	GApp::onInit();

	scene()->registerEntitySubclass("FlyingEntity", &FlyingEntity::create);

	// load user setting from file
	userTable = UserTable::getUserTable();
	printUserTableToLog(userTable);

	// load experiment setting from file
	experimentConfig = ExperimentConfig::getExperimentConfig();
	printExpConfigToLog(experimentConfig);

	// Get and save system configuration
	SystemConfig sysConfig = SystemConfig::getSystemConfig();
	sysConfig.printSystemInfo();									// Print system info to log.txt
	sysConfig.toAny().save("systemconfig.Any");						// Update the any file here (new system info to write)

	setSubmitToDisplayMode(
		//SubmitToDisplayMode::EXPLICIT);
		SubmitToDisplayMode::MINIMIZE_LATENCY);
		//SubmitToDisplayMode::BALANCE);
	    //SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);

	showRenderingStats = false;
	makeGUI();
	developerWindow->videoRecordDialog->setCaptureGui(true);
	m_outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	m_hudFont = GFont::fromFile(System::findDataFile("dominant.fnt"));
	m_hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));

	if (experimentConfig.playMode) {
		m_fireSound = Sound::create(System::findDataFile("sound/42108__marcuslee__Laser_Wrath_6.wav"));
		m_explosionSound = Sound::create(System::findDataFile("sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav"));
	}

	loadModels();
	setReticle(m_reticleIndex);

	//spawnTarget(Point3(37.6184f, -0.54509f, -2.12245f), 1.0f);
	//spawnTarget(Point3(39.7f, -2.3f, 2.4f), 1.0f);

	updateMouseSensitivity();			// Update (apply) mouse sensitivity
	updateSessionDropDown();			// Update the session drop down to remove already completed sessions
	updateSessionPress();				// Update session to create results file/start collection

	// initialize comport driver
	if (useSerialPort) {
		DWORD errorMsg;
		m_com.Open(2, errorMsg);
		int aa = 1;
	}
}

void App::printUserTableToLog(UserTable table) {
	logPrintf("Current User: %s\n", table.currentUser);
	for (UserConfig user : table.users) {
		logPrintf("\tUser ID: %s, cmp360 = %f, mouseDPI = %d\n", user.id, user.cmp360, user.mouseDPI);
	}
}

void App::printExpConfigToLog(ExperimentConfig config) {
	logPrintf("-------------------\nExperiment Config\n-------------------\nPlay Mode = %s\nTask Type = %s\nappendingDescription = %s\nscene name = %s\nFeedback Duration = %f\nReady Duration = %f\nTask Duration = %f\nMax Clicks = %d\n",
		(config.playMode ? "true" : "false"), config.taskType, config.appendingDescription, config.sceneName, config.feedbackDuration, config.readyDuration, config.taskDuration, config.maxClicks);
	// Iterate through sessions and print them
	for (int i = 0; i < config.sessions.size(); i++) {
		SessionConfig sess = config.sessions[i];
		logPrintf("\t-------------------\n\tSession Config\n\t-------------------\n\tID = %s\n\tFrame Rate = %f\n\tFrame Delay = %d\n\tSelection Order = %s\n",
			sess.id, sess.frameRate, sess.frameDelay, sess.selectionOrder);
		// Now iterate through each run
		for (int j = 0; j < sess.trials.size(); j++) {
			logPrintf("\t\tTrial Run Config: ID = %s, Count = %d\n",
				sess.trials[j].id, sess.trials[j].count);
		}
	}
	// Iterate through trials and print them
	for (int i = 0; i < config.targets.size(); i++) {
		TargetConfig target = config.targets[i];
		logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tElevation Locked = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n",
			target.id, target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.visualSize[0], target.visualSize[1], target.elevLocked ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1]);
	}
}

void App::openUserSettingsWindow() {
    m_userSettingsMode = true;
    m_userSettingsWindow->setVisible(m_userSettingsMode);
}

void App::updateMouseSensitivity() {
    // G3D expects mouse sensitivity in radians
    // we're converting from mouseDPI and centimeters/360 which explains
    // the screen resolution (dots), cm->in factor (2.54) and 2PI
    double mouseSensitivity = 2.0 * pi() * 2.54 * 1920.0 / (userTable.getCurrentUser()->cmp360 * userTable.getCurrentUser()->mouseDPI);
    // additional correction factor based on few samples - TODO: need more careful setup to study this
    mouseSensitivity = mouseSensitivity * 1.0675; // 10.5 / 10.0 * 30.5 / 30.0
    const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
    if (m_userSettingsMode) {
        // set to 3rd person
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
    }
    else {
        // Force into FPS mode
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
        fpm->setMoveRate(0.0);
    }
    fpm->setTurnRate(mouseSensitivity);
}

void App::spawnParameterizedRandomTarget(float motionDuration=4.0f, float motionDecisionPeriod=0.5f, float speed=2.0f, float radius=10.0f, float scale=2.0f) {
    Random& rng = Random::threadCommon();

    // Construct a reference frame
    // Remove the vertical component
    Vector3 Z = -m_debugCamera->frame().lookVector();
    debugPrintf("lookatZ = [%.4f, %.4f, %.4f]\n", Z.x, Z.y, Z.z);
    debugPrintf("origin  = [%.4f, %.4f, %.4f]\n", m_debugCamera->frame().translation.x, m_debugCamera->frame().translation.y, m_debugCamera->frame().translation.z);
    Z.y = 0.0f;
    Z = Z.direction();
    Vector3 Y = Vector3::unitY();
    Vector3 X = Y.cross(Z);

    // Make a random vector in front of the player in a narrow field of view
    Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.5f, 0.5f)).direction();

    // Ray from user/camera toward intended spawn location
    Ray ray = Ray::fromOriginAndDirection(m_debugCamera->frame().translation, dir);

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

void App::spawnRandomTarget() {
    // TODO: temporary shortcut
    spawnParameterizedRandomTarget();
    return;

	Random& rng = Random::threadCommon();

	bool done = false;
	int tries = 0;

	// Construct a reference frame
	// Remove the vertical component
	Vector3 Z = -m_debugCamera->frame().lookVector();
	Z.y = 0.0f;
	Z = Z.direction();
	Vector3 Y = Vector3::unitY();
	Vector3 X = Y.cross(Z);

	do {
		// Make a random vector in front of the player in a narrow field of view
		Vector3 dir = (-Z + X * rng.uniform(-1, 1) + Y * rng.uniform(-0.3f, 0.5f)).direction();

		// Make sure the spawn location is visible
		Ray ray = Ray::fromOriginAndDirection(m_debugCamera->frame().translation, dir);
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


shared_ptr<FlyingEntity> App::spawnTarget(const Point3& position, float scale, bool spinLeft, const Color3& color) {
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_targetModelArray.length() - 1);

	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(format("target%03d", ++m_lastUniqueID), scene().get(), m_targetModelArray[scaleIndex], CFrame());

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

shared_ptr<FlyingEntity> App::spawnFlyingTarget(
	const Point3& position,
	float scale,
	const Color3& color,
	const Vector2& speedRange,
	const Vector2& motionChangePeriodRange,
	Point3 orbitCenter)
{
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_targetModelArray.length() - 1);

	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(
		format("target%03d", ++m_lastUniqueID),
		scene().get(),
		m_targetModelArray[scaleIndex],
		CFrame(),
		speedRange,
		motionChangePeriodRange,
		orbitCenter
	);

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
	Point3 orbitCenter)
{
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_targetModelArray.length() - 1);

	const shared_ptr<JumpingEntity>& target = JumpingEntity::create(
		format("target%03d", ++m_lastUniqueID),
		scene().get(),
		m_targetModelArray[scaleIndex],
		CFrame(),
		speedRange,
		motionChangePeriodRange,
		jumpPeriodRange,
		distanceRange,
		jumpSpeedRange,
		gravityRange,
		orbitCenter,
		m_targetDistance
	);

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

// old target uses ifs/d12.ifs below plus setting color with "mesh" above
//UniversalMaterial::Specification materialSpecification;
//materialSpecification.setLambertian(Texture::Specification(color));
//materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
//materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));
//
//const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
//amPose->materialTable.set("mesh", UniversalMaterial::create(materialSpecification));
//target->setPose(amPose);
//
//
//filename = "ifs/d12.ifs";

void App::loadModels() {
	const static Any modelSpec = PARSE_ANY(ArticulatedModel::Specification{
		filename = "model/sniper/sniper.obj";
		preprocess = {
			transformGeometry(all(), Matrix4::yawDegrees(90));
			transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
		};
		scale = 0.25;
	});

	m_viewModel = ArticulatedModel::create(modelSpec, "viewModel");

	const static Any laserSpec = PARSE_ANY(ArticulatedModel::Specification{
		filename = "ifs/d10.ifs";
		preprocess = {
			transformGeometry(all(), Matrix4::pitchDegrees(90));
			transformGeometry(all(), Matrix4::scale(0.05,0.05,2));
			setMaterial(all(), UniversalMaterial::Specification {
				lambertian = Color3(0);
			emissive = Power3(5,4,0);
			});
		}; });

	m_laserModel = ArticulatedModel::create(laserSpec, "laserModel");

	const static Any decalSpec = PARSE_ANY(ArticulatedModel::Specification{
		filename = "ifs/square.ifs";
		preprocess = {
			transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));
			setMaterial(all(), UniversalMaterial::Specification{
				lambertian = Texture::Specification {
					filename = "bullet-decal-256x256.png";
					encoding = Color3(1, 1, 1);
				};
			});
		}; });

	m_decalModel = ArticulatedModel::create(decalSpec, "decalModel");

	const static Any explosionSpec = PARSE_ANY(ArticulatedModel::Specification{
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
		}; });

	m_explosionModel = ArticulatedModel::create(explosionSpec, "explosionModel");

	for (int i = 0; i <= 20; ++i) {
		const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - TARGET_MODEL_ARRAY_OFFSET);
		m_targetModelArray.push(ArticulatedModel::create(Any::parse(format(STR(ArticulatedModel::Specification{
			filename = "model/target/target.obj";
			cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings {
				allowVertexMerging = true;
				forceComputeNormals = false;
				forceComputeTangents = false;
				forceVertexMerging = true;
				maxEdgeLength = inf;
				maxNormalWeldAngleDegrees = 0;
				maxSmoothAngleDegrees = 0;
			};
			scale = %f;
			};), scale))));
	}
}


void App::makeGUI() {
	debugWindow->setVisible(!experimentConfig.playMode);
	developerWindow->setVisible(!experimentConfig.playMode);
	developerWindow->sceneEditorWindow->setVisible(!experimentConfig.playMode);
	developerWindow->cameraControlWindow->setVisible(!experimentConfig.playMode);
	developerWindow->videoRecordDialog->setEnabled(true);

	const float SLIDER_SPACING = 35;
	debugPane->beginRow(); {
		debugPane->addCheckBox("Hitscan", &m_hitScan);
		debugPane->addCheckBox("Show Laser", &m_renderHitscan);
		debugPane->addCheckBox("Weapon", &m_renderViewModel);
		debugPane->addCheckBox("HUD", &m_renderHud);
		debugPane->addCheckBox("FPS", &m_renderFPS);
		debugPane->addCheckBox("Turbo", &m_emergencyTurbo);
		static int frames = 0;
		GuiControl* c = nullptr;

        debugPane->addButton("Spawn", this, &App::spawnRandomTarget);
		debugPane->setNewChildSize(230.0f, -1.0f, 70.0f);

		c = debugPane->addNumberBox("Framerate", Pointer<float>(
			[&]() { return 1.0f / float(realTimeTargetDuration()); },
			[&](float f) {
			// convert to seconds from fps
			f = 1.0f / f;
			const float current = (float)realTimeTargetDuration();
			if (abs(f - current) > 1e-5f) {
				// Only set when there is a change, otherwise the simulation's deltas are confused.
				setFrameDuration(f, GApp::REAL_TIME);
			}}), "Hz", GuiTheme::LOG_SLIDER, 30.0f, 5000.0f); c->moveBy(SLIDER_SPACING, 0);

			c = debugPane->addNumberBox("Input Lag", &frames, "f", GuiTheme::LINEAR_SLIDER, 0, 60); c->setEnabled(false); c->moveBy(SLIDER_SPACING, 0);
			c = debugPane->addNumberBox("Display Lag", &m_displayLagFrames, "f", GuiTheme::LINEAR_SLIDER, 0, 60); c->moveBy(SLIDER_SPACING, 0);
			debugPane->addNumberBox("Reticle", &m_reticleIndex, "", GuiTheme::LINEAR_SLIDER, 0, numReticles - 1, 1)->moveBy(SLIDER_SPACING, 0);
			debugPane->addNumberBox("Brightness", &m_sceneBrightness, "x", GuiTheme::LOG_SLIDER, 0.01f, 2.0f)->moveBy(SLIDER_SPACING, 0);
	} debugPane->endRow();


    // set up user settings window
    m_userSettingsWindow = GuiWindow::create("User Settings", nullptr, 
        Rect2D::xywh((float)window()->width() * 0.5f - 200.0f, (float)window()->height() * 0.5f - 100.0f, 400.0f, 200.0f));
    addWidget(m_userSettingsWindow);
    GuiPane* p = m_userSettingsWindow->pane();
    m_currentUserPane = p->addPane("Current User Settings");
    updateUserGUI();

    m_ddCurrentUser = userTable.getCurrentUserIndex();
    p = p->addPane("Experiment Settings");
    p->beginRow();
        m_userDropDown = p->addDropDownList("User", userTable.getIds(), &m_ddCurrentUser);
	    p->addButton("Select User", this, &App::updateUser);
    p->endRow();
    p->beginRow();
        m_sessDropDown = p->addDropDownList("Session", Array<String>({}), &m_ddCurrentSession);
        updateSessionDropDown();
	    p->addButton("Select Session", this, &App::updateSessionPress);
    p->endRow();
    p->addButton("Quit", this, &App::quitRequest);
    m_userSettingsWindow->setVisible(m_userSettingsMode);

	debugWindow->pack();
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}

void App::userSaveButtonPress(void) {
	// Save the any file
	Any a = Any(userTable);
	a.save("userconfig.Any");
	logPrintf("User table saved.");			// Print message to log
}	

void App::updateUser(void){
	// Update the user if needed
	if (m_lastSeenUser != m_ddCurrentUser) {
		if(m_sessDropDown->numElements() > 0) updateSession(updateSessionDropDown()[0]);
		String id = getDropDownUserId();
		String filename = "../results/" + experimentConfig.taskType + "_" + id + "_" + String(Logger::genFileTimestamp()) + ".db";
		logger->createResultsFile(filename, id);
		m_lastSeenUser = m_ddCurrentUser;

        userTable.currentUser = id;
        updateUserGUI();
	}
	// Get new session list for (new) user
	updateSessionDropDown();
}

void App::updateUserGUI() {
    m_currentUserPane->removeAllChildren();
    m_currentUserPane->addLabel(format("Current User: %s", userTable.currentUser));
    m_mouseDPILabel = m_currentUserPane->addLabel(format("Mouse DPI: %f", userTable.getCurrentUser()->mouseDPI));
    m_currentUserPane->addNumberBox("Mouse 360", &(userTable.getCurrentUser()->cmp360), "cm", GuiTheme::LINEAR_SLIDER, 0.2, 100.0, 0.2);
    m_currentUserPane->addButton("Save cm/360", this, &App::userSaveButtonPress);
}

Array<String> App::updateSessionDropDown(void) {
	// Create updated session list
    Array<String> remainingSess = {};
    UserConfig* currentUser = userTable.getCurrentUser();
    for (const SessionConfig& sess : experimentConfig.sessions) {
        // user hasn't completed this session
        if (!currentUser->completedSessions.contains(sess.id)) {
            remainingSess.append(sess.id);
        }
    }
	m_sessDropDown->setList(remainingSess);

	// Print message to log
	logPrintf("Updated session drop down to:\n");
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

void App::markSessComplete(String id) {
	userTable.getCurrentUser()->addCompletedSession(id);
}

shared_ptr<UserConfig> App::getCurrUser(void) {
    //return m_userTable.getIds()[m_ddCurrentUser];
	return userTable.getUserById(getDropDownUserId());
}

void App::updateSessionPress(void) {
	updateSession(getDropDownSessId());
}

String GetLastErrorString() {
	DWORD error = GetLastError();
	if (error){
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		if (bufLen){
			LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
			std::string result(lpMsgStr, lpMsgStr + bufLen);
			LocalFree(lpMsgBuf);
			return String(result);
		}
	}
	return String();
}

void App::updateSession(String id) {
	if (!id.empty()) {
		// Get the new session config
		shared_ptr<SessionConfig> sessConfig = experimentConfig.getSessionConfigById(id);
		// Print message to log
		logPrintf("User selected session: %s. Updating now...\n", id);
		// apply frame lag
		setDisplayLatencyFrames(sessConfig->frameDelay);

		// Set a maximum *finite* frame rate
		float dt = 0;
		if (unlockFramerate) dt = 1.0f / 8192.0f;					
		else if (variableRefreshRate) dt = 1.0f / sessConfig->frameRate;
		else dt = 1.0f / float(window()->settings().refreshRate);
		setFrameDuration(dt, GApp::REAL_TIME);

		// Update session drop-down selection
		m_sessDropDown->setSelectedValue(id);
	}

	// Initialize the experiment (session) and logger
	if (experimentConfig.taskType == "reaction") {
		ex = ReactionExperiment::create(this);
		logger = ReactionLogger::create();
	}
	else if (experimentConfig.taskType == "target") {
		ex = TargetExperiment::create(this);
		logger = TargetLogger::create();
		// Load the experiment scene if we haven't already (target only)
		if (!m_sceneLoaded) {
			loadScene(experimentConfig.sceneName);
			m_sceneLoaded = true;
		}
	}

	// Check for need to start latency logging and if so run the logger now
	SystemConfig sysConfig = SystemConfig::getSystemConfig();
	if (sysConfig.hasLogger) {
		// Handle running logger if we need to (terminate then merge results)
		if (m_loggerRunning) {
			killPythonLogger();
			if (!pythonMergeLogs(m_logName)) {
				logPrintf("Error merging logs for file: %s", m_logName + ".db");
			}
		}
		// Run a new logger if we need to
		m_logName = "../results/" + experimentConfig.taskType + "_" + id + "_" + userTable.currentUser + "_" + String(Logger::genFileTimestamp());
		runPythonLogger(m_logName, sysConfig.loggerComPort, sysConfig.hasSync, sysConfig.syncComPort);
	}

	// Don't create a results file for a user w/ no sessions left
	if (m_sessDropDown->numElements() == 0) logPrintf("No sessions remaining for selected user.");
	// Create the results file here (but how do we make sure user set up name?)
	else logger->createResultsFile(m_logName + ".db", userTable.currentUser);

	// TODO: Remove the following by invoking a call back.
	ex->onInit();
}

void App::runPythonLogger(String logName, String com, bool hasSync, String syncComPort = "") {
	// Variables for creating process/getting handle
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Come up w/ command string
	String cmd = "pythonw.exe ../scripts/\"event logger\"/software/event_logger.py " + com + " " + logName;
	if (hasSync) cmd += " " + syncComPort;

	LPSTR command = LPSTR(cmd.c_str());
	if (!CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		logPrintf("Failed to start logger: %s", GetLastErrorString());
	}
	// Update logger management variables
	m_loggerRunning = true;
	m_loggerHandle = pi.hProcess;
}

void App::killPythonLogger() {
	if (m_loggerRunning) TerminateProcess(m_loggerHandle, 0);
}

void App::quitRequest() {
    setExitCode(0);
    killPythonLogger();
}

bool App::pythonMergeLogs(String basename) {
	String dbFile = basename + ".db";
	String eventFile = basename + "_event.csv";

	// If we can't find either the db output file or the csv input return false
	if (!FileSystem::exists(dbFile) || !FileSystem::exists(eventFile)) return false;

	// Variables for creating process/getting handle
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	String cmd = "pythonw.exe ../scripts/\"event logger\"/software/event_log_insert.py " + eventFile + " " + dbFile;	
	LPSTR command = LPSTR(cmd.c_str());
	if (!CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		logPrintf("Failed to merge results: %s", GetLastErrorString());
	}
	return true;
}

void App::setDisplayLatencyFrames(int f) {
	m_displayLagFrames = f;
}

void App::onAfterLoadScene(const Any& any, const String& sceneName) {
	m_debugCamera->setFieldOfView(horizontalFieldOfViewDegrees * units::degrees(), FOVDirection::HORIZONTAL);
	setSceneBrightness(m_sceneBrightness);
	setActiveCamera(m_debugCamera);
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

    if (m_displayLagFrames > 0) {
		// Need one more frame in the queue than we have frames of delay, to hold the current frame
		if (m_ldrDelayBufferQueue.size() <= m_displayLagFrames) {
			// Allocate new textures
			for (int i = m_displayLagFrames - m_ldrDelayBufferQueue.size(); i >= 0; --i) {
				m_ldrDelayBufferQueue.push(Framebuffer::create(Texture::createEmpty(format("Delay buffer %d", m_ldrDelayBufferQueue.size()), rd->width(), rd->height(), ImageFormat::RGB8())));
			}
			debugAssert(m_ldrDelayBufferQueue.size() == m_displayLagFrames + 1);
		}

		// When the display lag changes, we must be sure to be within range
		m_currentDelayBufferIndex = min(m_displayLagFrames, m_currentDelayBufferIndex);

		rd->pushState(m_ldrDelayBufferQueue[m_currentDelayBufferIndex]);
	}

	scene()->lightingEnvironment().ambientOcclusionSettings.enabled = ! m_emergencyTurbo;
	m_debugCamera->filmSettings().setAntialiasingEnabled(! m_emergencyTurbo);
	m_debugCamera->filmSettings().setBloomStrength(m_emergencyTurbo ? 0.0f : 0.5f);

	GApp::onGraphics3D(rd, surface);

	if (m_displayLagFrames > 0) {
		// Display the delayed frame
		rd->popState();
		rd->push2D(); {
			// Advance the pointer to the next, which is also the oldest frame
			m_currentDelayBufferIndex = (m_currentDelayBufferIndex + 1) % (m_displayLagFrames + 1);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrDelayBufferQueue[m_currentDelayBufferIndex]->texture(0), Sampler::buffer());
		} rd->pop2D();
	}
}

Point2 App::getViewDirection()
{   // returns (azimuth, elevation), where azimuth is 0 deg when straightahead and + for right, - for left.
	Point3 view_cartesian = m_debugCamera->frame().lookVector();
	float az = atan2(- view_cartesian.z, - view_cartesian.x) * 180 / pif();
	float el = atan2(view_cartesian.y, sqrtf(view_cartesian.x * view_cartesian.x + view_cartesian.z * view_cartesian.z)) * 180 / pif();
	return Point2(az, el);
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {

	// TODO (or NOTTODO): The following can be cleared at the cost of one more level of inheritance.
	ex->onSimulation(rdt, sdt, idt);

	GApp::onSimulation(rdt, sdt, idt);

    // make sure mouse sensitivity is set right
    if (m_userSettingsMode) {
        updateMouseSensitivity();
    }

	const RealTime now = System::time();
	for (int p = 0; p < projectileArray.size(); ++p) {
		const Projectile& projectile = projectileArray[p];

		if (!m_hitScan) {
			// Check for collisions
		}

		if (projectile.endTime < now) {
			// Expire
			projectileArray.fastRemove(p);
			--p;
		}
		else {
			// Animate
			projectile.entity->setFrame(projectile.entity->frame() + projectile.entity->frame().lookVector() * 0.6f);
		}
	}

	// explosion animation
	if (notNull(m_explosion) && m_explosionEndTime < now) {
		scene()->remove(m_explosion);
		m_explosion = nullptr;
	}
	else {
		// could update animation here...
	}

	// Example GUI dynamic layout code.  Resize the debugWindow to fill
	// the screen horizontally.
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

	// Check for completed session
	if (ex->moveOn) {
		String nextSess = updateSessionDropDown()[0];
		updateSession(nextSess);
	}
}

bool App::onEvent(const GEvent& event) {
	// Handle super-class events
	if (GApp::onEvent(event)) { return true; }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::KP_MINUS)) {
        quitRequest();
        return true;
    }

	// If you need to track individual UI events, manage them here.
	// Return true if you want to prevent other parts of the system
	// from observing this specific event.
	//
	// For example,
	// if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
	// if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }
	// if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) { ... return true; }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::ESCAPE || event.key.keysym.sym == GKey::TAB)) {
        m_userSettingsMode = !m_userSettingsMode;
        m_userSettingsWindow->setVisible(m_userSettingsMode);
        if (m_userSettingsMode) {
            // set focus so buttons properly highlight
            m_widgetManager->setFocusedWidget(m_userSettingsWindow);
        }
        // switch to first or 3rd person mode
        updateMouseSensitivity();
        return true;
    }
	return false;
}


void App::onPostProcessHDR3DEffects(RenderDevice *rd) {
	GApp::onPostProcessHDR3DEffects(rd);

	rd->push2D(); {
		// TODO: Is this the right place to call it?
		ex->onGraphics2D(rd);

		// Paint both sides by the width of latency measuring box.
		Color3 blackColor = Color3::black();
		Point2 latencyRect = Point2(0.09f, 0.1f);
		Draw::rect2D(
			Rect2D::xywh(
				(float)m_framebuffer->width() * 0.0f,
				(float)m_framebuffer->height() * 0.0f,
				(float)m_framebuffer->width() * latencyRect.x,
				(float)m_framebuffer->height()
			), rd, blackColor
		);
		Draw::rect2D(
			Rect2D::xywh(
				(float)m_framebuffer->width() * (1.0f - latencyRect.x),
				(float)m_framebuffer->height() * 0.0f,
				(float)m_framebuffer->width() * latencyRect.x,
				(float)m_framebuffer->height()
			), rd, blackColor
		);

		// Click to photon latency measuring corner box
		if (measureClickPhotonLatency) {
			Color3 cornerColor = (m_buttonUp) ? Color3::white() * 0.2f : Color3::white() * 0.8f;
			//Draw::rect2D(rd->viewport().wh() / 10.0f, rd, cornerColor);
			//Draw::rect2D(Rect2D::xywh((float)window()->width() * 0.925f, (float)window()->height() * 0.0f, (float)window()->width() * 0.075f, (float)window()->height() * 0.15f), rd, cornerColor);
			Draw::rect2D(
				Rect2D::xywh(
					(float)m_framebuffer->width() * (1.0f - latencyRect.x),
					(float)m_framebuffer->height() * (0.5f - latencyRect.y / 2),
					(float)m_framebuffer->width() * latencyRect.x,
					(float)m_framebuffer->height() * latencyRect.y
				), rd, cornerColor
			);
			if (useSerialPort) {
				if (m_buttonUp) {
					m_com.SetRts();
					int aa = 1;
					//m_com.SetDtr();
				}
				else {
					m_com.ClearRts();
					int aa = 1;
					//m_com.ClearDtr();
				}
			}
		}
	} rd->pop2D();

	if (testCustomProjection) {
		// This code could be run more efficiently at LDR after Film::exposeAndRender or even during the
		// latency queue copy
			
		// Copy the post-VFX HDR framebuffer
		static shared_ptr<Framebuffer> temp = Framebuffer::create(Texture::createEmpty("temp distortion source", 256, 256, m_framebuffer->texture(0)->format()));
		temp->resize(m_framebuffer->width(), m_framebuffer->height());
		m_framebuffer->blitTo(rd, temp, false, false, false, false, true);

		rd->push2D(m_framebuffer); {
			Args args;
			args.setUniform("sourceTexture", temp->texture(0), Sampler::video());
			args.setRect(rd->viewport());
			LAUNCH_SHADER("shader/distort.pix", args);
		} rd->pop2D();
	}
}


void App::fire() {
	Point3 aimPoint = m_debugCamera->frame().translation + m_debugCamera->frame().lookVector() * 1000.0f;
	bool hitTarget = false;

	if (m_hitScan) {
		const Ray& ray = m_debugCamera->frame().lookRay();

		float closest = finf();
		int closestIndex = -1;
		for (int t = 0; t < targetArray.size(); ++t) {
			if (targetArray[t]->intersect(ray, closest)) {
				closestIndex = t;
			}
		}

		if (closestIndex >= 0) {
			// create explosion animation
			CFrame explosionFrame = targetArray[closestIndex]->frame();
			explosionFrame.rotation = m_debugCamera->frame().rotation;
			const shared_ptr<VisibleEntity>& newExplosion = VisibleEntity::create("explosion", scene().get(), m_explosionModel, explosionFrame);
			scene()->insert(newExplosion);
			m_explosion = newExplosion;
			m_explosionEndTime = System::time() + 0.1f; // make explosion end in 0.5 seconds

			// destroy target
			destroyTarget(closestIndex);
			aimPoint = ray.origin() + ray.direction() * closest;
			m_targetHealth -= 1; // TODO: health point should be tracked by Target Entity class (not existing yet).
			hitTarget = true;
		}
	}

	// Create the laser
	if (m_renderHitscan) {
		CFrame laserStartFrame = m_weaponFrame;
		laserStartFrame.translation += laserStartFrame.upVector() * 0.1f;

		// Adjust for the discrepancy between where the gun is and where the player is looking
		laserStartFrame.lookAt(aimPoint);

		laserStartFrame.translation += laserStartFrame.lookVector() * 2.0f;
		const shared_ptr<VisibleEntity>& laser = VisibleEntity::create(format("laser%03d", ++m_lastUniqueID), scene().get(), m_laserModel, laserStartFrame);
		laser->setShouldBeSaved(false);
		laser->setCanCauseCollisions(false);
		laser->setCastsShadows(false);
		/*
		const shared_ptr<Entity::Track>& track = Entity::Track::create(laser.get(), scene().get(),
			Any::parse(format("%s", laserStartFrame.toXYZYPRDegreesString().c_str())));
		laser->setTrack(track);
		*/
		projectileArray.push(Projectile(laser, System::time() + 1.0f));
		scene()->insert(laser);
	}

	if (experimentConfig.playMode) {
		if (hitTarget) {
			m_explosionSound->play(10.0f);
			//m_explosionSound->play(target->frame().translation, Vector3::zero(), 50.0f);
		}
		else {
			m_fireSound->play(0.5f);
			//m_fireSound->play(m_debugCamera->frame().translation, m_debugCamera->frame().lookVector() * 2.0f, 0.5f);
		}
	}

	if (experimentConfig.renderDecals && !hitTarget) {
		// compute world intersection
		const Ray& ray = m_debugCamera->frame().lookRay();
		Model::HitInfo info;
		float closest = finf();
		scene()->intersect(ray, closest, false, {m_explosion, m_lastDecal, m_firstDecal}, info);

		// Find where to put the decal
		CFrame decalFrame = m_debugCamera->frame();
		decalFrame.translation += ray.direction() * (closest - 0.01f);
		// TODO: Make it rotate to the surface normal. info.normal appears to be at inf...
		//decalFrame.rotation = info.entity->frame().rotation;
		//decalFrame.rotation = info.normal;

		// remove last decal if at max size
		if (notNull(m_lastDecal)) {
			scene()->remove(m_lastDecal);
		}

		// add decal to scene
		const shared_ptr<VisibleEntity>& newDecal = VisibleEntity::create(format("decal%03d", ++m_lastUniqueID), scene().get(), m_decalModel, decalFrame);
		scene()->insert(newDecal);
		m_lastDecal = m_firstDecal;
		m_firstDecal = newDecal;
	}
}

void App::clearTargets() {
	while (targetArray.size() > 0) {
		destroyTarget(0);
	}
}

void App::onUserInput(UserInput* ui) {
	GApp::onUserInput(ui);
	(void)ui;

	if (ui->keyPressed(GKey::LEFT_MOUSE)) {
		// check for hit, add graphics, update target state
		if (ex->responseReady()) {
			// count clicks
			ex->countClick();
			fire();
			if (m_targetHealth == 0) {
				// target eliminated, must be 'hit'.
				if (m_presentationState == PresentationState::task)
				{
					ex->accumulatePlayerAction("hit");
				}
			}
			else {
				// target still present, must be 'miss'.
				if (m_presentationState == PresentationState::task)
				{
					ex->accumulatePlayerAction("miss");
				}
			}
		}
		else {
			// target still present, must be 'miss'.
			if (m_presentationState == PresentationState::task)
			{
				ex->accumulatePlayerAction("invalid");
			}
		}
	}

	// used for click-to-photon box
	if (ui->keyPressed(GKey::LEFT_MOUSE)) {
		m_buttonUp = false;
	}
	else if (ui->keyReleased(GKey::LEFT_MOUSE)) {
		m_buttonUp = true;
	}

	if (m_lastReticleLoaded != m_reticleIndex) {
		// Slider was used to change the reticle
		setReticle(m_reticleIndex);
	}

	m_debugCamera->filmSettings().setSensitivity(m_sceneBrightness);

}

void App::destroyTarget(int index) {
	// Not a reference because we're about to manipulate the array
	const shared_ptr<VisibleEntity> target = targetArray[index];
	targetArray.fastRemove(index);

	scene()->removeEntity(target->name());
}

void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
	GApp::onPose(surface, surface2D);

	if (m_renderViewModel) {
		const float yScale = -0.12f;
		const float zScale = -yScale * 0.5f;
		const float lookY = m_debugCamera->frame().lookVector().y;
		const float prevLookY = m_debugCamera->previousFrame().lookVector().y;
		m_weaponFrame = m_debugCamera->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
		const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
		m_viewModel->pose(surface, m_weaponFrame, m_debugCamera->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
	}
}

void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
 
 //   // Faster than the full stats widget
	//std::string expDebugStr = "%d fps ";
	//expDebugStr += ex->getDebugStr(); // debugging message
 //   debugFont->draw2D(rd, format(expDebugStr.c_str(), iRound(renderDevice->stats().smoothFrameRate)), Point2(10,10), 12.0f, Color3::yellow());

    //// Display DONE when complete
    //if (ex->isExperimentDone()) {
    //    static const shared_ptr<Texture> doneTexture = Texture::fromFile("done.png");
    //    rd->push2D(); {
    //        const float scale = rd->viewport().width() / 3840.0f;
    //        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    //        Draw::rect2D(doneTexture->rect2DBounds() * scale + (rd->viewport().wh() - doneTexture->vector2Bounds() * scale) / 2.0f, rd, Color3::white(), doneTexture);
    //    } rd->pop2D();
    //}
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

		// FPS display (faster than the full stats widget)
		if (m_renderFPS) {
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

			m_outputFont->draw2D(rd, msg, (Point2(30, 28) * scale).floor(), floor(20.0f * scale), Color3::yellow());
		}

	} rd->pop2D();

	//MIght not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}

void App::setReticle(int r) {
	m_lastReticleLoaded = m_reticleIndex = clamp(0, r, numReticles - 1);
	m_reticleTexture = Texture::fromFile(System::findDataFile("gui/reticle.png"));
}

void App::setSceneBrightness(float b) {
	m_sceneBrightness = b;
}


void App::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
	if (useSerialPort) {
		DWORD errorMsg;
		m_com.Close(errorMsg);
	}
}

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	
	// load experiment setting from file
	if (!FileSystem::exists("experimentconfig.Any")) { // if file not found, copy from the sample config file.
		FileSystem::copyFile(System::findDataFile("SAMPLEexperimentconfig.Any"), "experimentconfig.Any");
	}

	ExperimentConfig m_expConfig = Any::fromFile(System::findDataFile("experimentconfig.Any"));
	
	{
		G3DSpecification spec;
		spec.audio = m_expConfig.playMode;
		initGLG3D(spec);
	}

	(void)argc; (void)argv;
	GApp::Settings settings(argc, argv);

	if (m_expConfig.playMode) {
		settings.window.width = 1920; settings.window.height = 1080;
	}
	else {
		settings.window.width = 1920; settings.window.height = 980;
	}
	settings.window.fullScreen = m_expConfig.playMode;
	settings.window.resizable = !settings.window.fullScreen;

    // V-sync off always
	settings.window.asynchronous = true;
	settings.window.caption = "NVIDIA Abstract FPS";
	settings.window.refreshRate = -1;
	settings.window.defaultIconFilename = "icon.png";

	settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
	settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
	settings.dataDir = FileSystem::currentDirectory();
	settings.screenCapture.includeAppRevision = false;
	settings.screenCapture.includeG3DRevision = false;
	settings.screenCapture.outputDirectory = ""; // "../journal/"
	settings.screenCapture.filenamePrefix = "_";

	settings.renderer.deferredShading = true;
	settings.renderer.orderIndependentTransparency = false;

	return App(settings).run();
}

void App::resetView() {
	// reset view direction (look front!)
    const shared_ptr<Camera>& camera = activeCamera();
	//activeCamera()->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
    // Account for the camera translation to ensure correct look vector
	//camera->lookAt(camera->frame().translation + Point3(0, 0, -1));
	const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
	//fpm->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
	fpm->lookAt(Point3(0,0,-1));
}

