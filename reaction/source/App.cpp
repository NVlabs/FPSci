/** \file App.cpp */
#include "App.h"

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

static const bool testCustomProjection = false;

/** global startup config - sets playMode and experiment/user paths */
StartupConfig startupConfig;

App::App(const GApp::Settings& settings) : GApp(settings) {
}

void App::onInit() {
	// feed a random seed based on time.
	Random::common().reset(uint32(time(0)));

	GApp::onInit();

	// load per user setting from file
	userTable = UserTable::load(startupConfig.userConfig());
	printUserTableToLog(userTable);

	// load per experiment user settings from file
	userStatusTable = UserStatusTable::load();
	printUserStatusTableToLog(userStatusTable);

	// load experiment setting from file
	experimentConfig = ExperimentConfig::load(startupConfig.experimentConfig());
	printExpConfigToLog(experimentConfig);

	// Get and save system configuration
	SystemConfig sysConfig = SystemConfig::load();
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

	// Further evaluate where this should go!
	for (int i = 0; i < m_MatTableSize; i++) {
		Color3 color = { 1.0f - pow((float)i/ m_MatTableSize, 2.2f), pow((float)i / m_MatTableSize, 2.2f), 0.0f };
		UniversalMaterial::Specification materialSpecification;
		materialSpecification.setLambertian(Texture::Specification(color));
		materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
		materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));
		m_materials.append(UniversalMaterial::create(materialSpecification));
	}

	updateSessionDropDown();			// Update the session drop down to remove already completed sessions
	updateSessionPress();				// Update session to create results file/start collection
}

void App::printUserTableToLog(UserTable table) {
	logPrintf("Current User: %s\n", table.currentUser);
	for (UserConfig user : table.users) {
		logPrintf("\tUser ID: %s\n", user.id);
	}
}

void App::printUserStatusTableToLog(UserStatusTable table) {
	for (UserSessionStatus status : table.userInfo) {
		String sessOrder = "";
		for (String sess : status.sessionOrder) {
			sessOrder += sess + ", ";
		}
		sessOrder = sessOrder.substr(0, sessOrder.length() - 2);
		String completedSess = "";
		for (String sess : status.completedSessions) {
			completedSess += sess + ", ";
		}
		completedSess = completedSess.substr(0, completedSess.length() - 2);

		logPrintf("Subject ID: %s\nSession Order: [%s]\nCompleted Sessions: [%s]\n", status.id, sessOrder, completedSess);
	}
}

void App::printExpConfigToLog(ExperimentConfig config) {
	logPrintf("-------------------\nExperiment Config\n-------------------\nTask Type = %s\nappendingDescription = %s\nscene name = %s\nFeedback Duration = %f\nReady Duration = %f\nTask Duration = %f\n",
		config.taskType, config.appendingDescription, config.sceneName, config.feedbackDuration, config.readyDuration, config.taskDuration);
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
}

void App::openUserSettingsWindow() {
    m_userSettingsMode = true;
    m_userSettingsWindow->setVisible(m_userSettingsMode);
}


