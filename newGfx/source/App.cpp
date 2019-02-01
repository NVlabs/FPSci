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

////////////////////////////////////////////////////////////////////////////////////////////

const float App::TARGET_MODEL_ARRAY_SCALING = 0.1f;

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
    developerWindow->videoRecordDialog->setCaptureGui(true);
    m_outputFont = GFont::fromFile(System::findDataFile("arial.fnt"));
    m_hudFont = GFont::fromFile(System::findDataFile("dominant.fnt"));
    m_hudTexture = Texture::fromFile(System::findDataFile("gui/hud.png"));
        
    if (playMode) {
        m_fireSound = Sound::create(System::findDataFile("sound/42108__marcuslee__Laser_Wrath_6.wav"));
        m_explosionSound = Sound::create(System::findDataFile("sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav"));
    }

    loadModels();
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
    const int scaleIndex = clamp(iRound(log(scale) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + 10), 0, m_targetModelArray.length() - 1);    
    
    const shared_ptr<VisibleEntity>& target = VisibleEntity::create(format("target%03d", ++m_lastSpawnedTargetIndex), scene().get(), m_targetModelArray[scaleIndex], CFrame());
    const shared_ptr<Entity::Track>& track = Entity::Track::create(target.get(), scene().get(),
        Any::parse(format("combine(orbit(0, 0.1), CFrame::fromXYZYPRDegrees(%f, %f, %f))", position.x, position.y, position.z)));
    target->setTrack(track);
    target->setShouldBeSaved(false);
    m_targetArray.append(target);
    scene()->insert(target);
    return target;
}


void App::loadModels() {
    const static Any modelSpec = PARSE_ANY(ArticulatedModel::Specification {
        filename = "model/sniper/sniper.obj";
        preprocess = {
            transformGeometry(all(), Matrix4::yawDegrees(90));
        transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
        };
        scale = 0.25;
    });

    m_viewModel = ArticulatedModel::create(modelSpec, "viewModel");

    const static Any laserSpec = PARSE_ANY(ArticulatedModel::Specification {
        filename = "ifs/d10.ifs";
        preprocess = {
            transformGeometry(all(), Matrix4::pitchDegrees(90));
            transformGeometry(all(), Matrix4::scale(0.05,0.05,2));
            setMaterial(all(), UniversalMaterial::Specification {
                lambertian = Color3(0);
            emissive = Power3(5,4,0);
            });
        };});

    m_laserModel = ArticulatedModel::create(laserSpec, "laserModel");

    for (int i = 0; i <= 20; ++i) {
        const float scale = pow(1.0f + TARGET_MODEL_ARRAY_SCALING, float(i) - 10.0f);
        m_targetModelArray.push(ArticulatedModel::create(Any::parse(format(STR(ArticulatedModel::Specification { 
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
            scale = %f;
            preprocess = preprocess{
                setMaterial(all(), UniversalMaterial::Specification { 
                emissive = Color3(0.7, 0, 0 ); 
                glossy = Color4(0.4, 0.2, 0.1, 0.8 ); 
                lambertian = Color3(1, 0.09, 0 ); 
                } ) }; 
            };), scale))));
    }
}


void App::makeGUI() {
    debugWindow->setVisible(! playMode);
    developerWindow->setVisible(! playMode);
    developerWindow->sceneEditorWindow->setVisible(! playMode);
    developerWindow->cameraControlWindow->setVisible(! playMode);
    developerWindow->videoRecordDialog->setEnabled(true);

    debugPane->setNewChildSize(280.0f, -1.0f, 62.0f);
    debugPane->beginRow(); {
        debugPane->addCheckBox("Hitscan", &m_hitScan);
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
        Point3 aimPoint = m_debugCamera->frame().translation + m_debugCamera->frame().lookVector() * 1000.0f;

        if (m_hitScan) {
            const Ray& ray = m_debugCamera->frame().lookRay();

            float closest = finf();
            int closestIndex = -1;
            for (int t = 0; t < m_targetArray.size(); ++t) {
                if (m_targetArray[t]->intersect(ray, closest)) {
                    closestIndex = t;
                }
            }

            if (closestIndex >= 0) {
                destroyTarget(closestIndex);
                aimPoint = ray.origin() + ray.direction() * closest;
            }
        }

        // Create the laser
        if (false) {
            CFrame laserStartFrame = m_weaponFrame;
            laserStartFrame.translation += laserStartFrame.upVector() * 0.1f;
            
            // Adjust for the discrepancy between where the gun is and where the player is looking
            laserStartFrame.lookAt(aimPoint);

            laserStartFrame.translation += laserStartFrame.lookVector() * 2.0f; 
            const shared_ptr<VisibleEntity>& laser = VisibleEntity::create(format("laser%03d", ++m_lastSpawnedTargetIndex), scene().get(), m_laserModel, CFrame());
            laser->setShouldBeSaved(false);
            laser->setCanCauseCollisions(false);
            laser->setCastsShadows(false);
            const shared_ptr<Entity::Track>& track = Entity::Track::create(laser.get(), scene().get(),
                Any::parse(format("%s", laserStartFrame.toXYZYPRDegreesString().c_str())));
            laser->setTrack(track);
            scene()->insert(laser);
        }

        m_fireSound->play(m_debugCamera->frame().translation, m_debugCamera->frame().lookVector() * 2.0f, 3.0f);
    }

    if (m_lastReticleLoaded != m_reticleIndex) {
        // Slider was used to change the reticle
        setReticle(m_reticleIndex);
    }

    m_debugCamera->filmSettings().setSensitivity(m_sceneBrightness);

}


void App::destroyTarget(int index) {
    // Not a reference because we're about to manipulate the array
    const shared_ptr<VisibleEntity> target = m_targetArray[index];
    m_targetArray.fastRemove(index);

    scene()->removeEntity(target->name());

    if (playMode) {
        // 3D audio
        m_explosionSound->play(target->frame().translation, Vector3::zero(), 16.0f);
    }
}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    if (m_renderViewModel) {
        const float yScale = -0.12f;
        const float zScale = -yScale * 0.5f;
        const float lookY = m_debugCamera->frame().lookVector().y;
        const float prevLookY = m_debugCamera->previousFrame().lookVector().y;
        m_weaponFrame = m_debugCamera->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
        const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
        m_viewModel->pose(surface, m_weaponFrame, m_debugCamera->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
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
