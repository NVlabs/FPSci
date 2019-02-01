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
static const bool  playMode                   = false;


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
    showRenderingStats      = false;
    makeGUI();
    developerWindow->videoRecordDialog->setCaptureGui(false);
    m_outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
    m_hudFont = GFont::fromFile(System::findDataFile("dominant.fnt"));
    m_hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));
    m_targetModel = ArticulatedModel::create(PARSE_ANY(ArticulatedModel::Specification { 
        filename = "ifs/d12.ifs"; 
        cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings { 
            allowVertexMerging = true; 
            forceComputeNormals = false; 
            forceComputeTangents = false; 
            forceVertexMerging = true; 
            maxEdgeLength = inf; 
            maxNormalWeldAngleDegrees = 0; 
            maxSmoothAngleDegrees = 0; 
        }; 
        preprocess = preprocess{
            setMaterial(all(), UniversalMaterial::Specification { 
            emissive = Color3(0.7, 0, 0 ); 
            glossy = Color4(0.4, 0.2, 0.1, 0.8 ); 
            lambertian = Color3(1, 0.09, 0 ); 
            } ) }; 
        };));
    
    if (playMode) {
        m_fireSound = Sound::create(System::findDataFile("sound/42108__marcuslee__Laser_Wrath_6.wav"));
    }

    loadViewModel();
    setReticle(m_reticleIndex);
    loadScene("eSports Simple Hallway");

    spawnTarget(Point3(37.6184f, -0.54509f, -2.12245f), 1.0f);
    spawnTarget(Point3(39.7f, -2.3f, 2.4f), 1.0f);

    if (playMode) {
        // Force into FPS mode
        const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(cameraManipulator());
        fpm->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT);
        fpm->setMoveRate(0.0);
    }

}


shared_ptr<VisibleEntity> App::spawnTarget(const Point3& position, float scale) {
    const shared_ptr<VisibleEntity>& target = VisibleEntity::create(format("target%02d", m_targetArray.size()), scene().get(), m_targetModel, CFrame());
    const shared_ptr<Entity::Track>& track = Entity::Track::create(target.get(), scene().get(),
        Any::parse(format("combine(orbit(0, 0.1), CFrame::fromXYZYPRDegrees(%f, %f, %f))", position.x, position.y, position.z)));
    target->setTrack(track);
    m_targetArray.append(target);
    scene()->insert(target);
    return target;
}


void App::loadViewModel() {
    const static Any modelSpec = PARSE_ANY(ArticulatedModel::Specification {
        filename = "model/sniper/sniper.obj";
        preprocess = {
            transformGeometry(all(), Matrix4::yawDegrees(90));
        transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
        };
        scale = 0.25;
    });

    m_viewModel = ArticulatedModel::create(modelSpec, "viewModel");
}


