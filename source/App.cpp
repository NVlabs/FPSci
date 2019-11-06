/** \file App.cpp */
#include "App.h"
#include "TargetEntity.h"
#include "PlayerEntity.h"

// Scale and offset for target
const float App::TARGET_MODEL_ARRAY_SCALING = 0.2f;
const float App::TARGET_MODEL_ARRAY_OFFSET = 20;

// Storage for configuration static vars
FpsConfig SessionConfig::defaultConfig;
int TrialCount::defaultCount;
Array<String> UserSessionStatus::defaultSessionOrder;

/** global startup config - sets playMode and experiment/user paths */
StartupConfig startupConfig;

App::App(const GApp::Settings& settings) : GApp(settings) {}

/** Initialize the app */
void App::onInit() {
	// Seed random based on the time
	Random::common().reset(uint32(time(0)));

	// Initialize the app
	GApp::onInit();

	// Load per user settings from file
	userTable = UserTable::load(startupConfig.userConfig());
	userTable.printToLog();

	// Load per experiment user settings from file
	userStatusTable = UserStatusTable::load();
	userStatusTable.printToLog();

	// Load experiment setting from file
	experimentConfig = ExperimentConfig::load(startupConfig.experimentConfig());
	experimentConfig.printToLog();

	// Get and save system configuration
	SystemConfig sysConfig = SystemConfig::load();
	sysConfig.printToLog();											// Print system info to log.txt
	sysConfig.toAny().save("systemconfig.Any");						// Update the any file here (new system info to write)

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
	developerWindow->videoRecordDialog->setCaptureGui(true);
	   
	// Load fonts and images
	outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));

	// Load models and set the reticle
	loadModels();
	setReticle(reticleIndex);

	updateMouseSensitivity();			// Update (apply) mouse sensitivity
	updateMoveRate(experimentConfig.player.moveRate);
	updateSessionDropDown();			// Update the session drop down to remove already completed sessions
	updateSessionPress();				// Update session to create results file/start collection
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
    }
	// Control player motion using the experiment config parameter
	//fpm->setMoveRate(sessConfig->moveRate);
    fpm->setTurnRate(mouseSensitivity);
}

void App::updateMoveRate(float rate) {
	const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
	fpm->setMoveRate(rate);
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
	bool axisLock[3],
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
	bool axisLock[3],
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

void App::loadModels() {
	if (experimentConfig.weapon.renderModel) {
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
		scale = 0.25;
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

void App::makeGUI() {
	debugWindow->setVisible(!startupConfig.playMode);
	developerWindow->setVisible(!startupConfig.playMode);
	developerWindow->sceneEditorWindow->setVisible(!startupConfig.playMode);
	developerWindow->cameraControlWindow->setVisible(!startupConfig.playMode);
	developerWindow->videoRecordDialog->setEnabled(true);

	theme = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"));

	// Setup the waypoint config/display
	WaypointDisplayConfig config = WaypointDisplayConfig();
	m_waypointControls = WaypointDisplay::create(this, theme, config, (shared_ptr<Array<Destination>>)&m_waypoints);
	m_waypointControls->setVisible(false);
	this->addWidget(m_waypointControls);

	// Setup the player control
	m_playerControls = PlayerControls::create((FpsConfig)experimentConfig, std::bind(&App::exportScene, this), theme);
	m_playerControls->setVisible(false);
	this->addWidget(m_playerControls);

	// Setup the render control
	m_renderControls = RenderControls::create((FpsConfig)experimentConfig, renderFPS, emergencyTurbo, reticleIndex, numReticles, sceneBrightness, theme);
	m_renderControls->setVisible(false);
	this->addWidget(m_renderControls);

	m_weaponControls = WeaponControls::create(sessConfig->weapon, theme);
	m_weaponControls->setVisible(false);
	this->addWidget(m_weaponControls);

	// Open sub-window panes here...
	debugPane->beginRow(); {
		debugPane->addButton("Render Controls [1]", this, &App::showRenderControls);
		debugPane->addButton("Player Controls [2]", this, &App::showPlayerControls);
		debugPane->addButton("Waypoint Manager [3]", this, &App::showWaypointManager);
		debugPane->addButton("Weapon Controls [4]", this, &App::showWeaponControls);
	}debugPane->endRow();

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
	m_debugMenuHeight = startupConfig.playMode ? 0.0f : debugWindow->rect().height();
}

void App::dropWaypoint(void) {
	// Create the destination
	Point3 xyz = activeCamera()->frame().translation;
	SimTime time = m_waypoints.size() == 0 ? 0.0f : m_waypoints.last().time + waypointDelay;
	Destination dest = Destination(xyz, time);
	dropWaypoint(dest, Point3(0, -waypointVertOffset, 0));
}

void App::dropWaypoint(Destination dest, Point3 offset) {
	// Apply the offset
	dest.position += offset;

	// If this isn't the first point, connect it to the last one with a line
	if (m_waypoints.size() > 0) {
		shared_ptr<CylinderShape> shape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints.last().position,
			dest.position, 
			m_waypointConnectRad)));
		DebugID arrowID = debugDraw(shape, finf(), m_waypointColor, Color4::clear());
		m_arrowIDs.append(arrowID);
	}

	// Draw the waypoint (as a sphere)
	DebugID pointID = debugDraw(Sphere(dest.position, m_waypointRad), finf(), m_waypointColor, Color4::clear());

	// Update the arrays and time tracking
	m_waypoints.append(dest);
	m_waypointIDs.append(pointID);

	// Print to the log
	logPrintf("Dropped waypoint... Time: %f, XYZ:[%f,%f,%f]\n", dest.time, dest.position[0], dest.position[1], dest.position[2]);
}

