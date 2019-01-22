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
static const bool  unlockFramerate = true;

//========================================================================
// variables related to experimental condition and record.
static const std::string weaponType = "tracking"; // hitscan or tracking
static const float targetFrameRate = 360.0f; // hz
static const std::string subjectID = "JK"; // your name
const int numFrameDelay = 0;
static const std::string expVersion = "real"; // training or real
//========================================================================

static const std::string datafile = "ver2.db";

/** Make objects fade towards black with distance as a depth cue */

static float distanceDarken(const float csZ) {
    const float t = max(0.0f, abs(csZ) - 30.0f);
    return exp(-t * 0.02f);
}

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);
    
    if (playMode) {
        settings.window.width       = 1920; settings.window.height      = 1080;
    } else {
        settings.window.width       = 1280; settings.window.height      = 720;
    }
    settings.window.fullScreen  = playMode;
    settings.window.resizable   = ! settings.window.fullScreen;
    settings.window.asynchronous = false;
    settings.window.caption = "Max Perf";
	//settings.window.refreshRate = int(targetFrameRate);
	settings.window.refreshRate = -1;
	// settings.window.fullScreenMonitorName = "Generic PnP Monitor"; // use this on Josef's machine

    //ExperimentSettingsList expList = ExperimentSettingsList(Any::fromFile(System::findDataFile("experiment1.Exp.Any")));
    //debugPrintf("Experiment: %s %lfDPI %lfcmp360 %d settings\n", expList.subjectID, expList.mouseDPI, expList.cmp360, expList.settingsList.size());

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


void App::initPsychophysicsLib() {
	// start cpu timer.
	StartCPUTimer();

	ex.setFrameRate(targetFrameRate);
	ex.setWeaponType(weaponType);
	ex.setNumFrameDelay(numFrameDelay);
	ex.init(subjectID,expVersion,0,datafile,false);

	// required initial response to start an experiment.
	ex.startTimer();
	ex.update("Spc");

	// initializing member variables that are related to the experiment.
	m_presentationState = PresentationState::ready;
	m_t_stateStart = GetCPUTime();
	m_targetHealth = 1.0f;
	m_isTrackingOn = false;
	m_reticleColor = Color3::white();

	// reset viewport to look straight ahead.
	resetView();
}

void App::onInit() {
    GApp::onInit();

	//float const dt = 1.0f / (unlockFramerate ? 2048.0f : float(window()->settings().refreshRate));
	float const dt = 1.0f / targetFrameRate;
	setFrameDuration(dt);
	//setFrameDuration(-1);
	renderDevice->setColorClearValue(Color3::white() * 0.0f);
    debugCamera()->setFrame(Point3(-5, -2, 0));
    m_debugController->setFrame(debugCamera()->frame());

    if (playMode) {
        const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
		fpm->setMoveRate(0.0);
		fpm->setTurnRate(0.0); // start with mouse motion disabled. Will turned on only during tasks.
	}

    Projection& P = debugCamera()->projection();
    P.setFarPlaneZ(-finf());

    debugWindow->setVisible(false);
    developerWindow->setVisible(false);
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;

	//activeCamera()->setFieldOfView(30.0f * pi() / 180.0f, FOVDirection::HORIZONTAL);

	initPsychophysicsLib();

   // // Append targets to target array
   // m_targetArray.append(
   //     Target(CFrame::fromXYZYPRDegrees(3, 2, -8, 0.0f, 0.0f, 0.0f),
   //         CFrame::fromXYZYPRDegrees(0.0f, 0.0f, 0.0f, 10.0f * dt, -7.0f * dt, 0),
			//1.f),

   //     Target(CFrame::fromXYZYPRDegrees(-2.0f, -0.5f, -15.0f, 40.0f, 0.0f, 10.0f),
   //         CFrame::fromXYZYPRDegrees(0.0f, 0.0f, 0.0f, -5.0f * dt, 40.0f * dt, 0.0f),
			//1.f));

    m_delayedFrames.resize(numFrameDelay + 1);
    for (int i = 0; i < numFrameDelay + 1; ++i) {
        m_delayedFrames[i] = Framebuffer::create(Texture::createEmpty(format("delayedFrame_%d", i), m_framebuffer->width(), m_framebuffer->height(), m_framebuffer->texture(0)->encoding(), m_framebuffer->texture(0)->dimension()),
            Texture::createEmpty(format("delayedFrame_%d_depth", i), m_framebuffer->width(), m_framebuffer->height(), ImageFormat::DEPTH32(), m_framebuffer->texture(0)->dimension()));
    }
}


