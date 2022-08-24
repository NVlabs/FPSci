#include "FPSciTests.h"

std::unique_ptr<GApp::Settings> g_defaultSettings;
std::unique_ptr<GApp::Settings> g_settings;

// Static storage
std::shared_ptr<FPSciApp>		FPSciTests::s_app;
CFrame							FPSciTests::s_cameraSpawnFrame;
std::shared_ptr<TestFakeInput>	FPSciTests::s_fakeInput;

float							FPSciTests::s_targetSpawnDistance = 0.5f;

// Most basic smoke test - launch the app with the default config. Covers frequent exceptions thrown.
// TODO: Disabled because G3D has trouble running twice
TEST(DefaultConfigTests, DISABLED_RenderTenFrames)
{
	if (!g_defaultSettings) {
		//GTEST_SKIP();
		assert(!"not implemented: skip the test (GTEST_SKIP undefined)");
		return;
	}

	FPSciApp app(*g_defaultSettings);
	app.onInit();

	// Render 10 frames and check the player object was loaded
	for (int i = 0; i < 10; ++i)
		app.oneFrame();
	auto player = app.scene()->typedEntity<PlayerEntity>("player");
	EXPECT_NE(nullptr, player);
	
	app.quitRequest();
}

void FPSciTests::runAppFrames(int n) {
	for (int i = 0; i < n; ++i) {
		s_app->oneFrame();
	}
}

void FPSciTests::SetUpTestSuite() {
	try {
		// Googletest catches ALL exceptions here. This can be a nightmare to debug if it goes unnoticed.
		SetUpTestSuiteSafe();
	} catch (std::exception e) {
		printf("Exception caught during SetUpTestSuiteSafe: %s\n", e.what());
		assert(!"exception thrown");
		throw;
	}
	catch (ParseError e) {
		printf("Exception caught during SetUpTestSuiteSafe: %s\n", e.message.c_str());
		assert(!"exception thrown");
		throw;
	} catch (String e) {
		printf("Exception caught during SetUpTestSuiteSafe: %s\n", e.c_str());
		assert(!"exception thrown");
		throw;
	} catch (const char* e) {
		printf("Exception caught during SetUpTestSuiteSafe: %s\n", e);
		assert(!"exception thrown");
		throw;
	} catch (...) {
		printf("Exception caught during SetUpTestSuiteSafe: (unknown)\n");
		assert(!"exception thrown");
		throw;
	}
}
void FPSciTests::SetUpTestSuiteSafe() {
	// Load a testing specific config file
	FPSciApp::startupConfig = Any::fromFile("test/startupconfig.Any");

	g_settings->window.width = (int)FPSciApp::startupConfig.windowSize.x;
	g_settings->window.height = (int)FPSciApp::startupConfig.windowSize.y;
	g_settings->window.fullScreen = FPSciApp::startupConfig.fullscreen;
	g_settings->window.resizable = !g_settings->window.fullScreen;

	s_app = std::make_shared<FPSciApp>(*g_settings);

	s_app->onInit();
	s_app->closeUserSettingsWindow();

	// Make frames advance with a constant timestep. This is necessary for movement
	s_app->setFrameDuration(s_app->frameDuration(), GApp::MATCH_REAL_TIME_TARGET);

	// Set up per-frame fake input
	s_fakeInput = std::make_shared<TestFakeInput>(s_app, s_app->currentUser()->mouseDPI);
	s_fakeInput->defocusOriginalWindow();
	s_app->addWidget(s_fakeInput);

	// Prime the app and load the scene
	s_app->oneFrame();
	s_cameraSpawnFrame = s_app->activeCamera()->frame();

	assert(s_app->sess->currentState == PresentationState::initial);

	// Fire to make the red target appear
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	assert(s_app->sess->currentState == PresentationState::trialFeedback);

	System::sleep(0.5f);
	s_app->oneFrame();

	// Press shift to become ready
	// If this fails, future g_app->sess->presentationState checks will fail
	s_fakeInput->window().injectReady();
	s_app->oneFrame();

	// Wait for the ready timer
	// TODO: it sounds like this should be needed before injectReady() and not after as well, but it often fails without.
	System::sleep(0.5f);
	s_app->oneFrame();
}

