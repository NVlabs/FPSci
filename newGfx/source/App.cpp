/** \file App.cpp */
#include "App.h"


/** Set this variable to a value in frames per second (Hz) to lock a specific rate.
    
    Set your monitor's desktop refresh rate (e.g., in the NVIDIA Control Panel)
    to the highest rate that it supports before running this program.
   */
static const float targetFrameRate            = 1000; // Hz

/** Enable this to see maximum CPU/GPU rate when not limited by the monitor. */
static const bool  unlockFramerate            = true;

/* Set to true if the monitor has G-SYNC/Adaptive VSync/FreeSync, 
   which allows the application to submit asynchronously with vsync
   without tearing. */
static const bool  variableRefreshRate        = true;

static const float horizontalFieldOfViewDegrees = 90; // deg

/** Set to false when debugging */
static const bool  playMode                   = true;



App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();

    float dt = 0;

    if (unlockFramerate) {
        // Set a maximum *finite* frame rate
        dt = 1.0f / 8192.0f;
    } else if (variableRefreshRate) {
        dt = 1.0f / targetFrameRate;
    } else {
        dt = 1.0f / float(window()->settings().refreshRate);
    }
    setFrameDuration(dt);
    setSubmitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);    
    showRenderingStats      = true;
    makeGUI();
    developerWindow->videoRecordDialog->setCaptureGui(false);

    loadScene("Test");
}


void App::makeGUI() {
    debugWindow->setVisible(false);
    developerWindow->videoRecordDialog->setEnabled(true);
    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onAfterLoadScene(const Any& any, const String& sceneName) {
    m_debugCamera->setFieldOfView(horizontalFieldOfViewDegrees * units::degrees(), FOVDirection::HORIZONTAL);
}


void App::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) { ... return true; }

    return false;
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
}



// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    initGLG3D(G3DSpecification());

    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);

    if (playMode) {
        settings.window.width       = 1920; settings.window.height      = 1080;
    } else {
        settings.window.width       = 1280; settings.window.height      = 720;
    }
    settings.window.fullScreen  = playMode;
    settings.window.resizable   = ! settings.window.fullScreen;
    settings.window.asynchronous = unlockFramerate;
    settings.window.caption = "Max Perf";
    settings.window.refreshRate = -1;
    settings.window.defaultIconFilename = "icon.png";

    settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
    settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
    settings.screenCapture.outputDirectory = FileSystem::currentDirectory();
    settings.screenCapture.includeAppRevision = false;
    settings.screenCapture.includeG3DRevision = false;
    settings.screenCapture.filenamePrefix = "_";

    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = false;

    return App(settings).run();
}