void App::makeGUI() {
    debugWindow->setVisible(! playMode);
    developerWindow->setVisible(! playMode);
    developerWindow->sceneEditorWindow->setVisible(! playMode);
    developerWindow->cameraControlWindow->setVisible(! playMode);
    developerWindow->videoRecordDialog->setEnabled(true);

    debugPane->setNewChildSize(280.0f, -1.0f, 62.0f);
    debugPane->beginRow(); {
        debugPane->addCheckBox("Weapon", &m_renderViewModel);
        debugPane->addCheckBox("HUD", &m_renderHud);
        debugPane->addCheckBox("FPS", &m_renderFPS);
        debugPane->addNumberBox("Input Lag", Pointer<float>(
            [&](){ return userInput->artificialLatency() * 1000.0f; },
            [&](float ms) { userInput->setArtificialLatency(ms * 0.001f); }), "ms", GuiTheme::LINEAR_SLIDER, 0.0f, 120.0f, 0.5f);
        debugPane->addNumberBox("Reticle", &m_reticleIndex, "", GuiTheme::LINEAR_SLIDER, 0, numReticles - 1, 1)->moveBy(50, 0);
        debugPane->addNumberBox("Brightness", &m_sceneBrightness, "x", GuiTheme::LOG_SLIDER, 0.01f, 2.0f)->moveBy(50, 0);
    } debugPane->endRow();

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onAfterLoadScene(const Any& any, const String& sceneName) {
    m_debugCamera->setFieldOfView(horizontalFieldOfViewDegrees * units::degrees(), FOVDirection::HORIZONTAL);
    setSceneBrightness(m_sceneBrightness);
    setActiveCamera(m_debugCamera);
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
    

    if (playMode && ui->keyPressed(GKey::LEFT_MOUSE)) {
        // Fire

        // This effect can be modified to be a 3D sound and to track the laser itself
        m_fireSound->play();
    }

    if (m_lastReticleLoaded != m_reticleIndex) {
        // Slider was used to change the reticle
        setReticle(m_reticleIndex);
    }

    m_debugCamera->filmSettings().setSensitivity(m_sceneBrightness);

}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    if (m_renderViewModel) {
        const float yScale = -0.12f;
        const float zScale = -yScale * 0.5f;
        const float lookY = m_debugCamera->frame().lookVector().y;
        const float prevLookY = m_debugCamera->previousFrame().lookVector().y;
        const CFrame weaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
        const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
        m_viewModel->pose(surface, m_debugCamera->frame() * weaponPos, m_debugCamera->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
    }
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.

    rd->push2D(); {
        const float scale = rd->viewport().width() / 1920.0f;
        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        // Reticle
        Draw::rect2D((m_reticleTexture->rect2DBounds() * scale - m_reticleTexture->vector2Bounds() * scale / 2.0f) / 4.0f + rd->viewport().wh() / 2.0f, rd, Color3::white(), m_reticleTexture);

        if (m_renderHud) {
            const Point2 hudCenter(rd->viewport().width() / 2.0f, m_hudTexture->height() * scale * 0.48f);
            Draw::rect2D((m_hudTexture->rect2DBounds() * scale - m_hudTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), m_hudTexture);
            m_hudFont->draw2D(rd, "1:36", hudCenter - Vector2(80, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
            m_hudFont->draw2D(rd, "86%", hudCenter + Vector2(7, -1), scale * 30, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
            m_hudFont->draw2D(rd, "2080", hudCenter + Vector2(125, 0) * scale, scale * 20, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
        }

        // FPS display (faster than the full stats widget)
        if (m_renderFPS) {
            m_outputFont->draw2D(rd, format("%d measured / %d requested fps", 
                iRound(renderDevice->stats().smoothFrameRate), 
                window()->settings().refreshRate), 
                (Point2(36, 24) * scale).floor(), floor(28.0f * scale), Color3::yellow());
        }
    } rd->pop2D();

    Surface2D::sortAndRender(rd, posed2D);
}


void App::setReticle(int r) {
    m_lastReticleLoaded = m_reticleIndex = clamp(0, r, numReticles - 1);
    m_reticleTexture = Texture::fromFile(System::findDataFile(format("gui/reticle/reticle-%03d.png", m_reticleIndex)));
}


void App::setSceneBrightness(float b) {
    m_sceneBrightness = b;
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
}


// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    {
        G3DSpecification spec;
        spec.audio = playMode;
        initGLG3D(spec);
    }

    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);

    if (playMode) {
        settings.window.width       = 1920; settings.window.height      = 1080;
    } else {
        settings.window.width       = 1920; settings.window.height      = 980;
    }
    settings.window.fullScreen  = playMode;
    settings.window.resizable   = ! settings.window.fullScreen;
    settings.window.asynchronous = unlockFramerate;
    settings.window.caption = "NVIDIA Abstract FPS";
    settings.window.refreshRate = -1;
    settings.window.defaultIconFilename = "icon.png";

    settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
    settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir                       = FileSystem::currentDirectory();
    settings.screenCapture.includeAppRevision = false;
    settings.screenCapture.includeG3DRevision = false;
    settings.screenCapture.filenamePrefix = "_";

    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = false;

    return App(settings).run();
}