bool App::updateWaypoint(Destination dest, int idx) {
	// Check for valid indexing
	if (idx < 0) {
		idx = m_waypointControls->getSelected();	// Use the selected item if no index is passed in
	}
	else if (idx > m_waypoints.lastIndex()) 
		return false;							// If the index is too high don't update

	// Handle drawing highlight
	if(m_highlighted != m_waypointIDs[idx]){
		removeDebugShape(m_highlighted);
		m_highlighted = debugDraw(Sphere(dest.position, m_waypointRad*1.1f), finf(), Color4::clear(), m_highlightColor);
	}

	// Skip points w/ no change
	if (dest.position == m_waypoints[idx].position) {
		return true;
	}

	// Array management
	m_waypoints[idx] = dest;

	// Remove the waypoint debugDraw, then replace it w/ a new one
	DebugID pointID = m_waypointIDs[idx];
	removeDebugShape(pointID);
	pointID = debugDraw(Sphere(dest.position, m_waypointRad), finf(), m_waypointColor, Color4::clear());
	m_waypointIDs[idx] = pointID;

	// Update the arrows around this point
	if (idx > 0 && idx < m_waypoints.lastIndex()) {
		shared_ptr<CylinderShape> toShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints[idx - 1].position,
			dest.position,
			m_waypointConnectRad)));
		shared_ptr<CylinderShape> fromShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			dest.position,
			m_waypoints[idx + 1].position,
			m_waypointConnectRad)));
		// Internal point (2 arrows to update)
		removeDebugShape(m_arrowIDs[idx - 1]);
		removeDebugShape(m_arrowIDs[idx]);
		m_arrowIDs[idx-1] = debugDraw(toShape, finf(), m_waypointColor, Color4::clear());
		m_arrowIDs[idx] = debugDraw(fromShape, finf(), m_waypointColor, Color4::clear());
	}
	else if (idx == m_waypoints.lastIndex()) {
		shared_ptr<CylinderShape> toShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints[idx - 1].position,
			dest.position,
			m_waypointConnectRad)));
		// Last point (just remove "to" arrow)
		removeDebugShape(m_arrowIDs[idx - 1]);
		m_arrowIDs[idx - 1] = debugDraw(toShape, finf(), m_waypointColor, Color4::clear());
	}
	else if (idx == 0) {
		shared_ptr<CylinderShape> fromShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			dest.position,
			m_waypoints[idx + 1].position,
			m_waypointConnectRad)));
		// First point (just remove the "from" arrow)
		removeDebugShape(m_arrowIDs[idx]);
		m_arrowIDs[idx] = debugDraw(fromShape, finf(), m_waypointColor, Color4::clear());
	}
	return true;
}

void App::removeHighlighted(void) {
	// Remove the selected waypoint
	removeWaypoint(m_waypointControls->getSelected());
}

bool App::removeWaypoint(int idx) {
	// Check for valid idx
	if (idx >= 0 && idx < m_waypoints.size()) {
		// Check if we are not at the first or last point
		if (idx > 0 && idx < m_waypoints.lastIndex()) {
			// Remove the arrows to and from this point
			removeDebugShape(m_arrowIDs[idx]);
			removeDebugShape(m_arrowIDs[idx-1]);
			m_arrowIDs.remove(idx-1, 2);
			// Draw a new arrow "around" the point
			shared_ptr<CylinderShape> shape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
				m_waypoints[idx-1].position,
				m_waypoints[idx+1].position,
				m_waypointConnectRad)));
			DebugID arrowID = debugDraw(shape, finf(), m_waypointColor, Color4::clear());
			m_arrowIDs.insert(idx-1, arrowID);
		}
		// Otherwise check if we are at the last index (just remove the arrow to this point)
		else if (idx == m_waypoints.lastIndex() && idx > 0) {
			// Remove the arrow from this point
			removeDebugShape(m_arrowIDs.last());
			m_arrowIDs.remove(m_arrowIDs.lastIndex());
		}

		// Remove the waypoint and its debug shape
		removeDebugShape(m_waypointIDs[idx]);
		m_waypointIDs.fastRemove(idx);
		m_waypoints.fastRemove(idx);

		// Get rid of the debug highlight and clear selection
		m_waypointControls->setSelected(-1);
		removeDebugShape(m_highlighted);
		return true;
	}
	else 
		return false;
}

