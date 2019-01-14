/**
  \file maxPerf/App.h

  Sample application showing how to render simple graphics with maximum throughput and 
  minimum latency by stripping away most high level VFX and convenience features for
  development. This approach is good for some display and perception research. For general
  game and rendering applications, look at the G3D starter app and vrStarter which give very
  performance with a lot of high-level game engine features.

 */
#pragma once
#include <G3D/G3D.h>
#include "experiment.h"
#include "ExperimentSettingsList.h"

// An enum that tracks presentation state within a trial. Duration defined in experiment.h
// ready: ready scene that happens before beginning of a task.
// task: actual task (e.g. instant hit, tracking, projectile, ...)
// feedback: feedback showing whether task performance was successful or not.
enum PresentationState { ready, task, feedback , complete };

class App : public GApp {

protected:

    class Target {
    public:
        float                       hitRadius;

        CFrame                      cframe;

        /** Transform cframe by this every frame. It is linear and
            angular velocity in object space per frame (not per second) */
        CFrame                      velocity;

        Target() {}
        Target(const CFrame& f, const CFrame& v, const float& s) : cframe(f), velocity(v), hitRadius(s) {}
    };
    typedef Target Projectile;

    const float                     m_targetDistance = 30.0f;
    const float                     m_projectileSpeed = 150.0f; // meters per second
	const float                     m_projectileShotPeriod = 0.3f; // minimum time between two repeated shots
    const float                     m_projectileSize = 0.5f;
    Array<Target>                   m_targetArray;
    Array<Projectile>               m_projectileArray;
	
    shared_ptr<Shape> targetShape;
    shared_ptr<Shape> projectileShape;
	CFrame m_firstTarget;
	CFrame m_secondTarget;

    // Extra framebuffers for adding delay
    int m_nextFrame = 0;
    Array<shared_ptr<Framebuffer>> m_delayedFrames;

	/** Parameters related to animation during a trial. */
	// Animation flow.
	// updateAnimation() is called at the beginning of onGraphics3D. Workflow in updateLocation()
	//       1. Check if trial time passed m_trialParam.trialDuration.
	//                 If yes, end the current trial and initialize the next trial.
	//       2. Check if motionChange is required.
	//                 If yes, update m_rotationAxis. This is chosen among the vectors orthogonal to [camera - target] line.
	//       3. update target location.
	//       4. append to m_TargetArray an object with m_targetLocation.

	// hardware setting
	struct ScreenSetting
	{
		float viewingDistance = 0.5f; // in m
		float screenDiagonal = 25.0f * 0.0254f; // in m (diagonal)
		Vector2 resolution = Vector2(1920, 1080);
		float pixelSize = screenDiagonal / sqrt(resolution.x * resolution.x + resolution.y * resolution.y);
		Vector2 screenSize = resolution * pixelSize;
	} m_screenSetting;
	// Parameters constant during a trial.
	// Parameters that can change during a trial.
	CFrame                          m_motionFrame; // object at 10 m away in -z direction in this coordinate frame
	// Timestamp of previous animation to get as close as possible to world clock timing.
	double                          m_t_lastAnimationUpdate;
	double                          m_t_stateStart;
	double                          m_t_lastProjectileShot = - inf();
	enum PresentationState          m_presentationState; // which sequence are we in?
	float                           m_targetHealth; // 1 if never hit, 0 if hit. Binary for instant hit weapon, but tracking weapon will continuously reduce it.
	float                           m_framePeriod;
	Color3                          m_tunnelColor;
	Color3                          m_targetColor;
	Color3                          m_reticleColor;
	bool                            m_isTrackingOn; // true if down AND weapon type is tracking, false otherwise.
    double                          m_mouseDPI = 2400.0; // normal mice are 800.0. Gaming mice go up to 12000.0. Josef's G502 is set to 2400.0
    double                          m_cmp360 = 12.75; // Joohwan set this to ~12.75, Josef prefers ~9.25
    ExperimentSettingsList          m_experimentSettingsList;

    virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
    bool onFileDrop(const Array<String>& fileArray);

public:
	Psychophysics::EccentricityExperiment ex;
    
    App(const GApp::Settings& settings = GApp::Settings());

	void initTrialAnimation();
	void resetView();
	void processUserInput(const GEvent& e);
	void initPsychophysicsLib();

    virtual void onInit() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onGraphics3D(RenderDevice* rd, Array< shared_ptr<Surface> >& surface) override;
    virtual void onGraphics2D(RenderDevice* rd, Array< shared_ptr<Surface2D> >& surface2D) override;
    virtual bool onEvent(const GEvent& e) override;
	int getHitObject();
	void updateAnimation();
	void informTrialSuccess();
	void informTrialFailure();
	void updateTrialState(double t);
};