void FPSciTests::SelectSession(const String& sessionId) {
	s_app->updateSession(sessionId);
	s_fakeInput->defocusOriginalWindow();

	// Fire to make the red target appear
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	assert(s_app->sess->currentState == PresentationState::trialFeedback);

	System::sleep(0.5f);
	s_app->oneFrame();

	// Press shift to become ready
	// If this fails, future g_app->sess->presentationState checks will fail
	s_fakeInput->window().injectReady();
	s_app->oneFrame();

	// Wait for the ready timer
	// TODO: it sounds like this should be needed before injectReady() and not after as well, but it often fails without.
	System::sleep(0.5f);
	s_app->oneFrame();
}

void FPSciTests::TearDownTestSuite() {
	s_app->quitRequest();
	s_app.reset();
}

G3D::RealTime FPSciTests::fixedTestDeltaTime()
{
	return s_app->simulationTimeScale() * s_app->frameDuration();
}

inline const shared_ptr<PlayerEntity> FPSciTests::getPlayer()
{
	auto player = s_app->scene()->typedEntity<PlayerEntity>("player");
	EXPECT_NE(nullptr, player);
	return player;
}

void FPSciTests::zeroCameraRotation()
{
	auto player = getPlayer();
	EXPECT_TRUE(notNull(player));
	if (player) {
		player->respawn();
	}
	EXPECT_EQ(player->heading(), 0.f);
}

int FPSciTests::respawnTargets()
{
	s_app->sess->clearTargets();
	s_app->sess->initTargetAnimation();

	// TODO: investigate why it takes a frame for targets to be placed. Without this, the transform below is clobbered.
	s_app->oneFrame();

	// Make all target positions relative to the player spawn
	for (auto& t : s_app->sess->targetArray()) {
		auto cf = t->frame();
		cf.translation = s_cameraSpawnFrame.translation + cf.translation * s_targetSpawnDistance;
		t->setFrame(cf);
	}

	s_app->oneFrame();
	return s_app->sess->targetArray().size();
}

void FPSciTests::rotateCamera(double degX, double degY)
{
	const double degPerMm = s_app->currentUser()->mouseDegPerMm;
	s_fakeInput->window().injectMove(0.001 * degX / degPerMm, 0.001 * degY / degPerMm);
}

void FPSciTests::getTargets(shared_ptr<TargetEntity>& front, shared_ptr<TargetEntity>& right)
{
	front.reset();
	right.reset();
	for (auto& target : s_app->sess->targetArray()) {
		Vector3 direction = target->frame().translation - s_cameraSpawnFrame.translation;

		// A target off the same Y plane as the camera is kept alive to avoid ending the session
		if (abs(direction.y) > 0.1)
			continue;

		double bearing = toDegrees(atan2(direction.x, -direction.z));

		if (G3D::abs(bearing) < 1.0) {
			front = target;
		}

		if (G3D::abs(bearing - 30.0) < 1.0) {
			right = target;
		}
	}
}

void FPSciTests::checkTargets(bool& aliveFront, bool& aliveRight)
{
	shared_ptr<TargetEntity> front;
	shared_ptr<TargetEntity> right;
	getTargets(front, right);
	aliveFront = (bool)front;
	aliveRight = (bool)right;
}

TEST_F(FPSciTests, InitialTestConditions) {
	// The app must have loaded a session
	ASSERT_TRUE(notNull(s_app->sess));

	// Make sure a player exists
	auto player = getPlayer();
	ASSERT_TRUE(notNull(player));

	// Default config properties
	EXPECT_FALSE(s_app->sessConfig->weapon.autoFire);
}