void App::makeGUI() {
	debugWindow->setVisible(!startupConfig.playMode);
	developerWindow->setVisible(!startupConfig.playMode);
	developerWindow->sceneEditorWindow->setVisible(!startupConfig.playMode);
	developerWindow->cameraControlWindow->setVisible(!startupConfig.playMode);
	developerWindow->videoRecordDialog->setEnabled(true);

	const float SLIDER_SPACING = 35;
	debugPane->beginRow(); {
		debugPane->addCheckBox("Hitscan", &m_hitScan);
		debugPane->addCheckBox("HUD", &m_renderHud);
		debugPane->addCheckBox("FPS", &m_renderFPS);
		debugPane->addCheckBox("Turbo", &m_emergencyTurbo);
		static int frames = 0;
		GuiControl* c = nullptr;

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
	a.save(startupConfig.userConfig());
	logPrintf("User table saved.\n");			// Print message to log
}

Array<String> App::updateSessionDropDown(void) {
    // Create updated session list
    String userId = userTable.getCurrentUser()->id;
    shared_ptr<UserSessionStatus> userStatus = userStatusTable.getUserStatus(userId);
    // If we have a user that doesn't have specified sessions
    if (userStatus == nullptr) {
        // Create a new user session status w/ no progress and default order (from experimentconfig.Any)
        logPrintf("User %s not found. Creating a new user w/ default session ordering.\n", userId);
        UserSessionStatus newStatus = UserSessionStatus();
        newStatus.id = userId;
        newStatus.sessionOrder = experimentConfig.getSessIds();
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

void App::updateUser(void){
	// Update the user if needed
	if (m_lastSeenUser != m_ddCurrentUser) {
		// This creates a new results file...
		if(m_sessDropDown->numElements() > 0) updateSession(updateSessionDropDown()[0]);
		String id = getDropDownUserId();
		//String filename = "../results/" + experimentConfig.taskType + "_" + id + "_" + String(Logger::genFileTimestamp()) + ".db";
		//logger->createResultsFile(filename, id);
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
}

String App::getDropDownUserId(void) {
	return m_userDropDown->get(m_ddCurrentUser);
}

void App::markSessComplete(String sessId) {
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

String App::getDropDownSessId(void) {
    if (m_sessDropDown->numElements() == 0) return "";
    return m_sessDropDown->get(m_ddCurrentSession);
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
		ex = Experiment::create(this);
		logger = Logger::create();
	}
	else {
        debugPrintf("Only reaction experiments are supported");
        exit(-1);
	}

	// Check for need to start latency logging and if so run the logger now
	SystemConfig sysConfig = SystemConfig::load();
	m_logName = "../results/" + experimentConfig.taskType + "_" + id + "_" + userTable.currentUser + "_" + String(Logger::genFileTimestamp());
	if (sysConfig.hasLogger) {
		// Handle running logger if we need to (terminate then merge results)
		if (m_loggerRunning) {
			killPythonLogger();
			pythonMergeLogs(m_logName);
		}
		// Run a new logger if we need to
		runPythonLogger(m_logName, sysConfig.loggerComPort, sysConfig.hasSync, sysConfig.syncComPort);
	}

	// Don't create a results file for a user w/ no sessions left
	if (m_sessDropDown->numElements() == 0) {
		logPrintf("No sessions remaining for selected user.\n");
	}
	// Create the results file here (but how do we make sure user set up name?)
	else {
		logger->createResultsFile(m_logName + ".db", userTable.currentUser);
		logPrintf("Created results file: %s.db\n", m_logName.c_str());
	}

	// TODO: Remove the following by invoking a call back.
	ex->onInit();
}

void App::mergeCurrentLogToCurrentDB() {
	logger->closeResultsFile();
	if (m_loggerRunning) {
		killPythonLogger();
		pythonMergeLogs(m_logName);
	}
}

void App::runPythonLogger(String logName, String com, bool hasSync, String syncComPort = "") {
	// Variables for creating process/getting handle
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Come up w/ command string
	String cmd = "python ../scripts/\"event logger\"/software/event_logger.py " + com + " \"" + logName + "\"";
	if (hasSync) cmd += " " + syncComPort;

    logPrintf("Running python command: '%s'\n", cmd.c_str());

	LPSTR command = LPSTR(cmd.c_str());
	if (!CreateProcess(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		logPrintf("Failed to start logger: %s\n", GetLastErrorString());
	}
	// Update logger management variables
	m_loggerRunning = true;
	m_loggerHandle = pi.hProcess;
}

void App::killPythonLogger() {
	if (m_loggerRunning) TerminateProcess(m_loggerHandle, 0);
	m_loggerRunning = false;
}

void App::quitRequest() {
    setExitCode(0);
	mergeCurrentLogToCurrentDB();
    //killPythonLogger();
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
		logPrintf("Failed to merge results: %s\n", GetLastErrorString());
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

	// Example GUI dynamic layout code.  Resize the debugWindow to fill
	// the screen horizontally.
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

	// Check for completed session
	if (ex->moveOn) {
		String nextSess = userStatusTable.getNextSession(userTable.currentUser);
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

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::LCTRL)) {
        Profiler::setEnabled(false);
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
        return true;
    }
	return false;
}


void App::onPostProcessHDR3DEffects(RenderDevice *rd) {
	GApp::onPostProcessHDR3DEffects(rd);

	rd->push2D(); {
		// TODO: Is this the right place to call it?
		ex->onGraphics2D(rd);

		// Target health bar (removed for now)
		//float healthBarWidth = 0.1f;
		//float healthBarHeight = 0.01f;
		//Color3 color = { 1.0f - pow(m_targetHealth, 2.2f), pow(m_targetHealth, 2.2f), 0.0f };
		//Draw::rect2D(
		//	Rect2D::xywh(
		//	(float)m_framebuffer->width() * (0.5f - healthBarWidth / 2),
		//		(float)m_framebuffer->height() * (0.5f + 0.05f),
		//		(float)m_framebuffer->width() * healthBarWidth * m_targetHealth,
		//		(float)m_framebuffer->height() * healthBarHeight
		//	), rd, color
		//);

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
		if (experimentConfig.renderClickPhoton) {
            float boxLeft = (float)m_framebuffer->width() * (1.0f - latencyRect.x);
            if (experimentConfig.clickPhotonSide != "right") {
                // swap side
                boxLeft = (float)m_framebuffer->width() * 0.0f;
            }
			Color3 cornerColor = (m_buttonUp) ? Color3::white() * 0.2f : Color3::white() * 0.8f;
			//Draw::rect2D(rd->viewport().wh() / 10.0f, rd, cornerColor);
			//Draw::rect2D(Rect2D::xywh((float)window()->width() * 0.925f, (float)window()->height() * 0.0f, (float)window()->width() * 0.075f, (float)window()->height() * 0.15f), rd, cornerColor);
			Draw::rect2D(
				Rect2D::xywh(
					boxLeft,
					(float)m_framebuffer->height() * (0.5f - latencyRect.y / 2),
					(float)m_framebuffer->width() * latencyRect.x,
					(float)m_framebuffer->height() * latencyRect.y
				), rd, cornerColor
			);
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

void App::onUserInput(UserInput* ui) {
    BEGIN_PROFILER_EVENT("onUserInput");
	static bool haveReleased = false;
	static bool fired = false;
	GApp::onUserInput(ui);
	(void)ui;

	// Require release between clicks for non-autoFire modes
	if (ui->keyReleased(GKey::LEFT_MOUSE)) {
		m_buttonUp = true;
	}

	// Handle the mouse down events
	if (ui->keyDown(GKey::LEFT_MOUSE)) {		
		haveReleased = false;					// Make it known we are no longer in released state
		m_buttonUp = false;
	}
	
	// Handle spacebar during feedback
	if (ui->keyPressed(GKey::SPACE) && (m_presentationState == PresentationState::feedback)) {
        // TODO: how do we start the experiment?
	}

	m_debugCamera->filmSettings().setSensitivity(m_sceneBrightness);
    END_PROFILER_EVENT();
}

void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
	GApp::onPose(surface, surface2D);
}

void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
 
 //   // Faster than the full stats widget
	//std::string expDebugStr = "%d fps ";
	//expDebugStr += ex->getDebugStr(); // debugging message
 //   debugFont->draw2D(rd, format(expDebugStr.c_str(), iRound(renderDevice->stats().smoothFrameRate)), Point2(10,10), 12.0f, Color3::yellow());

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

	// Might not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}

void App::setSceneBrightness(float b) {
	m_sceneBrightness = b;
}

void App::resetView() {
    // reset view direction (look front!)
    const shared_ptr<Camera>& camera = activeCamera();
    //activeCamera()->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
    // Account for the camera translation to ensure correct look vector
    //camera->lookAt(camera->frame().translation + Point3(0, 0, -1));
    const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
    //fpm->setFrame(CFrame::fromXYZYPRDegrees(0, 0, 0, 0, 0, 0));
    fpm->lookAt(Point3(0, 0, -1));
}

void App::onCleanup() {
	// Called after the application loop ends.  Place a majority of cleanup code
	// here instead of in the constructor so that exceptions can be caught.
}


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
        m_debugCamera->onPose(m_posed3D);
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
		spec.audio = startupConfig.playMode;
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

