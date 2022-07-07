/** \file FPSciNetworkApp.cpp */
#include "FPSciNetworkApp.h"
#include "PhysicsScene.h"
#include "WaypointManager.h"

FPSciNetworkApp::FPSciNetworkApp(const GApp::Settings& settings) : FPSciApp(settings) { }


void FPSciNetworkApp::initExperiment() {
	// Load config from files
	loadConfigs(startupConfig.experimentList[experimentIdx]);
	m_lastSavedUser = *currentUser();			// Copy over the startup user for saves

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
		FPSciApp::waypointManager = WaypointManager::create(this);
	}

	// Setup the scene
	setScene(PhysicsScene::create(m_ambientOcclusion));
	scene()->registerEntitySubclass("PlayerEntity", &PlayerEntity::create);			// Register the player entity for creation
	scene()->registerEntitySubclass("FlyingEntity", &FlyingEntity::create);			// Register the target entity for creation

	weapon = Weapon::create(&experimentConfig.weapon, scene(), activeCamera());
	weapon->setHitCallback(std::bind(&FPSciNetworkApp::hitTarget, this, std::placeholders::_1));
	weapon->setMissCallback(std::bind(&FPSciNetworkApp::missEvent, this));

	// Load models and set the reticle
	loadModels();
	setReticle(reticleConfig.index);

	// Load fonts and images
	outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
	hudTextures.set("scoreBannerBackdrop", Texture::fromFile(System::findDataFile("gui/scoreBannerBackdrop.png")));

	// Setup the GUI
	showRenderingStats = false;
	makeGUI();

	updateMouseSensitivity();				// Update (apply) mouse sensitivity
	const Array<String> sessions = m_userSettingsWindow->updateSessionDropDown();	// Update the session drop down to remove already completed sessions
	updateSession(sessions[0], true);		// Update session to create results file/start collection

	// Setup the connection to the server if this experiment is networked
		ENetAddress localAddress;
		if (false){//experimentConfig.serverAddress.c_str() != "") {
			enet_address_set_host(&localAddress, experimentConfig.serverAddress.c_str());
		}
		else {
			localAddress.host = ENET_HOST_ANY;
		}
		localAddress.port = experimentConfig.serverPort;
		m_serverHost = enet_host_create(&localAddress, 32, 2, 0, 0);
		if (m_serverHost == NULL) {
			throw std::runtime_error("Could not create a local host for the clients to connect to");
		}
        m_listenSocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM); //Create unreliable UDP socket
        enet_socket_set_option(m_listenSocket, ENET_SOCKOPT_NONBLOCK, 1); //Set socket to non-blocking
        localAddress.port += 1;
        if (enet_socket_bind(m_listenSocket, &localAddress)) {
            debugPrintf("bind failed with error: %d\n", WSAGetLastError());
			throw std::runtime_error("Could not bind to the local address");
        }
}

void FPSciNetworkApp::onNetwork() {
	

    ENetAddress addr_from;
    ENetBuffer buff;
	// TODO: make this choose the MTU better than this
    void* data = malloc(1500);  //Allocate 1 mtu worth of space for the data from the packet
    buff.data = data;
    buff.dataLength = 1500;
	int status = enet_socket_receive(m_listenSocket, &addr_from, &buff, 1);
    Array<float> movementMap;
    Array<float> mouseDeltas;
	
    if (status > 0) {
        BinaryInput input((const uint8 *)buff.data, buff.dataLength, G3D_BIG_ENDIAN, false, true);
        Array<Array<float32>> userInput;
        //G3D::deserialize(userInput, input);

        movementMap.append(input.readFloat32());
        movementMap.append(input.readFloat32());
		
        mouseDeltas.append(input.readFloat32());
        mouseDeltas.append(input.readFloat32());
        /*float value = userInput[0][0];
        value = userInput[0][1];
        value = userInput[0][2];
        value = userInput[0][3];
        value = userInput[1][0];
        value = userInput[1][1];*/

        debugPrintf("Movement Key map: [%s, %s]\n", String(std::to_string(movementMap[0])), String(std::to_string(movementMap[1])));
        debugPrintf("Mouse Deltas: [%s, %s]\n", String(std::to_string(mouseDeltas[0])), String(std::to_string(mouseDeltas[1])));

    }
    
    if (isNull(m_serverHost)) {
        return;
    }
	/*ENetEvent event;
	while (enet_host_service(m_serverHost, &event, 0) > 0) {
		char ip[16];
		enet_address_get_host_ip(&event.peer->address, ip, 16);
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			m_serverPeers.append(event.peer);
			
			logPrintf("made connection to %s in response to input\n", ip);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			logPrintf("%s disconnected.\n", ip);
			// Reset the peer's client information.
			event.peer->data = NULL;
			break;
        case ENET_EVENT_TYPE_RECEIVE:
			debugPrintf("%s\n", event.packet->data);
        }
	}
	*/
}

void FPSciNetworkApp::onInit() {
    if (enet_initialize()) {
        throw std::runtime_error("Failed to initalize enet!");
    }
	FPSciApp::onInit();
}

void FPSciNetworkApp::oneFrame() {
    // Count this frame (for shaders)
    m_frameNumber++;

    // Target frame time (only call this method once per one frame!)
    RealTime targetFrameTime = sess->targetFrameTime();

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
            // Perform wait for target time needed
            RealTime duration = targetFrameTime;
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
        BEGIN_PROFILER_EVENT("FPSciNetworkApp::onNetwork");
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
                sdt = (SimTime)targetFrameTime;
            }
            else if (sdt == REAL_TIME) {
                sdt = float(timeStep);
            }
            sdt *= m_simTimeScale;

            SimTime idt = (SimTime)targetFrameTime;

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
            RealTime duration = targetFrameTime;
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

    if (m_endProgram && window()->requiresMainLoop()) {
        window()->popLoopBody();
    }
}