TEST_F(FPSciTests, UsingFixedTimestep) {
	EXPECT_EQ(s_app->simStepDuration(), GApp::MATCH_REAL_TIME_TARGET);
	const double e = 0.00001;
	SimTime t0 = s_app->simTime();
	s_app->oneFrame();
	SimTime t1 = s_app->simTime();
	s_app->oneFrame();
	SimTime t2 = s_app->simTime();
	EXPECT_NEAR(t1 - t0, t2 - t1, e);
}

TEST_F(FPSciTests, TestTargetPositions) {
	respawnTargets();

	// A common issue is targets spawning in the wrong spot or the wrong session/experiment configs being loaded.
	EXPECT_EQ(s_app->sess->targetArray().size(), 3);
	if (s_app->sess->targetArray().size() < 3) {
		return;
	}

	auto player = getPlayer();
	ASSERT_TRUE(notNull(player));

	auto camPos = s_app->activeCamera()->frame().translation;

	float e = 0.01f;
	auto front = s_app->sess->targetArray()[1]->frame().translation;
	auto right = s_app->sess->targetArray()[2]->frame().translation;
	EXPECT_NEAR((front.x - camPos.x) / s_targetSpawnDistance, 0.0f, e);
	EXPECT_NEAR((front.y - camPos.y) / s_targetSpawnDistance, 0.0f, e);
	EXPECT_NEAR((front.z - camPos.z) / s_targetSpawnDistance, -2.0f, e);
	EXPECT_NEAR((right.x - camPos.x) / s_targetSpawnDistance, 1.0f, e);
	EXPECT_NEAR((right.y - camPos.y) / s_targetSpawnDistance, 0.0f, e);
	EXPECT_NEAR((right.z - camPos.z) / s_targetSpawnDistance, -1.73205f, e);
}

TEST_F(FPSciTests, CanDetectWhichTargets) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);

	respawnTargets();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_TRUE(aliveFront);
	EXPECT_TRUE(aliveRight);
}

TEST_F(FPSciTests, KillTargetFront) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);

	int spawnedTargets = respawnTargets();

	// Kill the front target - just fire
	zeroCameraRotation();
	s_fakeInput->window().injectFire();
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_FALSE(aliveFront) << "Front target should not remain (should have been destroyed)!";
	EXPECT_TRUE(aliveRight) << "Right target should remain (should not have been destroyed)!";
	EXPECT_EQ(s_app->sess->targetArray().size(), spawnedTargets-1) << format("We shot once (and hit a target). There should be %d targets left.", spawnedTargets-1).c_str();
}

TEST_F(FPSciTests, KillTargetFrontHoldclick) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);
	
	int spawnedTargets = respawnTargets();
	
	zeroCameraRotation();
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_FALSE(aliveFront) << "Front target should not remain (should have been destroyed)!";
	EXPECT_TRUE(aliveRight) << "Right target should remain (should not have been destroyed)!";
	EXPECT_EQ(s_app->sess->targetArray().size(), spawnedTargets - 1) << format("We shot once (and hit a target). There should be %d targets left.", spawnedTargets - 1).c_str();

	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
}

TEST_F(FPSciTests, KillTargetRightRotate) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);

	int spawnedTargets = respawnTargets();

	// Kill the right target by rotating to line it up
	zeroCameraRotation();
	rotateCamera(30.0, 0);

	runAppFrames(10);

	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_TRUE(aliveFront) << "Front target should remain (should not have been destroyed)!";
	EXPECT_FALSE(aliveRight) << "Right target should not remain (should have been destroyed)!";
	EXPECT_EQ(s_app->sess->targetArray().size(), spawnedTargets - 1) << format("We shot once (and hit a target). There should be %d targets left.", spawnedTargets - 1).c_str();
}

