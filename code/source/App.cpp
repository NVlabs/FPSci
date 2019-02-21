/** \file App.cpp */
#include "App.h"

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCPUTimer()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		std::cout << "QueryPerformanceFrequency failed!\n";

	PCFreq = double(li.QuadPart);

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCPUTime() // unit is second
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

// Set to false when just editing content
static const bool playMode = true;
// Enable this to see maximum CPU/GPU rate when not limited
// by the monitor. 
static const bool  unlockFramerate = false;

// Set to true if the monitor has G-SYNC/Adaptive VSync/FreeSync, 
// which allows the application to submit asynchronously with vsync
// without tearing.
static const bool  variableRefreshRate = true;

const float App::TARGET_MODEL_ARRAY_SCALING = 0.2f;
const float App::TARGET_MODEL_ARRAY_OFFSET = 30;

//static const float verticalFieldOfViewDegrees = 90; // deg
static const float horizontalFieldOfViewDegrees = 103; // deg

static const bool measureClickPhotonLatency = true;

// JBS: TODO: Refactor these as experiment variables
//========================================================================
// variables related to experimental condition and record.
//static const float targetFrameRate = 360; // hz
//const int numFrameDelay = 0;
//static const std::string expMode = "training"; // training or real
//static const std::string taskType = "reaction"; // reaction or targeting
//static const std::string appendingDescription = "ver1";
//========================================================================

App::App(const GApp::Settings& settings) : GApp(settings) {
	// TODO: make method that changes definition of ex, and have constructor call that
	// method to set default experiment
	// JBS: moved experiment definition to `onInit()`

}


void App::onInit() {
	GApp::onInit();

    // load settings from file
    UserConfig u(Any::fromFile(System::findDataFile("userconfig.Any")));
    m_cmp360 = u.cmp360;
    m_mouseDPI = u.mouseDPI;
    m_subjectID = u.subjectID;
    // debug print
    debugPrintf("User: %s, DPI: %f, cmp360 %f\n", m_subjectID, m_mouseDPI, m_cmp360);
	ExperimentConfig e(Any::fromFile(System::findDataFile("experimentconfig.Any")));
	m_targetFrameRate = e.targetFrameRate;
	m_expMode = e.expMode;
	m_expVersion = e.expVersion;
	m_taskType = e.taskType;
	m_appendingDescription = e.appendingDescription;
	// debug print
	debugPrintf("Target Framerate %f, expMode: %s, taskType: %s, appendingDescription: %s\n",
		m_targetFrameRate, m_expMode, m_taskType, m_appendingDescription);

	// TODO: app pointer is not a memory-managed pointer.
	if (m_taskType == "reaction") {
		m_ex = std::make_shared<Psychophysics::ReactionExperiment>(this);
		dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->setFrameRate(m_targetFrameRate);
	}
	else if (m_taskType == "targeting") {
		m_ex = std::make_shared<Psychophysics::TargetingExperiment>(this);
		dynamic_pointer_cast<Psychophysics::TargetingExperiment>(m_ex)->setFrameRate(m_targetFrameRate);
	}

	float dt = 0;
	if (unlockFramerate) {
		// Set a maximum *finite* frame rate
		dt = 1.0f / 8192.0f;
	}
	else if (variableRefreshRate) {
		dt = 1.0f / m_targetFrameRate;
	}
	else {
		dt = 1.0f / float(window()->settings().refreshRate);
	}
	setFrameDuration(dt, GApp::REAL_TIME);
	setSubmitToDisplayMode(
		//SubmitToDisplayMode::MINIMIZE_LATENCY);
		SubmitToDisplayMode::BALANCE);
	//SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);    
	showRenderingStats = false;
	makeGUI();
	developerWindow->videoRecordDialog->setCaptureGui(true);
	m_outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	m_hudFont = GFont::fromFile(System::findDataFile("dominant.fnt"));
	m_hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));

	if (playMode) {
		m_fireSound = Sound::create(System::findDataFile("sound/42108__marcuslee__Laser_Wrath_6.wav"));
		m_explosionSound = Sound::create(System::findDataFile("sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav"));
	}

	loadModels();
	setReticle(m_reticleIndex);
	
	if (m_ex->expName == "TargetingExperiment") {
		loadScene("eSports Simple Hallway");
	}

	initPsychophysicsLib();

	//spawnTarget(Point3(37.6184f, -0.54509f, -2.12245f), 1.0f);
	//spawnTarget(Point3(39.7f, -2.3f, 2.4f), 1.0f);

	if (playMode) {
		// Force into FPS mode
		const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
		fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
		fpm->setMoveRate(0.0);

		// G3D expects mouse sensitivity in radians
		// we're converting from mouseDPI and centimeters/360 which explains
		// the screen resolution (dots), cm->in factor (2.54) and 2PI
		double mouseSensitivity = 2.0 * pi() * 2.54 * 1920.0 / (m_cmp360 * m_mouseDPI);
		fpm->setTurnRate(mouseSensitivity);
	}
	else {
		// fix mouse sensitivity for developer mode
		const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
		double mouseSensitivity = 2.0 * pi() * 2.54 * 1920.0 / (m_cmp360 * m_mouseDPI);
		fpm->setTurnRate(mouseSensitivity);

	}

}