void App::removeLastWaypoint(void) {
	if (m_waypoints.size() > 0) {
		removeWaypoint(m_waypoints.lastIndex());
	}
}

void App::clearWaypoints(void) {
	m_waypoints.clear();
	for (DebugID id : m_waypointIDs) {
		removeDebugShape(id);
	}
	m_waypointIDs.clear();
	for (DebugID id : m_arrowIDs) {
		removeDebugShape(id);
	}
	m_arrowIDs.clear();
	// Clear the highlighted shape
	removeDebugShape(m_highlighted);
	m_waypointControls->setSelected(-1);
}

void App::exportWaypoints(void) {
	TargetConfig t = TargetConfig();
	t.id = "test";
	t.destSpace = "world";
	t.destinations = m_waypoints;
	t.toAny().save(waypointFile);		// Save the file
}

void App::loadWaypoints(void) {
	String fname;
	bool gotName = FileDialog::getFilename(fname, "Any", false);
	if (!gotName) return;

	recordMotion = false;		// Stop recording (if doing so)
	clearWaypoints();			// Clear the current waypoints

	TargetConfig t = TargetConfig::load(fname);	// Load the target config
	if (t.destinations.size() > 0) {
		setWaypoints(t.destinations);
	}
}

void App::setWaypoints(Array<Destination> waypoints) {
	m_waypoints = waypoints;
	for (Destination d : waypoints) {
		dropWaypoint(d);
	}
}

void App::previewWaypoints(void) {
	// Check if a preview target exists, if so remove it
	if (m_previewIdx >= 0) {
		destroyTarget(m_previewIdx);
		m_previewIdx = -1;
	}
	if (m_waypoints.size() > 1) {
		// Create a new target and set its index
		spawnDestTarget(Vector3::zero(), m_waypoints, 1.0, Color3::white(), "reference", 0, 0, "preview");
		m_previewIdx = targetArray.size() - 1;
	}
}

void App::stopPreview(void) {
	if (m_previewIdx >= 0) {			// Check if a preview target exists
		destroyTarget(m_previewIdx);	// Destory the target
		m_previewIdx = -1;				// Use -1 value to indicate no preview present
	}
}

void App::exportScene() {
	CFrame frame = scene()->typedEntity<PlayerEntity>("player")->frame();
	logPrintf("Player position is: [%f, %f, %f]\n", frame.translation.x, frame.translation.y, frame.translation.z);
	String filename = Scene::sceneNameToFilename(sessConfig->sceneName);
	scene()->toAny().save(filename);
}

