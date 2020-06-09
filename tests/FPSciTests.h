#pragma once

#include <memory>

// Can't forward declare GApp::Settings
#include <G3D-app/GApp.h>

extern std::unique_ptr<G3D::GApp::Settings> g_defaultSettings;
extern std::unique_ptr<G3D::GApp::Settings> g_settings;
