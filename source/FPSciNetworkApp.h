/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include <G3D/G3D.h>


class FPSciNetworkApp : public GApp {

public:
    FPSciNetworkApp(const GApp::Settings& settings) : GApp(settings, (OSWindow*)NoWindow::create(), nullptr, false) { }

};



class NoWindow : public OSWindow {
public:
    NoWindow(const OSWindow::Settings& settings) : OSWindow() {}

    static NoWindow* create(const Settings& s = Settings());
};