void App::spawnRandomTarget() {
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
			spawnTarget(ray.origin() + ray.direction() * rng.uniform(2.0f, distance - 1.0f), rng.uniform(0.1f, 1.5f), rng.uniform() > 0.5f);
			done = true;
		}
		++tries;
	} while (!done && tries < 100);
}


shared_ptr<VisibleEntity> App::spawnTarget(const Point3& position, float scale, bool spinLeft) {
	const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, m_targetModelArray.length() - 1);

	const shared_ptr<VisibleEntity>& target = VisibleEntity::create(format("target%03d", ++m_lastUniqueID), scene().get(), m_targetModelArray[scaleIndex], CFrame());
	String animation = format("combine(orbit(0, %d), CFrame::fromXYZYPRDegrees(%f, %f, %f))", spinLeft ? 1 : -1, position.x, position.y, position.z);

	// Don't set a track. We'll take care of the positioning after creation
	//const shared_ptr<Entity::Track>& track = Entity::Track::create(target.get(), scene().get(), Any::parse(animation));
	//target->setTrack(track);
	target->setShouldBeSaved(false);
	m_targetArray.append(target);
	scene()->insert(target);
	return target;
}

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

	for (int i = 0; i <= 20; ++i) {
		const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - TARGET_MODEL_ARRAY_OFFSET);
		m_targetModelArray.push(ArticulatedModel::create(Any::parse(format(STR(ArticulatedModel::Specification{
			filename = "ifs/d12.ifs";
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
			preprocess = preprocess{
				setMaterial(all(), UniversalMaterial::Specification {
				emissive = Color3(0.7, 0, 0);
				glossy = Color4(0.4, 0.2, 0.1, 0.8);
				lambertian = Color3(1, 0.09, 0);
				}) };
			};), scale))));
	}
}



void App::makeGUI() {
	debugWindow->setVisible(!playMode);
	developerWindow->setVisible(!playMode);
	developerWindow->sceneEditorWindow->setVisible(!playMode);
	developerWindow->cameraControlWindow->setVisible(!playMode);
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

	debugWindow->pack();
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
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

	if (m_ex->expName == "ReactionExperiment") {
		if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
			swapBuffers();
		}
	}
	else {
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

		scene()->lightingEnvironment().ambientOcclusionSettings.enabled = !m_emergencyTurbo;
		m_debugCamera->filmSettings().setAntialiasingEnabled(!m_emergencyTurbo);
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
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// TODO (or NOTTODO): The following can be cleared at the cost of one more level of inheritance.
	if (m_ex->expName == "ReactionExperiment") {
		dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->updateAnimation(rdt);
	}
	else if (m_ex->expName == "TargetingExperiment") {
		dynamic_pointer_cast<Psychophysics::TargetingExperiment>(m_ex)->updateAnimation(rdt);
	}

	GApp::onSimulation(rdt, sdt, idt);

	const RealTime now = System::time();
	for (int p = 0; p < m_projectileArray.size(); ++p) {
		const Projectile& projectile = m_projectileArray[p];

		if (!m_hitScan) {
			// Check for collisions
		}

		if (projectile.endTime < now) {
			// Expire
			m_projectileArray.fastRemove(p);
			--p;
		}
		else {
			// Animate
			projectile.entity->setFrame(projectile.entity->frame() + projectile.entity->frame().lookVector() * 0.6f);
		}
	}

	// Example GUI dynamic layout code.  Resize the debugWindow to fill
	// the screen horizontally.
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}

