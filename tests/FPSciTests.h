#pragma once

#include <memory>
#include "TestFakeInput.h"
#include <FPSciApp.h>
#include <PlayerEntity.h>
#include <Session.h>
#include <gtest/gtest.h>
#include <G3D/G3D.h>

extern std::unique_ptr<G3D::GApp::Settings> g_defaultSettings;
extern std::unique_ptr<G3D::GApp::Settings> g_settings;

class FPSciTests : public ::testing::Test {
protected:
	void SetUp()
	{
		// Catch the case when SetUpTestCase/SetUpTestSuite is silently skipped due to different googletest versions.
		assert(s_app);
	}

	// Test methods
	/** Setup the suite for all tests (error handling) */
	static void SetUpTestSuite();
	/** Setup the suite for all tests (FPSci specific) */
	static void SetUpTestSuiteSafe();
	/** Teardown the suite for all tests */
	static void TearDownTestSuite();
	/** Setup and individual test case (setup the test suite) */
	static void SetUpTestCase() { SetUpTestSuite(); };
	/** Teardown and individual test case (teardown the suite) */
	static void TearDownTestCase() { TearDownTestSuite(); };

	// Utility methods
	/** Frame period getter */
	static G3D::RealTime fixedTestDeltaTime();
	/** Player entity getter */
	static const shared_ptr<PlayerEntity> getPlayer();

	/** Run n app oneFrame() steps*/
	static inline void runAppFrames(int n);

	/** Reset the camera rotation to it's spawn position (expected to be 0 heading) */
	static void zeroCameraRotation();
	/** Rotate the camera through scaled mouse motion injection */
	static void rotateCamera(double degX, double degY);

	/** Respawn trial targets */
	static int respawnTargets();
	/** Get the current front/right targets */
	static void getTargets(shared_ptr<TargetEntity>& front, shared_ptr<TargetEntity>& right);
	/** Check whether the front/right target is still alive */
	static void checkTargets(bool& aliveFront, bool& aliveRight);
	

	// Static members
	static std::shared_ptr<FPSciApp>		s_app;						///< Running FPSciApp
	static CFrame							s_cameraSpawnFrame;			///< Original camera frame
	static std::shared_ptr<TestFakeInput>	s_fakeInput;				///< Fake input source for the app
	static float							s_targetSpawnDistance;		///< Distance at which targets are spawned from the camera
};