TEST_F(FPSciTests, KillTargetRightTranslate) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);
	
	int spawnedTargets = respawnTargets();

	zeroCameraRotation();
	auto player = getPlayer();
	ASSERT_TRUE(notNull(player));

	// Kill the right target by moving to line it up
	const float moveX = 0.5f;
	*player->moveRate = (float)(moveX / fixedTestDeltaTime());
	*player->moveScale = Vector2::one();
	s_fakeInput->window().injectKeyDown(GKey('d'));
	s_app->oneFrame();
	s_fakeInput->window().injectKeyUp(GKey('d'));
	s_app->oneFrame();

	// Fire with mouse click
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_EQ(s_app->simStepDuration(), GApp::MATCH_REAL_TIME_TARGET);
	EXPECT_TRUE(aliveFront) << "Front target should remain (should not have been destroyed)!";
	EXPECT_FALSE(aliveRight) << "Right target should not remain (should have been destroyed)!";
	EXPECT_EQ(s_app->sess->targetArray().size(), spawnedTargets - 1) << format("We shot once (and hit a target). There should be %d targets left.", spawnedTargets - 1).c_str();
}

TEST_F(FPSciTests, ResetCamera) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);

	// Move the camera off zero
	zeroCameraRotation();
	rotateCamera(12.3456, 34.5678);
	s_app->oneFrame();
	Vector3 lookDir = s_app->activeCamera()->frame().lookRay().direction();
	EXPECT_NE(lookDir.x, 0.0);
	EXPECT_NE(lookDir.y, 0.0);
	EXPECT_NE(lookDir.z, -1.0);

	// Zero the camera
	zeroCameraRotation();
	s_app->oneFrame();
	lookDir = s_app->activeCamera()->frame().lookRay().direction();
	EXPECT_DOUBLE_EQ(lookDir.x, 0.0);
	EXPECT_DOUBLE_EQ(lookDir.y, 0.0);
	EXPECT_DOUBLE_EQ(lookDir.z, -1.0);
}

TEST_F(FPSciTests, RotateCamera) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);
	zeroCameraRotation();
	s_app->oneFrame();

	// TODO: an objectionally large error value is used because the app seems significantly out by some small amount
	float e = 0.0001f;

	float rot = 0.0f;
	const float rotDelta = 30.0f;

	// Look around in a circle, checking the direction vector
	for (int i = 0; i <= 360 / 30; ++i) {
		rot += rotDelta;
		rotateCamera(rotDelta, 0);
		s_app->oneFrame();

		Vector3 lookDir = s_app->activeCamera()->frame().lookRay().direction();
		Vector3 expectLookDir = Vector3(sin(toRadians(rot)), 0.0, -cos(toRadians(rot)));
		EXPECT_NEAR(lookDir.x, expectLookDir.x, e);
		EXPECT_NEAR(lookDir.y, expectLookDir.y, e);
		EXPECT_NEAR(lookDir.z, expectLookDir.z, e);
	}
}

TEST_F(FPSciTests, MoveCamera) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);
	zeroCameraRotation();
	s_app->oneFrame();
	EXPECT_EQ(s_app->simStepDuration(), GApp::MATCH_REAL_TIME_TARGET);

	// TODO: reduce surprisingly large epsilon value.
	float e = 0.2f;

	Vector3 startPos = s_app->activeCamera()->frame().translation;

	auto player = getPlayer();
	ASSERT_TRUE(notNull(player));
	*player->moveRate = (float)(1.0 / fixedTestDeltaTime());
	*player->moveScale = Vector2::one();

	// Turn to the right
	float rot = 30.0f;
	rotateCamera(rot, 0);

	// Move right for a while
	s_fakeInput->window().injectKeyDown(GKey('d'));
	for (int i = 0; i <= 10; ++i) {
		s_app->oneFrame();
		EXPECT_EQ(s_app->simStepDuration(), GApp::MATCH_REAL_TIME_TARGET);

		Vector3 position = s_app->activeCamera()->frame().translation - startPos;
		Vector3 expectPosition = Vector3(cos(toRadians(rot)), 0.0, sin(toRadians(rot))) * ((float)i + 1.0f);

		// Currently the z axis is locked
		expectPosition.z = 0.0f;

		EXPECT_NEAR(position.x, expectPosition.x, e);
		EXPECT_NEAR(position.y, expectPosition.y, e);
		EXPECT_NEAR(position.z, expectPosition.z, e);
	}

	s_fakeInput->window().injectKeyUp(GKey('d'));
	s_app->oneFrame();
}