void App::showWaypointManager() {
	m_waypointControls->setVisible(true);
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
    m_currentUserPane->addLabel(format("Current User: %s", userTable.currentUser));
    m_mouseDPILabel = m_currentUserPane->addLabel(format("Mouse DPI: %f", userTable.getCurrentUser()->mouseDPI));
    m_currentUserPane->addNumberBox("Mouse 360", &(userTable.getCurrentUser()->cmp360), "cm", GuiTheme::LINEAR_SLIDER, 0.2, 100.0, 0.2);
    m_currentUserPane->addButton("Save cm/360", this, &App::userSaveButtonPress);
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
	for (int i = 0; i < userStatus->sessionOrder.size(); i++) {
        // user hasn't completed this session
        if (!userStatus->completedSessions.contains(userStatus->sessionOrder[i])) {
            remainingSess.append(userStatus->sessionOrder[i]);
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

void App::updateParameters(int frameDelay, float frameRate) {
	// Apply frame lag
	displayLagFrames = frameDelay;
	// Set a maximum *finite* frame rate
	float dt = 0;
	if (frameRate > 0) dt = 1.0f / frameRate;
	else dt = 1.0f / float(window()->settings().refreshRate);
	setFrameDuration(dt, GApp::REAL_TIME);
	m_renderControls->frameRate = frameRate;
}


void App::updateSession(String id) {
	// Check for a valid ID (non-emtpy and 
	Array<String> ids;
	experimentConfig.getSessionIds(ids);
	if (!id.empty() && ids.contains(id)) {
		sessConfig = experimentConfig.getSessionConfigById(id);						// Get the new session config
		logPrintf("User selected session: %s. Updating now...\n", id);				// Print message to log
		m_sessDropDown->setSelectedValue(id);										// Update session drop-down selection
		sess = Session::create(this, sessConfig);									// Create the session
	}
	else {
		sessConfig = SessionConfig::create();										// Create an empty session
		sess = Session::create(this, sessConfig);											
	}

	// Update the weapon controls
	m_weaponControls = WeaponControls::create(sessConfig->weapon, theme);
	m_weaponControls->setVisible(false);
	this->addWidget(m_weaponControls);

	// Update the frame rate/delay
	updateParameters(sessConfig->render.frameDelay, sessConfig->render.frameRate);

	// Load (session dependent) fonts
	hudFont = GFont::fromFile(System::findDataFile(sessConfig->hud.hudFont));
	m_combatFont = GFont::fromFile(System::findDataFile(sessConfig->targetView.combatTextFont));

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

	// Check for play mode specific parameters
	m_fireSound = Sound::create(System::findDataFile(sessConfig->weapon.fireSound));
	m_explosionSound = Sound::create(System::findDataFile(sessConfig->audio.explosionSound));

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
	// Copied from old FPM code
	double mouseSens = 2.0 * pi() * 2.54 * 1920.0 / (userTable.getCurrentUser()->cmp360 * userTable.getCurrentUser()->mouseDPI);
	mouseSens *= 1.0675; // 10.5 / 10.0 * 30.5 / 30.0
	player->mouseSensitivity = (float)mouseSens;
	player->moveRate = sessConfig->player.moveRate;
	player->jumpVelocity = sessConfig->player.jumpVelocity;
	player->jumpInterval = sessConfig->player.jumpInterval;
	player->jumpTouch = sessConfig->player.jumpTouch;
	player->height = sessConfig->player.height;
	player->crouchHeight = sessConfig->player.crouchHeight;
	updateMoveRate(sessConfig->player.moveRate);

	// Make sure all targets are cleared
	clearTargets();

	// Check for need to start latency logging and if so run the logger now
	SystemConfig sysConfig = SystemConfig::load();
	String logName = "../results/" + id + "_" + userTable.currentUser + "_" + String(Logger::genFileTimestamp());
	if (sysConfig.hasLogger) {
		if (m_pyLogger == nullptr) {
			m_pyLogger = PythonLogger::create(sysConfig.loggerComPort, sysConfig.hasSync, sysConfig.syncComPort);
		}
		else {
			// Handle running logger if we need to (terminate then merge results)
			m_pyLogger->mergeLogToDb();
		}
		// Run a new logger if we need to
		m_pyLogger->run(logName);
	}

	// Initialize the experiment (this creates the results file)
	sess->onInit(logName+".db", userTable.currentUser, experimentConfig.description + "/" + sessConfig->description);
	// Don't create a results file for a user w/ no sessions left
	if (m_sessDropDown->numElements() == 0) {
		logPrintf("No sessions remaining for selected user.\n");
	}
	else {
		logPrintf("Created results file: %s.db\n", logName.c_str());
	}
}

void App::quitRequest() {
	if (m_pyLogger != nullptr) {
		m_pyLogger->mergeLogToDb();
	}
    setExitCode(0);
}

void App::onAfterLoadScene(const Any& any, const String& sceneName) {
	// Pick between experiment and session settings
	Vector3 grav = experimentConfig.player.playerGravity;
	float FoV = experimentConfig.render.hFoV;
	if (sessConfig != nullptr) {
		grav = sessConfig->player.playerGravity;
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
	player->setRespawnPosition(activeCamera()->frame().translation);
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
	activeCamera()->filmSettings().setAntialiasingEnabled(!emergencyTurbo);
	activeCamera()->filmSettings().setBloomStrength(emergencyTurbo ? 0.0f : 0.5f);

	GApp::onGraphics3D(rd, surface);

	if (displayLagFrames > 0) {
		// Display the delayed frame
		rd->popState();
		rd->push2D(); {
			// Advance the pointer to the next, which is also the oldest frame
			m_currentDelayBufferIndex = (m_currentDelayBufferIndex + 1) % (displayLagFrames + 1);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrDelayBufferQueue[m_currentDelayBufferIndex]->texture(0), Sampler::buffer());
		} rd->pop2D();
	}
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

	// TODO (or NOTTODO): The following can be cleared at the cost of one more level of inheritance.
	sess->onSimulation(rdt, sdt, idt);

	// These are all we need from GApp::onSimulation() for walk mode
	m_widgetManager->onSimulation(rdt, sdt, idt);
	if (scene()) { scene()->onSimulation(sdt); }
	if (scene()) { scene()->onSimulation(sdt); }

	// make sure mouse sensitivity is set right
	if (m_userSettingsMode) {
		updateMouseSensitivity();
		m_userSettingsWindow->setVisible(m_userSettingsMode);		// Make sure window stays coherent w/ user settings mode
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
			projectile.entity->setFrame(projectile.entity->frame() + projectile.entity->frame().lookVector() * sessConfig->weapon.bulletSpeed);
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

	// Move the player
	const shared_ptr<PlayerEntity>& p = scene()->typedEntity<PlayerEntity>("player");
	activeCamera()->setFrame(p->frame());

	// Handle developer mode features here
	if (!startupConfig.playMode) {
		// Copy over dynamic elements
		// TODO:Consider moving to App owning GuiPane, and no need for all this copy-over...
		shared_ptr<PlayerEntity> player = scene()->typedEntity<PlayerEntity>("player");
		player->height = m_playerControls->playerHeight;
		player->crouchHeight = m_playerControls->crouchHeight;
		player->moveRate = m_playerControls->moveRate;

		sessConfig->hud.enable = m_renderControls->showHud;
		sessConfig->weapon.renderModel = m_renderControls->showWeapon;
		sessConfig->weapon.renderBullets = m_renderControls->showBullets;
		
		renderFPS = m_renderControls->showFps;
		emergencyTurbo = m_renderControls->turboMode;

		sessConfig->render.frameRate = m_renderControls->frameRate;
		updateParameters(m_renderControls->frameDelay, m_renderControls->frameRate);

		reticleIndex = m_renderControls->reticleIdx;
		sceneBrightness = m_renderControls->brightness;

		// Handle highlighting for selected target
		int selIdx = m_waypointControls->getSelected();
		if (selIdx >= 0) {
			// Handle position update
			Destination dest = m_waypoints[selIdx];
			dest.position += m_waypointMoveRate * m_waypointMoveMask;
			// This handles drawing the "highlight"
			updateWaypoint(dest);
		}

		// Handle player motion recording here (if we are doing so w/ playMode=False)
		if (recordMotion) {
			// Get the start time here if needed (this is the first iteration after enable)
			if (isnan(m_recordStart)) {
				m_recordStart = now;
				clearWaypoints();		// Just clear the waypoints for now
				dropWaypoint(Destination(p->frame().translation, 0.0f));
			}
			else {
				SimTime t = static_cast<SimTime>(now - m_recordStart);
				float distance = (m_waypoints.last().position - p->frame().translation).magnitude();
				switch (recordMode) {
				case 0: // This is fixed distance mode, check if we've moved "far enough" to drop a new waypoint
					if (distance > recordInterval) {
						// Use the m_waypointDelay to meter out time when in constant distance mode
						t = m_waypoints.size() * waypointDelay;
						dropWaypoint(Destination(p->frame().translation, t));
					}
					break;
				case 1: // This is fixed time mode, check if we are beyond the sampling interval and need a new waypoint
					if ((t - m_lastRecordTime) > recordInterval) {
						m_lastRecordTime = t;
						// Apply the recording time-scaling here (after checking for record interval)
						dropWaypoint(Destination(p->frame().translation, t / recordTimeScaling));
					}
					break;
				}
			}
		}
		else {
			m_recordStart = nan();
			m_lastRecordTime = 0.0f;
		}

		// Example GUI dynamic layout code.  Resize the debugWindow to fill
		// the screen horizontally.
		debugWindow->setRect(Rect2D::xywh(0.0f, 0.0f, (float)window()->width(), debugWindow->rect().height()));
	}
	   
	// Check for completed session
	if (sess->moveOn) {
		String nextSess = userStatusTable.getNextSession(userTable.currentUser);
		updateSession(nextSess);
	}
}

bool App::onEvent(const GEvent& event) {
	// Handle playMode=False shortcuts here...
	if (!startupConfig.playMode) {
		if (event.type == GEventType::KEY_DOWN) {
			bool foundKey = true;
			int selIdx = m_waypointControls->getSelected();
			switch (event.key.keysym.sym) {
			// Handle "hot keys" here
			case 'q':							// Use 'q' to drop a waypoint
				dropWaypoint();
				break;
			case 'r':							// Use 'r' to toggle recording of player motion
				recordMotion = !recordMotion;
				break;

			// Handle shortcuts for opening sub-menus here
			case '1':							// Use '1' to toggle the rendering controls
				m_renderControls->setVisible(!m_renderControls->visible());
				break;
			case '2':							// Use '2' to toggle the player controls
				m_playerControls->setVisible(!m_playerControls->visible());
				break;
			case '3':							// Use '3' to toggle the waypoint manager
				m_waypointControls->setVisible(!m_waypointControls->visible());
				break;
			case '4':
				m_weaponControls->setVisible(!m_weaponControls->visible());
				break;
			case GKey::PAGEUP:
				m_waypointMoveMask += Vector3(0.0f, 1.0f, 0.0f);
				break;
			case GKey::PAGEDOWN:
				m_waypointMoveMask += Vector3(0.0f, -1, 0);
				break;
			case GKey::HOME:
				m_waypointMoveMask += Vector3(0, 0, 1);
				break;
			case GKey::END:
				m_waypointMoveMask += Vector3(0, 0, -1);
				break;
			case GKey::INSERT:
				m_waypointMoveMask += Vector3(1, 0, 0);
				break;
			case GKey::DELETE:
				m_waypointMoveMask += Vector3(-1, 0, 0);
				break;
			// Handle unknown keypress here...
			default:
				foundKey = false;
				break;
			}
			if (foundKey) {
				return true;
			}
		}
		else if (event.type == GEventType::KEY_UP) {
			bool foundKey = true;
			// Handle release for waypoint keys here...
			switch (event.key.keysym.sym) {
			case GKey::PAGEUP:
				m_waypointMoveMask -= Vector3(0, 1, 0);
				break;
			case GKey::PAGEDOWN:
				m_waypointMoveMask -= Vector3(0, -1, 0);
				break;
			case GKey::HOME:
				m_waypointMoveMask -= Vector3(0, 0, 1);
				break;
			case GKey::END:
				m_waypointMoveMask -= Vector3(0, 0, -1);
				break;
			case GKey::INSERT:
				m_waypointMoveMask -= Vector3(1, 0, 0);
				break;
			case GKey::DELETE:
				m_waypointMoveMask -= Vector3(-1, 0, 0);
				break;
			default: 
				foundKey = false;
			}
			if (foundKey) {
				return true;
			}
		}
	}
	
	// Handle normal keypresses
	if (event.type == GEventType::KEY_DOWN) {
		if (event.key.keysym.sym == GKey::ESCAPE || event.key.keysym.sym == GKey::TAB) {
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

		// Override 'q', 'z', 'c', and 'e' keys
		else if ((event.key.keysym.sym == 'e'
			|| event.key.keysym.sym == 'z'
			|| event.key.keysym.sym == 'c'
			|| event.key.keysym.sym == 'q')) {
			return true;
		}
		else if (event.key.keysym.sym == GKey::KP_MINUS) {
			quitRequest();
			return true;
		}
		else if (event.key.keysym.sym == GKey::LCTRL) {
			scene()->typedEntity<PlayerEntity>("player")->setCrouched(true);
			return true;
		}
	}
	else if ((event.type == GEventType::KEY_UP) && (event.key.keysym.sym == GKey::LCTRL)) {
		scene()->typedEntity<PlayerEntity>("player")->setCrouched(false);
		return true;
	}

	// Handle super-class events
	if (GApp::onEvent(event)) { return true; }
	return false;
}

void App::onPostProcessHDR3DEffects(RenderDevice *rd) {
	GApp::onPostProcessHDR3DEffects(rd);
}

void App::drawHUD(RenderDevice *rd) {
	// Draw the HUD elements
	const Vector2 scale = Vector2(rd->viewport().width()/1920.0f, rd->viewport().height()/1080.0f);

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
		const Point2 hudCenter(rd->viewport().width() / 2.0f, sessConfig->hud.bannerVertVisible*hudTexture->height() * scale.y + debugMenuHeight());
		Draw::rect2D((hudTexture->rect2DBounds() * scale - hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), hudTexture);

		// Create strings for time remaining, progress in sessions, and score
		float remainingTime = sess->getRemainingTrialTime();
		float printTime = remainingTime > 0 ? remainingTime : 0.0f;
		String time_string = format("%0.2f", printTime);
		float prog = sess->getProgress();
		String prog_string = "";
		if (!isnan(prog)) {
			prog_string = format("%d", (int)(100.0f*sess->getProgress())) + "%";
		}
		String score_string = format("%d", (int)(10 * sess->getScore()));

		hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale.x, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale.x * sessConfig->hud.bannerLargeFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}
}

/** Method for handling weapon fire */
shared_ptr<TargetEntity> App::fire(bool destroyImmediately) {
    BEGIN_PROFILER_EVENT("fire");
	Point3 aimPoint = activeCamera()->frame().translation + activeCamera()->frame().lookVector() * 1000.0f;
	bool destroyedTarget = false;
	static bool hitTarget = false;
	static RealTime lastTime;
	shared_ptr<TargetEntity> target = nullptr;

	if (m_hitScan) {
		const Ray& ray = activeCamera()->frame().lookRay();		// Use the camera lookray for hit detection
		Array<shared_ptr<Entity>> dontHit = { m_explosion, m_lastDecal, m_firstDecal };
		for (auto projectile : projectileArray) {
			dontHit.append(projectile.entity);
		}
		for (auto target : targetArray) {
			dontHit.append(target);
		}
		// Check for closest hit
		float closest = finf();
		int closestIndex = -1;
		scene()->intersect(ray, closest, false, dontHit);
		for (int t = 0; t < targetArray.size(); ++t) {
			if (targetArray[t]->intersect(ray, closest)) {
				closestIndex = t;
			}
		}

		// Hit logic
		if (closestIndex >= 0) {
			target = targetArray[closestIndex];			// Assign the target pointer here (not null indicates the hit)

			// Damage the target
			float damage;
			if (destroyImmediately) damage = target->health();
			else if (sessConfig->weapon.firePeriod == 0.0f && hitTarget) {		// Check if we are in "laser" mode hit the target last time
				float dt = max(previousSimTimeStep(),0.0f);
				damage = sessConfig->weapon.damagePerSecond * dt;
			}
			else {																// If we're not in "laser" mode then damage/shot is just damage/second * second/shot
				damage = sessConfig->weapon.damagePerSecond * sessConfig->weapon.firePeriod;
			}
			hitTarget = true;

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
				const shared_ptr<VisibleEntity> newExplosion = VisibleEntity::create("explosion", scene().get(), m_explosionModels[target->scaleIndex()], explosionFrame);
				scene()->insert(newExplosion);
				m_explosion = newExplosion;
				m_explosionEndTime = System::time() + 0.1f; // make explosion end in 0.5 seconds
				sess->countDestroy();
				respawned = target->respawn();
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
		// Create the bullet start frame from the weapon frame plus muzzle offset
		CFrame bulletStartFrame = m_weaponFrame;
		bulletStartFrame.translation += sessConfig->weapon.muzzleOffset;

		// Angle the bullet start frame towards the aim point
		bulletStartFrame.lookAt(aimPoint);

		bulletStartFrame.translation += bulletStartFrame.lookVector() * 2.0f;
		const shared_ptr<VisibleEntity>& bullet = VisibleEntity::create(format("bullet%03d", ++m_lastUniqueID), scene().get(), m_bulletModel, bulletStartFrame);
		bullet->setShouldBeSaved(false);
		bullet->setCanCauseCollisions(false);
		bullet->setCastsShadows(false);

		/*
		const shared_ptr<Entity::Track>& track = Entity::Track::create(bullet.get(), scene().get(),
			Any::parse(format("%s", bulletStartFrame.toXYZYPRDegreesString().c_str())));
		bullet->setTrack(track);
		*/

		projectileArray.push(Projectile(bullet, System::time() + 1.0f));
		scene()->insert(bullet);
	}

    // play sounds
    if (destroyedTarget) {
		m_explosionSound->play(sessConfig->audio.explosionSoundVol);
		//m_explosionSound->play(target->frame().translation, Vector3::zero(), 50.0f);
	}
	else if(sessConfig->weapon.firePeriod > 0.0f || !sessConfig->weapon.autoFire) {
		m_fireSound->play(sessConfig->weapon.fireSoundVol);
		//m_fireSound->play(activeCamera()->frame().translation, activeCamera()->frame().lookVector() * 2.0f, 0.5f);
	}

	if (sessConfig->weapon.renderDecals && sessConfig->weapon.firePeriod > 0.0f && !hitTarget) {
		// compute world intersection
		const Ray& ray = activeCamera()->frame().lookRay();
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
		CFrame decalFrame = activeCamera()->frame();
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
}

/** Handle user input here */
void App::onUserInput(UserInput* ui) {
    BEGIN_PROFILER_EVENT("onUserInput");
	static bool haveReleased = false;
	static bool fired = false;
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

	// Require release between clicks for non-autoFire modes
	if (ui->keyReleased(GKey::LEFT_MOUSE)) {
		m_buttonUp = true;
			if (!sessConfig->weapon.autoFire) {
			haveReleased = true;
			fired = false;
		}
	}

	// Handle the mouse down events
	if (ui->keyDown(GKey::LEFT_MOUSE)) {
		if (sessConfig->weapon.autoFire || haveReleased) {		// Make sure we are either in autoFire mode or have seen a release of the mouse
			// check for hit, add graphics, update target state
			if ((sess->presentationState == PresentationState::task) && !m_userSettingsMode) {
				if (sess->canFire()) {

					fired = true;
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

		// Check for developer mode editing here
		if (!startupConfig.playMode) {
			const shared_ptr<Camera> cam = activeCamera();
			float closest = 1e6;
			int closestIdx = -1;
			// Crude hit-scane here w/ spheres
			for (int i = 0; i < m_waypoints.size(); i++) {
				Sphere pt = Sphere(m_waypoints[i].position, m_waypointRad);
				float distance = (m_waypoints[i].position - cam->frame().translation).magnitude();	// Get distance to the target
				const Point3 center = cam->frame().translation + cam->frame().lookRay().direction()*distance;
					Sphere probe = Sphere(center, m_waypointRad / 4);
					// Get closest intersection
					if (pt.intersects(probe) && distance < closest) {
						closestIdx = i;
						closest = distance;
				}
			}
			if (closestIdx != -1) {							// We are "hitting" this item
				m_waypointControls->setSelected(closestIdx);
			}
		}

		haveReleased = false;					// Make it known we are no longer in released state
		m_buttonUp = false;
	}
	
	// Handle spacebar during feedback
    GKey initShootKey = GKey::LSHIFT;
	if (ui->keyPressed(initShootKey) && (sess->presentationState == PresentationState::feedback)) {
		fire(true); // Space for ready target (destroy this immediately regardless of weapon)
	}	

	if (m_lastReticleLoaded != reticleIndex) {
		// Slider was used to change the reticle
		setReticle(reticleIndex);
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

	if (sessConfig->weapon.renderModel) {
		const float yScale = -0.12f;
		const float zScale = -yScale * 0.5f;
		const float lookY = activeCamera()->frame().lookVector().y;
		const float prevLookY = activeCamera()->previousFrame().lookVector().y;
		m_weaponFrame = activeCamera()->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
		const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
		m_viewModel->pose(surface, m_weaponFrame, activeCamera()->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
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

			outputFont->draw2D(rd, msg, (Point2(30, 28) * scale).floor(), floor(20.0f * scale), Color3::yellow());
		}

		// Handle recording indicator
		if (recordMotion) {
			Draw::point(Point2(window()->width()*0.9f - 15.0f, 20.0f+m_debugMenuHeight*scale), rd, Color3::red(), 10.0f);
			outputFont->draw2D(rd, "Recording Position", Point2(window()->width()*0.9f, m_debugMenuHeight*scale), 20.0f, Color3::red());
		}

		// Reticle
		Draw::rect2D(
			(reticleTexture->rect2DBounds() * scale - reticleTexture->vector2Bounds() * scale / 2.0f) / 2.0f + rd->viewport().wh() / 2.0f,
			rd, Color3::green(), reticleTexture);

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

		// Paint both sides by the width of latency measuring box.
		Point2 latencyRect = sessConfig->clickToPhoton.size;
		// weapon ready status
		if (sessConfig->hud.renderWeaponStatus) {
			// Draw the "active" cooldown box
			if (sessConfig->hud.cooldownMode == "box") {
				float boxLeft = (float)rd->viewport().width() * 0.0f;
				if (sessConfig->hud.weaponStatusSide == "right") {
					// swap side
					boxLeft = (float)rd->viewport().width() * (1.0f - latencyRect.x);
				}
				Draw::rect2D(
					Rect2D::xywh(
						boxLeft,
						(float)rd->viewport().height() * (float)(sess->weaponCooldownPercent()),
						(float)rd->viewport().width() * latencyRect.x,
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

		// Click to photon latency measuring corner box
		if (sessConfig->clickToPhoton.enabled) {
			float boxLeft = 0.0f;
			if (sessConfig->clickToPhoton.side == "right") {
				// swap side
				boxLeft = (float)rd->viewport().width() * (1.0f - latencyRect.x);
			}
			// Draw the "active" box
			Color3 cornerColor = (m_buttonUp) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
			Draw::rect2D(
				Rect2D::xywh(
					boxLeft,
					(float)rd->viewport().height() * (sessConfig->clickToPhoton.vertPos - latencyRect.y / 2),
					(float)rd->viewport().width() * latencyRect.x,
					(float)rd->viewport().height() * latencyRect.y
				), rd, cornerColor
			);
		}

		// Draw the HUD here
		if (sessConfig->hud.enable) {
			drawHUD(rd);
		}

		// Handle the feedback message
		String message = sess->getFeedbackMessage();
		if (!message.empty()) {
			outputFont->draw2D(rd, message.c_str(),
				(Point2((float)window()->width() / 2, (float)window()->height() / 2) * scale).floor(), floor(20.0f * scale), Color3::yellow(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		}


	} rd->pop2D();

	if (sessConfig->render.shader != "") {
		// This code could be run more efficiently at LDR after Film::exposeAndRender or even during the
		// latency queue copy

		// Think about whether or not this is the right place to run the shader...

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
void App::setReticle(int r) {
	m_lastReticleLoaded = reticleIndex = clamp(0, r, numReticles);
	if (r < numReticles) {
		reticleTexture = Texture::fromFile(System::findDataFile(format("gui/reticle/reticle-%03d.png", reticleIndex)));
	}
	else {
		// This special case is added to allow a custom reticle not in the gui/reticle/reticle-[x].png format
		reticleTexture = Texture::fromFile(System::findDataFile("gui/reticle.png"));
	}
}

void App::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
}

/** Overridden (optimized) oneFrame() function to improve latency */
void App::oneFrame() {

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
}


// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {

    if (FileSystem::exists("startupconfig.Any")) {
        startupConfig = Any::fromFile("startupconfig.Any");
    }
    else {
        // autogenerate if it wasn't there
        startupConfig.toAny().save("startupconfig.Any");
    }

	{
		G3DSpecification spec;
        spec.audio = startupConfig.audioEnable;
		initGLG3D(spec);
	}

	(void)argc; (void)argv;
	GApp::Settings settings(argc, argv);

	if (startupConfig.playMode) {
		settings.window.width = 1920; settings.window.height = 1080;
	}
	else {
		settings.window.width = 1920; settings.window.height = 980;
	}
	settings.window.fullScreen = startupConfig.playMode;
	settings.window.resizable = !settings.window.fullScreen;

    // V-sync off always
	settings.window.asynchronous = true;
	settings.window.caption = "First Person Science";
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

