/** \file FPSciNetworkApp.cpp */
#include "FPSciNetworkApp.h"
#include "PhysicsScene.h"
#include "WaypointManager.h"
#include <Windows.h>
#include "NetworkUtils.h"

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

	m_serverSocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    enet_socket_set_option(m_serverSocket, ENET_SOCKOPT_NONBLOCK, 1); //Set socket to non-blocking
    localAddress.port += 1;
    if (enet_socket_bind(m_serverSocket, &localAddress)) {
        debugPrintf("bind failed with error: %d\n", WSAGetLastError());
		throw std::runtime_error("Could not bind to the local address");
    }

    debugPrintf("Began listening\n");
}

void FPSciNetworkApp::onNetwork() {
	

    ENetAddress addr_from;
    ENetBuffer buff;
	// TODO: make this choose the MTU better than this
    void* data = malloc(1500);  //Allocate 1 mtu worth of space for the data from the packet
    buff.data = data;
    buff.dataLength = 1500;
	//int status = enet_socket_receive(m_listenSocket, &addr_from, &buff, 1);
    //Array<float> movementMap;
    //Array<float> mouseDeltas;
    BinaryOutput entity_updates;
    entity_updates.setEndian(G3D_BIG_ENDIAN);
    entity_updates.writeUInt8(BATCH_ENTITY_UPDATE);
    entity_updates.writeUInt8(0); // init to 0 updates
    uint8 num_entity_updates = 0;
	
    while (enet_socket_receive(m_serverSocket, &addr_from, &buff, 1)) { //while there are packets to receive
        logPrintf("Recieved a packet...\n");
        char ip[16];
        enet_address_get_host_ip(&addr_from, ip, 16);
        BinaryInput packet_contents((const uint8 *)buff.data, buff.dataLength, G3D_BIG_ENDIAN, false, true);
        CoordinateFrame frame;
        messageType type = (messageType)packet_contents.readUInt8();
        
        if (type == HANDSHAKE) {
            debugPrintf("Replying to handshake...\n");
            BinaryOutput bitstring;
            bitstring.setEndian(G3D_BIG_ENDIAN);
            bitstring.writeUInt8(HANDSHAKE_REPLY);
            ENetBuffer buff;
            buff.data = (void*)bitstring.getCArray();
            buff.dataLength = bitstring.length();
            debugPrintf("%i, %i\n", addr_from.host, addr_from.port);
            if (enet_socket_send(m_serverSocket, &addr_from, &buff, 1) <= 0) {
                debugPrintf("Failed to send reply...\n");
            };
        }
        else if (type == UPDATE_MESSAGE) {
            frame.deserialize(packet_contents);
            //movementMap.append(input.readFloat32());
            //movementMap.append(input.readFloat32());

            //mouseDeltas.append(input.readFloat32());
            //mouseDeltas.append(input.readFloat32());

            SYSTEMTIME now;
            GetSystemTime(&now);
            debugPrintf("\n%02d.%03d:\tPacket from %s:%u\n", now.wSecond, now.wMilliseconds, ip, addr_from.port);
            debugPrintf("Player is at: %s\n",  frame.toXYZYPRDegreesString());
            //debugPrintf("Movement Key map: [%s, %s]\n", String(std::to_string(movementMap[0])), String(std::to_string(movementMap[1])));
            //debugPrintf("Mouse Deltas: [%s, %s]\n", String(std::to_string(mouseDeltas[0])), String(std::to_string(mouseDeltas[1])));
            Array<shared_ptr<Entity>> entityArray;
            scene()->getTypedEntityArray<Entity>(entityArray);
            for (int i = 0; i < entityArray.size(); i++)
            {
                if (entityArray[i]->name() == "player") {
                    shared_ptr<Entity> player = entityArray[i];
                    player->setFrame(frame);
                }
            }
        }
        else if (type == BATCH_ENTITY_UPDATE) {
            //update locally: 
            int num_packet_members = packet_contents.readUInt8(); // get # of frames in this packet
            for (int i = 0; i < num_packet_members; i++) { // get new frames and update objects
                //GUniqueID entity_id = updated_objects.at(i);
                GUniqueID entity_id;
                packet_contents.readBytes(&entity_id, sizeof(entity_id));
                if (entity_id != m_playerGUID) { // don't let the server move this client
                    shared_ptr<NetworkedEntity> e = (*scene()).typedEntity<NetworkedEntity>(entity_id.toString16());
                    if (&e != nullptr) {
                        NetworkUtils::updateEntity(e, packet_contents);
                    }
                }
            }/*
            std::vector<GUniqueID> updated_objects = {};
            for (int i = 0; i < num_packet_members; i++) { // get IDs for each update object from first half of packet
                GUniqueID id;
                packet_contents.readBytes(&id, sizeof(id));
                updated_objects.push_back(id);
            }
            for (int i = 0; i < num_packet_members; i++) { // get new frames and update objects
                GUniqueID entity_id = updated_objects.at(i);
                CoordinateFrame frame;
                frame.deserialize(packet_contents);
                logPrintf("Received frame: %s\n", frame.toXYZYPRDegreesString());
                //shared_ptr<NetworkedEntity> e = (*scene()).typedEntity<NetworkedEntity>(entity_id.toString16());
                shared_ptr<Entity> e = (*scene()).entity(entity_id.toString16());
                if (e != nullptr) {
                    e->setFrame(frame);
                    logPrintf("Update player.\n");
                }
                //global_entities.get(entity_id)->setFrame(frame); // need to figure out how entities are stored in a larger context
            }
            */
            // here we in theory copy this update information into the entity updates buffer so that we can send it later.

            num_entity_updates += num_packet_members;
            packet_contents.setPosition(BATCH_UPDATE_COUNT_POSITION + 1);
            int num_bits = (packet_contents.getLength() - (BATCH_UPDATE_COUNT_POSITION + 1)) * 8;
            packet_contents.beginBits();
            entity_updates.beginBits();
            entity_updates.writeBits(packet_contents.readBits(num_bits), num_bits);
            packet_contents.endBits();
            entity_updates.endBits();
            /*
            // forward message:
            // Send the serialized frame to the server
            for (int i = 0; i < m_connectedAddresses.length(); i++) {
                ENetAddress toAddress = m_connectedAddresses[i];
                if (toAddress.host != addr_from.host) { // don't send back to the sender
                    if (enet_socket_send(m_serverSocket, &m_unreliableServerAddress, &buff, 1) <= 0) {
                        logPrintf("Failed to send a packet to the server\n");
                    }
                }
            }*/
        }

    }
    free(data);
	
	ENetEvent event;
	while (enet_host_service(m_serverHost, &event, 0) > 0) {
        debugPrintf("Processing Packet.... %i\n", event.type);
		char ip[16];
		enet_address_get_host_ip(&event.peer->address, ip, 16);
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
            debugPrintf("connection recieved...\n");
			m_serverPeers.append(event.peer);
			logPrintf("made connection to %s in response to input\n", ip);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
            debugPrintf("disconnection recieved...\n");
			logPrintf("%s disconnected.\n", ip);
			// Reset the peer's client information.
            for (int i = 0; i < m_connectedPeers.size(); i++) {
                if (m_connectedPeers[i].address.host == event.peer->address.host) {
                    m_connectedAddresses.remove(i, 1);
                    m_connectedGUIDs.remove(i, 1);
                    m_connectedPeers.remove(i, 1);
                    // TODO TELL OTHER CLIENTS THAT THIS CLIENT DC'D
                }
            }
			event.peer->data = NULL;
			break;
        case ENET_EVENT_TYPE_RECEIVE:
			debugPrintf("recieved packet\n");

            BinaryInput packet_contents(event.packet->data, event.packet->dataLength, G3D_BIG_ENDIAN);
            messageType type = (messageType)packet_contents.readUInt8();

            if (type == REGISTER_CLIENT) {
                debugPrintf("Registering client...\n");
                m_connectedPeers.append(*event.peer);
                GUniqueID clientGUID;
                clientGUID.deserialize(packet_contents);
                m_connectedGUIDs.append(clientGUID);

                ENetAddress addr = event.peer->address;
                addr.port = packet_contents.readUInt16();
                m_connectedAddresses.append(addr);
                debugPrintf("\tPort: %i\n", addr.port);
                debugPrintf("\tHost: %i\n", addr.host);
                //debugPrintf("Connected to client %s at address %s:%u\n", clientGUID, ip, addr_from.port);
                // create reply
                BinaryOutput bitstring;
                bitstring.setEndian(G3D_BIG_ENDIAN);
                bitstring.writeUInt8(CLIENT_REGISTRATION_REPLY);
                clientGUID.serialize(bitstring);		// Send the GUID as a byte string to the client in confirmation
                bitstring.writeUInt8(0);
                ENetBuffer buff;
                buff.data = (void*)bitstring.getCArray();
                buff.dataLength = bitstring.length();
                ENetPacket* replyPacket = enet_packet_create((void*)bitstring.getCArray(), bitstring.length(), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, replyPacket);

                debugPrintf("\tRegistered client: %s\n", clientGUID.toString16());

                Any modelSpec = PARSE_ANY(ArticulatedModel::Specification{			///< Basic model spec for target
                    filename = "model/target/low_poly_sphere_no_outline.obj";
                    cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings{
                    allowVertexMerging = true;
                    forceComputeNormals = false;
                    forceComputeTangents = false;
                    forceVertexMerging = true;
                    maxEdgeLength = inf;
                    maxNormalWeldAngleDegrees = 0;
                    maxSmoothAngleDegrees = 0;
                    };
                    });
                shared_ptr<Model> model = ArticulatedModel::create(modelSpec);

                const shared_ptr<NetworkedEntity>& target = NetworkedEntity::create(clientGUID.toString16(), &(*scene()), model, CFrame());
                //target->setFrame(position);
                target->setWorldSpace(true);
                //target->setHitSound(config->hitSound, m_app->soundTable, config->hitSoundVol);
                //target->setDestoyedSound(config->destroyedSound, m_app->soundTable, config->destroyedSoundVol);

                target->setColor(G3D::Color3(20.0, 20.0, 200.0));

                (*scene()).insert(target);
                // Add the target to the scene/target array
                //insertTarget(target);
                //return target;

                //ADD THIS CLIENT TO OTHER CLIENTS, ADD OTHER CLIENTS TO THIS
                BinaryOutput forwardingbitstring;
                forwardingbitstring.setEndian(G3D_BIG_ENDIAN);
                forwardingbitstring.writeUInt8(CREATE_ENTITY);
                m_playerGUID.serialize(forwardingbitstring);		// Send the GUID as a byte string to the server so it can identify the client
                ENetPacket* forwardingPacket = enet_packet_create((void*)forwardingbitstring.getCArray(), forwardingbitstring.length(), ENET_PACKET_FLAG_RELIABLE);

                for (int i = 0; i < m_connectedPeers.length(); i++) {
                    ENetAddress addr = m_connectedPeers[i].address;
                    if (addr.host != event.peer->address.host) {
                        // update the other peer with new connection
                        enet_peer_send(&m_connectedPeers[i], 0, forwardingPacket);

                        // update the new connection with other peer
                        BinaryOutput addExistingbitstring;
                        addExistingbitstring.setEndian(G3D_BIG_ENDIAN);
                        addExistingbitstring.writeUInt8(CREATE_ENTITY);
                        m_connectedGUIDs[i].serialize(addExistingbitstring);
                        ENetPacket* addExistingPacket = enet_packet_create((void*)addExistingbitstring.getCArray(), addExistingbitstring.length(), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, addExistingPacket);
                        
                    }
                }
            }
            enet_packet_destroy(event.packet);
            break;
        }
	}

    // and now we send data out:
    /*
    if (num_entity_updates > 0) {
        entity_updates.setPosition(BATCH_UPDATE_COUNT_POSITION);
        entity_updates.writeUInt8(num_entity_updates);
        ENetBuffer enet_buff;
        enet_buff.data = (void*)entity_updates.getCArray();
        enet_buff.dataLength = entity_updates.length();

        for (int i = 0; i < m_connectedAddresses.length(); i++) {
            ENetAddress toAddress = m_connectedAddresses[i];
            if (toAddress.host != addr_from.host) { // don't send back to the sender
                if (enet_socket_send(m_serverSocket, &m_unreliableServerAddress, &enet_buff, 1) <= 0) {
                    logPrintf("Failed to send a packet to the server\n");
                }
            }
        }
    }*/
    //
    BinaryOutput output;
    output.setEndian(G3D_BIG_ENDIAN);
    output.writeUInt8(BATCH_ENTITY_UPDATE);
    Array<shared_ptr<NetworkedEntity>> entityArray;
    scene()->getTypedEntityArray<NetworkedEntity>(entityArray);
    output.writeUInt8(entityArray.size());
    for (int i = 0; i < entityArray.size(); i++)
    {
        shared_ptr<NetworkedEntity> e = entityArray[i];
        GUniqueID guid = GUniqueID::fromString16((*e).name().c_str());
        NetworkUtils::createFrameUpdate(guid, e, output);
    }

    ENetBuffer enet_buff;
    enet_buff.data = (void*)output.getCArray();
    enet_buff.dataLength = output.length();
    for (int i = 0; i < m_connectedAddresses.length(); i++) {
        ENetAddress toAddress = m_connectedAddresses[i];
        if (enet_socket_send(m_serverSocket, &toAddress, &enet_buff, 1) <= 0) {
            logPrintf("Failed to send a packet to the client\n");
        }
    }
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

    // Pose
    BEGIN_PROFILER_EVENT("Pose");
    m_poseWatch.tick();
    {
        m_posed3D.fastClear();
        m_posed2D.fastClear();
        onPose(m_posed3D, m_posed2D);

        // The debug camera is not in the scene, so we have
        // to explicitly pose it. This actually does nothing, but
        // it allows us to trigger the TAA code.
        playerCamera->onPose(m_posed3D);
    }
    m_poseWatch.tock();
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
	
    // Graphics
    debugAssertGLOk();
    if ((submitToDisplayMode() == SubmitToDisplayMode::BALANCE) && (!renderDevice->swapBuffersAutomatically()))
    {
        swapBuffers();
    }

    if (notNull(m_gazeTracker))
    {
        BEGIN_PROFILER_EVENT("Gaze Tracker");
        sampleGazeTrackerData();
        END_PROFILER_EVENT();
    }

    BEGIN_PROFILER_EVENT("Graphics");
    renderDevice->beginFrame();
    m_widgetManager->onBeforeGraphics();
    m_graphicsWatch.tick();
    {
        debugAssertGLOk();
        renderDevice->pushState();
        {
            debugAssertGLOk();
            onGraphics(renderDevice, m_posed3D, m_posed2D);
        }
        renderDevice->popState();
    }
    m_graphicsWatch.tock();
    renderDevice->endFrame();
    if ((submitToDisplayMode() == SubmitToDisplayMode::MINIMIZE_LATENCY) && (!renderDevice->swapBuffersAutomatically()))
    {
        swapBuffers();
    }
    END_PROFILER_EVENT();

    // Remove all expired debug shapes
    for (int i = 0; i < debugShapeArray.size(); ++i)
    {
        if (debugShapeArray[i].endTime <= m_now)
        {
            debugShapeArray.fastRemove(i);
            --i;
        }
    }

    for (int i = 0; i < debugLabelArray.size(); ++i)
    {
        if (debugLabelArray[i].endTime <= m_now)
        {
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