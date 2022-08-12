/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include "FPSciApp.h"
#include "NoWindow.h"
#include "NetworkUtils.h"

class FPSciNetworkApp : public FPSciApp {
	
protected:
    ENetHost* m_serverHost;                              //> Host that the clients connect to
    Array <ENetPeer*> m_serverPeers;                     //> Peers that the server is connected to
    ENetSocket m_listenSocket;                           //> Socket for the server to listen on
    ENetSocket m_sendSocket;                             //> Socket for the server to send on
    Array <ENetAddress> m_connectedAddresses;            //> Addresses we have received UDP traffic from so we can broadcast worldstate updates
	Array <GUniqueID> m_connectedGUIDs;					 //> GUIDs of the connected clients
    Array <ENetPeer> m_connectedPeers;

public:
    FPSciNetworkApp(const GApp::Settings& settings);

    void onInit() override;
    void initExperiment() override;
    void onNetwork() override;
    void oneFrame() override;

};