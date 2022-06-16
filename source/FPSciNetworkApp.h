/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include "FPSciApp.h"
#include "NoWindow.h"


class FPSciNetworkApp : public FPSciApp {

public:
    FPSciNetworkApp(const GApp::Settings& settings) : FPSciApp(settings, (OSWindow*)NoWindow::create(), nullptr, true) { }

};