bool App::onEvent(const GEvent& event) {
	// Handle super-class events
	if (GApp::onEvent(event)) { return true; }

	// If you need to track individual UI events, manage them here.
	// Return true if you want to prevent other parts of the system
	// from observing this specific event.
	//
	// For example,
	// if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
	// if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }
	// if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) { ... return true; }

	return false;
}

void App::onUserInput(UserInput* ui) {
	GApp::onUserInput(ui);
	(void)ui;


	if (playMode || m_debugController->enabled()) {
		if (ui->keyPressed(GKey::LEFT_MOUSE)) {
			m_buttonUp = false;

			if (m_ex->expName == "ReactionExperiment") {
				// set reacted to true
				dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->m_reacted = true;

			}
			else if (m_ex->expName == "TargetingExperiment") {
				// Fire
				Point3 aimPoint = m_debugCamera->frame().translation + m_debugCamera->frame().lookVector() * 1000.0f;

				if (m_hitScan) {
					const Ray& ray = m_debugCamera->frame().lookRay();

					float closest = finf();
					int closestIndex = -1;
					for (int t = 0; t < m_targetArray.size(); ++t) {
						if (m_targetArray[t]->intersect(ray, closest)) {
							closestIndex = t;
						}
					}

					if (closestIndex >= 0) {
						destroyTarget(closestIndex);
						aimPoint = ray.origin() + ray.direction() * closest;
						m_targetHealth -= dynamic_pointer_cast<Psychophysics::TargetingExperiment>(m_ex)->renderParams.weaponStrength; // TODO: health point should be tracked by Target Entity class (not existing yet).
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
					m_projectileArray.push(Projectile(laser, System::time() + 1.0f));
					scene()->insert(laser);
				}

				if (playMode) {
					m_fireSound->play(m_debugCamera->frame().translation, m_debugCamera->frame().lookVector() * 2.0f, 3.0f);
				}
			}
		}
		if (ui->keyReleased(GKey::LEFT_MOUSE)) {
			m_buttonUp = true;
		}
	}


	if (m_lastReticleLoaded != m_reticleIndex) {
		// Slider was used to change the reticle
		setReticle(m_reticleIndex);
	}

	m_debugCamera->filmSettings().setSensitivity(m_sceneBrightness);

}

void App::destroyTarget(int index) {
	// Not a reference because we're about to manipulate the array
	const shared_ptr<VisibleEntity> target = m_targetArray[index];
	m_targetArray.fastRemove(index);

	scene()->removeEntity(target->name());

	if (playMode) {
		// 3D audio
		m_explosionSound->play(target->frame().translation, Vector3::zero(), 16.0f);
	}
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

		if (m_ex->expName == "ReactionExperiment") {
			rd->clear();
			const float scale = rd->viewport().width() / 1920.0f;
			Draw::rect2D(rd->viewport(), rd, dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->m_stimColor);

			// Click to photon latency measuring corner box
			if (measureClickPhotonLatency) {
				Color3 cornerColor = (m_buttonUp) ? Color3::white() * 0.2 : Color3::white() * 0.8;
				//Draw::rect2D(rd->viewport().wh() / 10.0f, rd, cornerColor);
				Draw::rect2D(Rect2D::xywh((float)window()->width() * 0.9f, (float)window()->height() * 0.0f, (float)window()->width() * 0.1f, (float)window()->height() * 0.2f), rd, cornerColor);
			}

			if (!dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->m_feedbackMessage.empty()) {
				m_outputFont->draw2D(rd, dynamic_pointer_cast<Psychophysics::ReactionExperiment>(m_ex)->m_feedbackMessage.c_str(),
					(Point2((float)window()->width() / 2 - 40, (float)window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
			}
		}
		else if (m_ex->expName == "TargetingExperiment") {
			const float scale = rd->viewport().width() / 1920.0f;
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

			// Reticle
			Draw::rect2D((m_reticleTexture->rect2DBounds() * scale - m_reticleTexture->vector2Bounds() * scale / 2.0f) / 8.0f + rd->viewport().wh() / 2.0f, rd, Color3::green(), m_reticleTexture);

			if (m_renderHud && !m_emergencyTurbo) {
				const Point2 hudCenter(rd->viewport().width() / 2.0f, m_hudTexture->height() * scale * 0.48f);
				Draw::rect2D((m_hudTexture->rect2DBounds() * scale - m_hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), m_hudTexture);
				m_hudFont->draw2D(rd, "1:36", hudCenter - Vector2(80, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
				m_hudFont->draw2D(rd, "86%", hudCenter + Vector2(7, -1), scale * 30, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
				m_hudFont->draw2D(rd, "2080", hudCenter + Vector2(125, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
			}

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

			// Click to photon latency measuring corner box
			if (measureClickPhotonLatency) {
				Color3 cornerColor = (m_buttonUp) ? Color3::black() : Color3::white();
				//Draw::rect2D(rd->viewport().wh() / 10.0f, rd, cornerColor);
				Draw::rect2D(Rect2D::xywh((float)window()->width() * 0.9f, (float)window()->height() * 0.8f, (float)window()->width() * 0.1f, (float)window()->height() * 0.2f), rd, cornerColor);
			}

			if (!dynamic_pointer_cast<Psychophysics::TargetingExperiment>(m_ex)->m_feedbackMessage.empty()) {
				m_outputFont->draw2D(rd, dynamic_pointer_cast<Psychophysics::TargetingExperiment>(m_ex)->m_feedbackMessage.c_str(),
					(Point2((float)window()->width() / 2 - 40, (float)window()->height() / 2 + 20) * scale).floor(), floor(20.0f * scale), Color3::yellow());
			}
		}
	} rd->pop2D();

	//MIght not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}

void App::setReticle(int r) {
	m_lastReticleLoaded = m_reticleIndex = clamp(0, r, numReticles - 1);
	m_reticleTexture = Texture::fromFile(System::findDataFile(format("gui/reticle/reticle-%03d.png", m_reticleIndex)));
}

void App::setSceneBrightness(float b) {
	m_sceneBrightness = b;
}


void App::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
}

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	{
		G3DSpecification spec;
		spec.audio = playMode;
		initGLG3D(spec);
	}

	(void)argc; (void)argv;
	GApp::Settings settings(argc, argv);

	if (playMode) {
		settings.window.width = 1920; settings.window.height = 1080;
	}
	else {
		settings.window.width = 1920; settings.window.height = 980;
	}
	settings.window.fullScreen = playMode;
	settings.window.resizable = !settings.window.fullScreen;
	settings.window.asynchronous = unlockFramerate;
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



////////////////////////////////////////// experiment-related funcitons //////////////////////////
void App::initPsychophysicsLib() {
	// start cpu timer.
	StartCPUTimer();

	String datafileName = m_taskType + "_" + m_expMode + "_" + m_appendingDescription + ".db";
	m_ex->init(m_subjectID.c_str(), m_expVersion.c_str(), 0, datafileName.c_str(), m_expMode == "training");

	// required initial response to start an experiment.
	m_ex->startTimer();
	m_ex->update("Spc");

	// initializing member variables that are related to the experiment.
	m_targetHealth = 1.0f;
	m_isTrackingOn = false;
	m_reticleColor = Color3::white();

	// reset viewport to look straight ahead.
	// TODO: restore
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

void App::informTrialSuccess()
{
	m_ex->update("Spc"); // needed to stop stimulus presentation of this trial
	m_ex->update("1"); // success
}

void App::informTrialFailure()
{
	m_ex->update("Spc"); // needed to stop stimulus presentation of this trial
	m_ex->update("2"); // failure
}


// QUESTION: This might be better suited to be in Experiment classes?