TEST_F(FPSciTests, TestAutoFire) {
	EXPECT_EQ(s_app->sess->currentState, PresentationState::trialTask);
	respawnTargets();
	zeroCameraRotation();

	const int frames = 3;
	const float damagePerFrame = 0.1f;
	const float firePeriod = (float)fixedTestDeltaTime() - 0.001f;

	s_app->sessConfig->weapon.autoFire = true;
	s_app->sessConfig->weapon.damagePerSecond = damagePerFrame / firePeriod;
	s_app->sessConfig->weapon.firePeriod = firePeriod;

	s_app->oneFrame();

	s_fakeInput->window().injectMouseDown(0);
	runAppFrames(frames);
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	bool frontAlive = false;
	for (auto& target : s_app->sess->targetArray()) {
		Vector3 direction = target->frame().translation - s_cameraSpawnFrame.translation;
		double bearing = toDegrees(atan2(direction.x, -direction.z));
		if (G3D::abs(direction.y) < 0.1 && G3D::abs(bearing) < 1.0) {
			EXPECT_NEAR(target->health(), 1.0f - frames * damagePerFrame, 0.001f);
			frontAlive = true;
		}
	}
	EXPECT_TRUE(frontAlive) << "Low damage-per-second with auto-fire but target still died";

	s_app->sessConfig->weapon.autoFire = false;
}


TEST_F(FPSciTests, TestTargetSizes) {
	SelectSession("sizes");

	// % error for size comparisons
	float e = 0.035f;

	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();

	ASSERT_EQ(spawnedtargets, 5);
	auto smallArr = s_app->sess->targetArray()[0];
	auto medium = s_app->sess->targetArray()[1];
	auto large = s_app->sess->targetArray()[2];
	auto toosmall = s_app->sess->targetArray()[3];
	auto toolarge = s_app->sess->targetArray()[4];

	// Expect to match the values written in the config
	auto smallid = smallArr->id();
	EXPECT_TRUE(smallid.compare("smallArr") == 0);
	auto smallsize = smallArr->size();
	float targetsmallsize = 0.05f;
	EXPECT_NEAR(smallsize, targetsmallsize, targetsmallsize * e);

	auto mediumid = medium->id();
	EXPECT_TRUE(mediumid.compare("medium") == 0);
	auto mediumsize = medium->size();
	float targetmediumsize = 0.1f;
	EXPECT_NEAR(mediumsize, targetmediumsize, targetmediumsize * e);

	auto largeid = large->id();
	EXPECT_TRUE(largeid.compare("large") == 0);
	auto largesize = large->size();
	float targetlargesize = 2.0f;
	EXPECT_NEAR(largesize, targetlargesize, targetlargesize * e);

	auto toosmallid = toosmall->id();
	EXPECT_TRUE(toosmallid.compare("toosmall") == 0);
	auto toosmallsize = toosmall->size();
	// smallest target is 0.1 mm
	float targettoosmallsize = 0.0001f;
	EXPECT_NEAR(toosmallsize, targettoosmallsize, targettoosmallsize * e);

	auto toolargeid = toolarge->id();
	EXPECT_TRUE(toolargeid.compare("toolarge") == 0);
	auto toolargesize = toolarge ->size();
	// largest target is 8 m
	float targettoolargesize = 8.0f;
	EXPECT_NEAR(toolargesize, targettoolargesize, targettoolargesize * e);
}