bool App::onEvent(const GEvent& e) {
	//processUserInput(e);
	if (GApp::onEvent(e)) {
		return true;
    }
    if (e.type == GEventType::FILE_DROP) {
        Array<String> fileArray;
        window()->getDroppedFilenames(fileArray);
        if (onFileDrop(fileArray)) {
            return true;
        }
    }
	processUserInput(e);
	return false;
}


static Color3 computeTunnelColor(float alpha, float angle) {
    const float a = clamp(1.0f - abs(alpha), 0.0f, 1.0f);

    static const Color3 pink(1.0f, 0.5f, 0.5f);
    const float c = abs(wrap(0.25f + angle / (2.0f * pif()), 1.0f) - 0.5f) * 2.0f;
    const Color3 shade = Color3::cyan().lerp(pink, c);
    const float distance = max(0.0f, 100.0f * abs(alpha) - 10.0f);

    return (shade * exp(-distance * 0.1f)).pow(0.5f);
}

void App::resetView() {
	// reset view direction (look front!)
	activeCamera()->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
	//activeCamera()->lookAt(Point3(0, 0, -1));
	const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
	fpm->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
	//fpm->lookAt(Point3(0,0,-1));
}

void App::initTrialAnimation() {
	// close the app if experiment ended.
	if (ex.getFSMState() == Psychophysics::FSM::State::SHUTDOWN)
	{
		m_presentationState = PresentationState::complete; // end of experiment
		m_tunnelColor = Color3::black(); // remove tunnel
		m_reticleColor = Color3::black(); // remove reticle
	}

	// initialize target location based on the initial displacement values
	m_motionFrame = CFrame::fromXYZYPRDegrees(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	m_motionFrame.lookAt(Point3(0.0f, 0.0f, -1.0f)); // look at the -z direction
	m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::rollDegrees(ex.renderParams.initialDisplacement.x)).approxCoordinateFrame();
	m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::yawDegrees(-ex.renderParams.initialDisplacement.y)).approxCoordinateFrame();

	// Apply roll rotation by a random amount (random angle in degree from 0 to 360)
	float randomAngleDegree = Random::common().uniform() * 360;
	m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();

	// Full health for the target
	m_targetHealth = 1.f;
}

int App::getHitObject() {
	const Ray& wsRay = activeCamera()->worldRay(floor(m_framebuffer->width() / 2.0f) + 0.5f,
		floor(m_framebuffer->height() / 2.0f) + 0.5f, m_framebuffer->rect2DBounds());

	// Find first hit
	int hitIndex = -1; // return -1 if no object was hit. Otherwise, return the index of the hit object.
	float hitDistance = finf();
	for (int i = 0; i < m_targetArray.size(); ++i) {
		const Target& target = m_targetArray[i];
		float t = wsRay.intersectionTime(Sphere(target.cframe.translation, target.hitRadius));
		if (t < hitDistance) {
			hitDistance = t;
			hitIndex = i;
		}
	}
	return hitIndex;
}

void App::informTrialSuccess()
{ 
	ex.update("Spc"); // needed to stop stimulus presentation of this trial
	ex.update("1"); // success
}

void App::informTrialFailure()
{
	ex.update("Spc"); // needed to stop stimulus presentation of this trial
	ex.update("2"); // failure
}

