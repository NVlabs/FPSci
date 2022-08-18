/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include "FPSciApp.h"
#include "NoWindow.h"
#include "NetworkUtils.h"

class FPSciServerApp : public FPSciApp {
	
protected:
	// Struct containing all the data needed to keep track of and comunicate with clients
    struct ConnectedClient {
        ENetPeer peer;
        GUniqueID guid;
        ENetAddress unreliableAddress;
    };
	
    ENetHost* m_serverHost;                              //> Host that the clients connect to
    ENetSocket m_listenSocket;                           //> Socket for the server to listen on
    ENetSocket m_sendSocket;                             //> Socket for the server to send on TODO: remove this and just send on the listen socket

    Array <ConnectedClient> m_connectedClients;          //> List of all connected clients and all atributes needed to comunicate with them
	
    

public:
    FPSciServerApp(const GApp::Settings& settings);

    void onInit() override;
    void initExperiment() override;
    void onNetwork() override;
    void oneFrame() override;

};