TEST_F(FPSciTests, TestCameraSelection) {

	SelectSession("defaultCamera");
	s_app->oneFrame();
	EXPECT_TRUE(s_app->playerCamera->name() == "camera") << "Camera name was " << s_app->playerCamera->name().c_str();

	SelectSession("customCamera");
	s_app->oneFrame();
	EXPECT_TRUE(s_app->playerCamera->name() == "customCamera") << "Camera name was " << s_app->playerCamera->name().c_str();

}


TEST_F(FPSciTests, TestWeapon60HzContinuous) {
	// Should be 1 second to kill
	SelectSession("60HzContinuous");
	EXPECT_TRUE(s_app->weapon->config()->isContinuous()) << "Weapon should be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.034) << "Failed to be within 2 frame periods of the expected end time!";
	ASSERT_NEAR(numFrames, 60, 2) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon30HzContinuous) {
	// Should be 1 second to kill
	SelectSession("30HzContinuous");
	EXPECT_TRUE(s_app->weapon->config()->isContinuous()) << "Weapon should be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.067) << "Failed to be within 2 frames of the expected end time!";
	ASSERT_NEAR(numFrames, 30, 1) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon30Hz67ms) {
	// Should be 1 second to kill
	SelectSession("30Hz67ms");
	EXPECT_FALSE(s_app->weapon->config()->isContinuous()) << "Weapon should NOT be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.067) << "Failed to be within a firePeriod of the expected end time!";
	ASSERT_NEAR(numFrames, 30, 67/33) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon60Hz150ms) {
	// Should be 1 second to kill
	SelectSession("60Hz150ms");
	EXPECT_FALSE(s_app->weapon->config()->isContinuous()) << "Weapon should NOT be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.150) << "Failed to be within a firePeriod of the expected end time!";
	ASSERT_NEAR(numFrames, 60, 150/17) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon60Hz67ms) {
	// Should be 1 second to kill
	SelectSession("60Hz67ms");
	EXPECT_FALSE(s_app->weapon->config()->isContinuous()) << "Weapon should NOT be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.067) << "Failed to be within a firePeriod of the expected end time!";
	ASSERT_NEAR(numFrames, 60, 67/17) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon60Hz33ms) {
	// Should be 1 second to kill
	SelectSession("60Hz33ms");
	EXPECT_FALSE(s_app->weapon->config()->isContinuous()) << "Weapon should NOT be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.034) << "Failed to be within a firePeriod of the expected end time!";
	ASSERT_NEAR(numFrames, 60, 34/17) << "Wrong number of frames taken.";
}

TEST_F(FPSciTests, TestWeapon60Hz16ms) {
	// Should be 1 second to kill
	SelectSession("60Hz16ms");
	EXPECT_FALSE(s_app->weapon->config()->isContinuous()) << "Weapon should NOT be continuous fire!";
	s_app->oneFrame();
	auto spawnedtargets = respawnTargets();
	ASSERT_EQ(spawnedtargets, 1);
	shared_ptr<TargetEntity> target = s_app->sess->targetArray()[0];
	s_fakeInput->window().injectMouseDown(0);
	int numFrames = 0;
	RealTime start = System::time();
	while (target->health() > 0.f && System::time() - start < 2.f) {
		s_app->oneFrame();
		numFrames++;
	}
	RealTime end = System::time();
	EXPECT_LE(target->health(), 0.f) << "Target should be destroyed!";
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
	EXPECT_NEAR(end - start, 1, 0.017) << "Failed to be within a frame of the expected end time!";
	ASSERT_NEAR(numFrames, 60, 1) << "Wrong number of frames taken.";
}

void checkColor(Color4 col, Color4 targetCol, float abs_error, String str) {
	EXPECT_NEAR(col.r, targetCol.r, abs_error) << "Red didn't match for target " << str.c_str();
	EXPECT_NEAR(col.g, targetCol.g, abs_error) << "Green didn't match for target " << str.c_str();
	EXPECT_NEAR(col.b, targetCol.b, abs_error) << "Blue didn't match for target " << str.c_str();
}

