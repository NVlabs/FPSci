/** \file FPSciNetworkApp.cpp */
#include "FPSciNetworkApp.h"


NoWindow* NoWindow::create(const Settings& s = Settings()) {
    NoWindow* window = new NoWindow(settings);
    return window;
}