void App::updateTrialState(double t)
{
	// This updates presentation state and also deals with data collection when each trial ends.
	// Hence the name 'updateTrialState' as opposed to 'updatePresentationState' because it is a bit more than just updating presentation state.
	PresentationState currentState = m_presentationState;
	PresentationState newState;
	double stateElapsedTime = t - m_t_stateStart;

	if (currentState == PresentationState::ready)
	{
		if (stateElapsedTime > ex.renderParams.readyDuration)
		{
			// turn on mouse interaction
            dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator())->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
            
            // G3D expects mouse sensitivity in radians
            // we're converting from mouseDPI and centimeters/360 which explains
            // the screen resolution (dots), cm->in factor (2.54) and 2PI
            double mouseSensitivity = 2.0 * pi() * 2.54 * 1920.0 / (m_cmp360 * m_mouseDPI);
			dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator())->setTurnRate(mouseSensitivity);
            ex.startTimer();

			newState = PresentationState::task;
		}
		else newState = currentState;
	}
	else if (currentState == PresentationState::task) // if tracking mode, reduce health point if shot is successful.
	{
		if (m_isTrackingOn & (m_presentationState == PresentationState::task))
		{
			int hitIndex = getHitObject();
			if (hitIndex != -1)
			{
				m_targetHealth -= ex.renderParams.weaponStrength * m_framePeriod;
			}
		}

		if ((stateElapsedTime > ex.renderParams.taskDuration) | (m_targetHealth <= 0))
		{
			newState = PresentationState::feedback;
		}
		else newState = currentState;
	}
	else if (currentState == PresentationState::feedback)
	{
		if (stateElapsedTime > ex.renderParams.feedbackDuration)
		{
			newState = PresentationState::ready;
			// turn off mouse interaction
			dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator())->setTurnRate(0.0);
			resetView();

			// Communicate with psychophysics library at this point
			if (m_targetHealth <= 0)
			{
				informTrialSuccess();
				initTrialAnimation();
			}
			else
			{
				informTrialFailure();
				initTrialAnimation();
			}
		}
		else newState = currentState;
	}
	
	if (currentState != newState)
	{ // handle state transition.
		m_t_stateStart = t;
		m_presentationState = newState;
	}
}

void App::updateAnimation()
{
	// calculate frame period based on cpu time stamps.
	double currentAnimationUpdateAt = GetCPUTime();
	m_framePeriod = (float)(currentAnimationUpdateAt - m_t_lastAnimationUpdate);

	// 1. Update presentation state and send task performance to psychophysics library.
	updateTrialState(currentAnimationUpdateAt);

	// 2. Check if motionChange is required (happens only during 'task' state with a designated level of chance).
	if (m_presentationState == PresentationState::task)
	{
		float motionChangeChancePerFrame = ex.renderParams.motionChangeChance * m_framePeriod;
		if (Random::common().uniform() < motionChangeChancePerFrame)
		{
			// If yes, rotate target coordinate frame by random (0~360) angle in roll direction
			float randomAngleDegree = Random::common().uniform() * 360;
			m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::rollDegrees(randomAngleDegree)).approxCoordinateFrame();
		}
	}

	// 3. update target location (happens only during 'task' and 'feedback' states).
	if ((m_presentationState == PresentationState::task) | (m_presentationState == PresentationState::feedback))
	{
		float rotationAngleDegree = m_framePeriod * ex.renderParams.speed;
		m_motionFrame = (m_motionFrame.toMatrix4() * Matrix4::yawDegrees(-rotationAngleDegree)).approxCoordinateFrame();
	}

	// 4. Update tunnel and target colors
	if (m_presentationState == PresentationState::ready)
	{
		// will color the tunnel when that becomes available.
	}
	else if (m_presentationState == PresentationState::task)
	{
		m_targetColor = m_targetHealth * Color3::cyan().pow(2.0f) + (1.0f - m_targetHealth) * Color3::brown().pow(2.0f);
	}
	else if (m_presentationState == PresentationState::feedback)
	{
		if (m_targetHealth > 0)
		{
			m_targetColor = Color3::red().pow(2.0f);
		}
		else
		{
			m_targetColor = Color3::green().pow(2.0f);
            // If the target is dead, empty the projectiles
            m_projectileArray.fastClear();
		}
	}

	// 5. Clear m_TargetArray. Append an object with m_targetLocation if necessary ('task' and 'feedback' states).
	Point3 t_pos = m_motionFrame.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
	m_targetArray.resize(0, false);
	if ((m_presentationState == PresentationState::task) | (m_presentationState == PresentationState::feedback))
	{
		m_targetArray.append(
			Target(CFrame::fromXYZYPRDegrees(t_pos.x, t_pos.y, t_pos.z, 0.0f, 0.0f, 0.0f),
				CFrame::fromXYZYPRDegrees(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
				1.f));
	}

	// done with timings, update the last update record.
	m_t_lastAnimationUpdate = currentAnimationUpdateAt;
}