TEST_F(FPSciTests, TestTargetColors) {
	// Should be 1 second to kill
	SelectSession("TargetColors");
	Color4 col;
	for (auto matkv : s_app->materials) {
		//// This is the code that prints the test cases that follow below
		//if (matkv.key == "1color" || matkv.key == "2colors" || matkv.key == "3colors" || matkv.key == "4colors" || matkv.key == "5colors" || matkv.key == "7colors") {
		//	printf("if (matkv.key == \"%s\") {\n", matkv.key.c_str());
		//	printf("printf(\"Testing materials for %%s.\n\", matkv.key.c_str());\n");
		//	int i = 0;
		//	for (auto mat : matkv.value) {
		//		Color4 col = mat->bsdf()->lambertian().mean();
		//		printf("\tcheckColor(Color4(%.4ff, %.4ff, %.4ff, 1.0), matkv.value[%d]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + \" Color[%d]\");\n", col.r, col.g, col.b, i, i++);
		//	}
		//	printf("}\n");
		//}

		//// This version prints the colors generated for visual debugging
		//printf("Colors for %s.\n", matkv.key.c_str());
		//for (auto mat : matkv.value) {
		//	Color4 col = mat->bsdf()->lambertian().mean();
		//	printf("\t(%.2f, %.2f, %.2f)\n", col.r, col.g, col.b );
		//}

		if (matkv.key == "1color") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
		if (matkv.key == "2colors") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(1.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.9167f, 0.0833f, 0.0000f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(0.8333f, 0.1667f, 0.0000f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(0.7500f, 0.2500f, 0.0000f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(0.6667f, 0.3333f, 0.0000f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.5833f, 0.4167f, 0.0000f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(0.5000f, 0.5000f, 0.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.4167f, 0.5833f, 0.0000f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(0.3333f, 0.6667f, 0.0000f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.2500f, 0.7500f, 0.0000f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.1667f, 0.8333f, 0.0000f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0833f, 0.9167f, 0.0000f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
		if (matkv.key == "3colors") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(1.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.8333f, 0.1667f, 0.0000f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(0.6667f, 0.3333f, 0.0000f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(0.5000f, 0.5000f, 0.0000f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(0.3333f, 0.6667f, 0.0000f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.1667f, 0.8333f, 0.0000f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(0.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.0000f, 0.8333f, 0.1667f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(0.0000f, 0.6667f, 0.3333f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.0000f, 0.5000f, 0.5000f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.0000f, 0.3333f, 0.6667f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0000f, 0.1667f, 0.8333f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
		if (matkv.key == "4colors") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(0.0000f, 1.0000f, 1.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.2500f, 0.7500f, 0.7500f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(0.5000f, 0.5000f, 0.5000f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(0.7500f, 0.2500f, 0.2500f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(1.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.7500f, 0.2500f, 0.0000f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(0.5000f, 0.5000f, 0.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.2500f, 0.7500f, 0.0000f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(0.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.0000f, 0.7500f, 0.2500f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.0000f, 0.5000f, 0.5000f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0000f, 0.2500f, 0.7500f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
		if (matkv.key == "5colors") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(1.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.6667f, 1.0000f, 0.3333f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(0.3333f, 1.0000f, 0.6667f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(0.0000f, 1.0000f, 1.0000f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(0.3333f, 0.6667f, 0.6667f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.6667f, 0.3333f, 0.3333f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(1.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.6667f, 0.3333f, 0.0000f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(0.3333f, 0.6667f, 0.0000f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.0000f, 0.6667f, 0.3333f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0000f, 0.3333f, 0.6667f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
		if (matkv.key == "7colors") {
			printf("Testing materials for %s.\n", matkv.key.c_str());
			checkColor(Color4(0.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[0]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[0]");
			checkColor(Color4(0.5000f, 0.0000f, 0.5000f, 1.0), matkv.value[1]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[1]");
			checkColor(Color4(1.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[2]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[2]");
			checkColor(Color4(1.0000f, 0.5000f, 0.5000f, 1.0), matkv.value[3]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[3]");
			checkColor(Color4(1.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[4]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[4]");
			checkColor(Color4(0.5000f, 1.0000f, 0.5000f, 1.0), matkv.value[5]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[5]");
			checkColor(Color4(0.0000f, 1.0000f, 1.0000f, 1.0), matkv.value[6]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[6]");
			checkColor(Color4(0.5000f, 0.5000f, 0.5000f, 1.0), matkv.value[7]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[7]");
			checkColor(Color4(1.0000f, 0.0000f, 0.0000f, 1.0), matkv.value[8]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[8]");
			checkColor(Color4(0.5000f, 0.5000f, 0.0000f, 1.0), matkv.value[9]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[9]");
			checkColor(Color4(0.0000f, 1.0000f, 0.0000f, 1.0), matkv.value[10]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[10]");
			checkColor(Color4(0.0000f, 0.5000f, 0.5000f, 1.0), matkv.value[11]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[11]");
			checkColor(Color4(0.0000f, 0.0000f, 1.0000f, 1.0), matkv.value[12]->bsdf()->lambertian().mean(), 0.0001f, matkv.key + " Color[12]");
		}
	}
}


TEST_F(FPSciTests, TestFreshStart) {
	// Make sure autogenereated config files aren't present from previous runs
	bool failDelete = false;
	failDelete = remove("test/emptyconfig.Any");
	EXPECT_TRUE(failDelete) << "Experiment Config present from previous run!";
	failDelete = remove("test/emptyuser.Any");
	EXPECT_TRUE(failDelete) << "User Config present from previous run!";
	failDelete = remove("test/emptystatus.Any");
	EXPECT_TRUE(failDelete) << "User Status present from previous run!";
	failDelete = remove("test/emptystatus.sessions.csv");
	EXPECT_TRUE(failDelete) << "User Status sessions present from previous run!";
	failDelete = remove("test/emptykeymap.Any");
	EXPECT_TRUE(failDelete) << "Keymap present from previous run!";
	failDelete = remove("test/emptysystem.Any");
	EXPECT_TRUE(failDelete) << "System Config present from previous run!";
	// Load `freshStartExperiment` (at index 1)
	s_app->experimentIdx = 1;
	EXPECT_NO_THROW(
		s_app->initExperiment();
	) << "Failed to initialize with autogenerated config";
	// Run one frame (to make sure there's no crash)
	EXPECT_NO_FATAL_FAILURE(
		s_app->oneFrame();
	) << "Failed to run one frame with autogenerated config";
	// Delete the generated files, error if any were not generated
	failDelete = remove("test/emptyconfig.Any");
	EXPECT_FALSE(failDelete) << "Experiment Config not generated!";
	failDelete = remove("test/emptyuser.Any");
	EXPECT_FALSE(failDelete) << "User Config not generated!";
	failDelete = remove("test/emptystatus.Any");
	EXPECT_FALSE(failDelete) << "User Status not generated!";
	failDelete = remove("test/emptykeymap.Any");
	EXPECT_FALSE(failDelete) << "Keymap not generated!";
	failDelete = remove("test/emptysystem.Any");
	EXPECT_FALSE(failDelete) << "System Config not generated!";

	// The sessions csv doesn't generate until some progress happens. 
	// We switch experiments here to force that to happen.
	// Load `testExperiment` (at index 0)
	s_app->experimentIdx = 0;
	EXPECT_NO_THROW(
		s_app->initExperiment();
	) << "Failed to initialize with test config";
	// Run one frame (to make sure there's no crash)
	EXPECT_NO_FATAL_FAILURE(
		s_app->oneFrame();
	) << "Failed to run one frame with test config";
	// Delete the sessions csv
	failDelete = remove("test/emptystatus.sessions.csv");
	EXPECT_FALSE(failDelete) << "User Status sessions csv not generated!";
}