/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include "FPSciApp.h"
#include "NoWindow.h"


class FPSciNetworkApp : public FPSciApp {
	
protected:
    ENetHost* m_serverHost;                              //> Host that the clients connect to
    Array <ENetPeer*> m_serverPeers;                     //> Peers that the server is connected to

public:
    FPSciNetworkApp(const GApp::Settings& settings);

    void onInit() override;
    void initExperiment() override;
    void onNetwork() override;
    void oneFrame() override;

};