void App::processUserInput(const GEvent& e) {
	// temporarily passing dummy responses.
	// success when mouse button is pressed, failure when keyboard button is pressed.
	// we should improve it so that it checks target hit/miss in future.

    if (e.type == GEventType::MOUSE_BUTTON_DOWN) {
		if (ex.renderParams.weaponType == "tracking")
		{
			m_isTrackingOn = true;
			m_reticleColor = Color3::red();
		}

		else if (ex.renderParams.weaponType == "hitscan")
		{
			int hitIndex = getHitObject();

			if (hitIndex != -1) {
				m_targetHealth -= ex.renderParams.weaponStrength;
			}
		}

        else if (ex.renderParams.weaponType == "projectile")
        {
            // Add a projectile. Deal with intersection in App::onSimulation;
			double cpu_now = GetCPUTime();
			if (cpu_now - m_t_lastProjectileShot > m_projectileShotPeriod)
			{
                // default projectile from center to center
                //m_projectileArray.push_back(Projectile(activeCamera()->frame().translation, (activeCamera()->frame().lookVector()) * m_projectileSpeed * m_framePeriod, m_projectileSize));
                // experimental projectile from bottom to center
                m_projectileArray.push_back(Projectile(activeCamera()->frame().translation + Point3(0.0f, -1.0f, 0.0f), (activeCamera()->frame().lookVector() + Point3(0.0f, 0.036f, 0.0f)) * m_projectileSpeed * m_framePeriod, m_projectileSize));
                m_t_lastProjectileShot = cpu_now;
			}
        }
	}

	else if (e.type == GEventType::MOUSE_BUTTON_UP) {
		if (ex.renderParams.weaponType == "tracking")
		{
			m_isTrackingOn = false;
			m_reticleColor = Color3::white();
		}
	}

	// below is useful only for debugging.
	else if (e.type == GEventType::KEY_DOWN) {
		{
			informTrialFailure();
			initTrialAnimation();
		}
	}

	// TODO after logging feature in psychophysics is ready.
	// Pass geometry setting and user input to psychophysics library for processing
	// this requires logging function implemented in the psychophysics library.
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    for (Target& target : m_targetArray) {
        target.cframe = target.cframe * target.velocity;
    }

    Array<int> projectileCullIndices;
    for (int i = 0; i < m_projectileArray.size(); ++i) {
        Projectile& projectile = m_projectileArray[i];

        const float distance = length(projectile.cframe.translation - activeCamera()->frame().translation);
        // Make projectiles disappear if they're too far away
        if (distance > (m_targetDistance + 2.0f)) {
            projectileCullIndices.insert(0, i);
        } // And intersect them with the targets if they're close enough 
        else if (distance > (m_targetDistance - 2.0f)) {
            for (Target& target : m_targetArray) {
                if (length(target.cframe.translation - projectile.cframe.translation) < (target.hitRadius + projectile.hitRadius)) {
                    m_targetHealth -= ex.renderParams.weaponStrength;
                    projectileCullIndices.insert(0, i);
                }
            }
        }

        projectile.cframe = projectile.cframe * projectile.velocity;
    }

    for (const int removeIndex : projectileCullIndices) {
        m_projectileArray.fastRemove(removeIndex);
    }
}

