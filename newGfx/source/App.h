/**
  \file App.h

  The G3D 10.00 default starter app is configured for OpenGL 4.1 and
  relatively recent GPUs.
 */
#pragma once
#include <G3D/G3D.h>

class Projectile {
public:
    shared_ptr<VisibleEntity>       entity;
    /** When in hitscan mode */
    RealTime                        endTime;
    Projectile() : endTime(0) {}
    Projectile(const shared_ptr<VisibleEntity>& e, RealTime t = 0) : entity(e), endTime(t) {}
};

/** \brief Application framework. */
class App : public GApp {
protected:
    static const float TARGET_MODEL_ARRAY_SCALING;
    const int                       numReticles = 55;

    shared_ptr<GFont>               m_outputFont;
    shared_ptr<GFont>               m_hudFont;
    shared_ptr<Texture>             m_reticleTexture;
    shared_ptr<Texture>             m_hudTexture;
    shared_ptr<ArticulatedModel>    m_viewModel;
    shared_ptr<Sound>               m_fireSound;
    shared_ptr<Sound>               m_explosionSound;

    shared_ptr<ArticulatedModel>    m_laserModel;

    /** m_targetModelArray[10] is the base size. Away from that they get larger/smaller by TARGET_MODEL_ARRAY_SCALING */
    Array<shared_ptr<ArticulatedModel>>  m_targetModelArray;

    /** Array of all targets in the scene */
    Array<shared_ptr<VisibleEntity>> m_targetArray;

    Array<Projectile>               m_projectileArray;

    /** Coordinate frame of the weapon, updated in onPose() */
    CFrame                          m_weaponFrame;

    int                             m_displayLagFrames = 0;

    /** Used to detect GUI changes to m_reticleIndex */
    int                             m_lastReticleLoaded = -1;
    int                             m_reticleIndex = 0;
    float                           m_sceneBrightness = 1.0f;
    bool                            m_renderViewModel = true;
    bool                            m_renderHud = true;
    bool                            m_renderFPS = true;
    bool                            m_renderHitscan = true;

    /** Projectile if false         */
    bool                            m_hitScan = true;

    int                             m_lastUniqueID = 0;

    /** When m_displayLagFrames > 0, 3D frames are delayed in this queue */
    Array<shared_ptr<Framebuffer>>  m_ldrDelayBufferQueue;
    int                             m_currentDelayBufferIndex = 0;

    /** Called from onInit */
    void makeGUI();
    void loadModels();
    void destroyTarget(int index);

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    /** Call to change the reticle. */
    void setReticle(int r);

    /** Increment the current reticle index */
    void nextReticle() {
        setReticle((m_reticleIndex + 1) % numReticles); 
    }

    /** Creates a random target in front of the player */
    void spawnRandomTarget();

    /** Creates a spinning target */
    shared_ptr<VisibleEntity> spawnTarget(const Point3& position, float scale);

    /** Call to set the 3D scene brightness. Default is 1.0. */
    void setSceneBrightness(float b);

    void setDisplayLatencyFrames(int f);

    int displayLatencyFrames() const {
        return m_displayLagFrames;
    }

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
    virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;
    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual void onCleanup() override;
};