void App::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {

    if (numFrameDelay != 0) {
        // choose next frame

        //m_nextFrame = (m_nextFrame + 1) % (1 + numFrameDelay);
        // swap whichever frame is next
        //rd->pushState(m_delayedFrames[outputFrame]);


    }
    if (numFrameDelay != 0) {
        //const int outputFrame = (m_nextFrame + 1) % (1 + numFrameDelay);

        rd->pushState(m_delayedFrames[m_nextFrame]); {
            debugAssert(notNull(activeCamera()));
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            onGraphics3D(rd, posed3D);
        } rd->popState();
    }
    else {
        rd->pushState(); {
            debugAssert(notNull(activeCamera()));
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            onGraphics3D(rd, posed3D);
        } rd->popState();
    }

    //if (notNull(m_screenCapture)) {
    //    m_screenCapture->onAfterGraphics3D(rd);
    //}
    if (numFrameDelay != 0) {
        //const int outputFrame = (m_nextFrame + 1) % (1 + numFrameDelay);
        rd->push2D(m_delayedFrames[m_nextFrame]); {
            onGraphics2D(rd, posed2D);
        } rd->pop2D();
    }
    else {
        rd->push2D(); {
            onGraphics2D(rd, posed2D);
        } rd->pop2D();
    }


    if (numFrameDelay != 0) {
        //rd->popState();

        // blit to hardware buffer

        // backup
        // rd->push2D(m_osm_osWindowDeviceFramebuffer);
        //Draw::rect2D(rd->viewport(), rd, Color3::white(), m_delayedFrames[outputFrame]->texture(0)); // maybe invertY
        //rd->pop2D();
        //const int outputFrame = (m_nextFrame + 1) % (1 + numFrameDelay);
        static bool allFramesRenderedTo = false;
        if (allFramesRenderedTo) {
            m_nextFrame = (m_nextFrame + 1) % (numFrameDelay + 1);
        }
        m_delayedFrames[m_nextFrame]->blitTo(rd, m_osWindowDeviceFramebuffer, true); //invertY
        if (!allFramesRenderedTo) {
            m_nextFrame = (m_nextFrame + 1) % (numFrameDelay + 1);
            if (m_nextFrame == 0) {
                allFramesRenderedTo = true;
            }
        }

    }
    //if (notNull(m_screenCapture)) {
    //    m_screenCapture->onAfterGraphics2D(rd);
    //}
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {
	updateAnimation();

	////////////////////////////////////////////////////////////////////
    //                                                                //
    //                      Under construction!                       //
    //                                                                //
    //  This is actually quite slow. It is a gameplay prototype that  //
    //  will be replaced mid-November 2018 with the actual optimized  //
    //  code which produces similar visuals using optimal rendering.  //
    //                                                                //
    ////////////////////////////////////////////////////////////////////

    rd->swapBuffers();
    rd->clear();



    // Done by the caller for us:
    // rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

    // For debugging
    // Draw::axes(Point3::zero(), rd);

    static const shared_ptr<Texture> reticleTexture = Texture::fromFile("reticle.png");

        //System::findDataFile("ifs/head.ifs"), 0.01f);

    static SlowMesh tunnelMesh(PrimitiveType::LINES);
    static bool first = true;

    if (first) {
		//targetShape = std::make_shared<MeshShape>(System::findDataFile("ifs/sphere.ifs"));
		targetShape = std::make_shared<MeshShape>(System::findDataFile("ifs/d20.ifs"));
        projectileShape = std::make_shared<MeshShape>(System::findDataFile("ifs/triangle.ifs"), m_projectileSize);

        // Tunnel
        const int axisSlices = 64;
        const int cylinderSlices = 12;
        const float radius = 12.0f;
        const float extent = 250.0f;

        for (int i = 0; i < axisSlices; ++i) {
            const float alpha = 2.0f * (float(i) / float(axisSlices - 1) - 0.5f);
            const float z = alpha * extent;

            const float nextAlpha = 2.0f * (float(i + 1) / float(axisSlices - 1) - 0.5f);
            const float nextZ = nextAlpha * extent;
        
            for (int a = 0; a < cylinderSlices; ++a) {
                const float angle = 2.0f * pif() * float(a) / float(cylinderSlices);
                const float nextAngle = 2.0f * pif() * float(a + 1) / float(cylinderSlices);

                const float x = cos(angle) * radius;
                const float y = sin(angle) * radius;
                const float nextX = cos(nextAngle) * radius;
                const float nextY = sin(nextAngle) * radius;

                const Color3 color = computeTunnelColor(alpha, angle);
                const Color3 nextColor = computeTunnelColor(nextAlpha, nextAngle);

                // Circle
				tunnelMesh.setColor(color);
                tunnelMesh.makeVertex(Point3(x, y, z));
                tunnelMesh.makeVertex(Point3(nextX, nextY, z));

                // Axis
                tunnelMesh.makeVertex(Point3(x, y, z));
				tunnelMesh.setColor(nextColor);
                tunnelMesh.makeVertex(Point3(x, y, nextZ));
            }
        }
		first = false;
    }
    tunnelMesh.render(rd);


    ///////////////////////////////////////////////////////////////////////////////////////////
// Draw target object in the tunnel
    const CFrame& cameraFrame = activeCamera()->frame();
    for (const Target& target : m_targetArray) { // TODO: make sure the size of target is as defined by 'hitRadius'.
        //const Point3& csCenter = cameraFrame.pointToObjectSpace(target.cframe.translation);
		//const Color3& color = distanceDarken(csCenter.z) * m_targetColor;
		const Color3& color = m_targetColor;
		debugDraw(targetShape, 0, (color * 0.6f).pow(0.5f), color.pow(0.5f), target.cframe);
    }

    for (const Projectile& projectile : m_projectileArray)
    {
        // The velocity frame contains the look vector of the camera at the time the projectile was launched.
        // We use it as the Y-axis here to rotate the triangle (which faces +Y in its coordinate frame by
        // default) for free.
        CFrame c = CFrame::fromYAxis(-normalize(projectile.velocity.translation), projectile.cframe.translation);
        debugDraw(projectileShape, 0, Color3::red(), Color3::red(), c);
    }
    

 //   // Objects in tunnel
 //   const static RealTime startTime = System::time();
 //   const float t = float(System::time() - startTime);

	//
	//// Set the location of the target object by initializing a coordinate frame
	//m_firstObject = CFrame::fromXYZYPRDegrees(3, 2, -8, t * 10.0f, -t * 7.0f, t);
	//m_secondObject = CFrame::fromXYZYPRDegrees(-2, -0.5, -15, 40 - t * 5.0f, 20 * t * 2.0f, 10);
	//
	////Location of camera center point in world space.
	//activeCamera()->frame().translation;
	//activeCamera()->frame().rotation;
	//activeCamera()->setFrame(CFrame::fromXYZYPRDegrees(0,0,0, 0,0,0));

 //   debugDraw(targetShape, 0, (Color3::orange() * 0.1f).pow(0.5f), (Color3::orange()).pow(0.5f), m_firstObject);
 //   debugDraw(targetShape, 0, (Color3::orange() * 0.1f * 0.2f).pow(0.5f), (Color3::orange() * 0.2f).pow(0.5f), m_secondObject);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();

    rd->push2D(); {
        const float scale = rd->viewport().width() / 3840.0f;
        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Draw::rect2D(reticleTexture->rect2DBounds() * scale + (rd->viewport().wh() - reticleTexture->vector2Bounds() * scale) / 2.0f, rd, m_reticleColor, reticleTexture);
    } rd->pop2D();

    //if (numFrameDelay != 0) {
    //    // pop extra buffered frame
    //    rd->popState();
    //}
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& surface2D) {
    Surface2D::sortAndRender(rd, surface2D);

    // Faster than the full stats widget
	std::string expDebugStr = "%d fps ";
	expDebugStr += ex.getDebugStr(); // debugging message
    debugFont->draw2D(rd, format(expDebugStr.c_str(), iRound(renderDevice->stats().smoothFrameRate)), Point2(10,10), 12.0f, Color3::yellow());

    if (ex.getFSMState() == Psychophysics::FSM::State::SHUTDOWN) {
        static const shared_ptr<Texture> doneTexture = Texture::fromFile("done.png");
        rd->push2D(); {
            const float scale = rd->viewport().width() / 3840.0f;
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            Draw::rect2D(doneTexture->rect2DBounds() * scale + (rd->viewport().wh() - doneTexture->vector2Bounds() * scale) / 2.0f, rd, Color3::white(), doneTexture);
        } rd->pop2D();
    }
}


bool App::onFileDrop(const Array<String>& fileArray) {
    if (ex.getFSMState() == Psychophysics::FSM::State::SHUTDOWN) {
        bool launchedOne = false;
        for (const String& filename : fileArray) {
            if (endsWith(filename, ".Exp.Any")) {
                // load the json
                m_experimentSettingsList = ExperimentSettingsList(Any::fromFile(filename));
                launchedOne = true;
            }
        }

        // TODO: start the experiment

        if (launchedOne) {
            return true;
        }
    }

    return false;
}
