
#include "FPSciTests.h"
#include "TestFakeInput.h"
#include <FPSciApp.h>
#include <PlayerEntity.h>
#include <Session.h>
#include <gtest/gtest.h>

using namespace G3D;

std::unique_ptr<GApp::Settings> g_defaultSettings;
std::unique_ptr<GApp::Settings> g_settings;

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

class FPSciTests : public ::testing::Test {
protected:
	void SetUp()
	{
		// Catch the case when SetUpTestCase/SetUpTestSuite is silently skipped due to different googletest versions.
		assert(s_app);
	}

	static void SetUpTestSuite();
	static void SetUpTestSuiteSafe();
	static void TearDownTestSuite();
	static void SetUpTestCase() { SetUpTestSuite(); };
	static void TearDownTestCase() { TearDownTestSuite(); };
	static G3D::RealTime fixedTestDeltaTime();
	static const shared_ptr<PlayerEntity> getPlayer();
	static void zeroCameraRotation();
	static void respawnTargets();
	static void rotateCamera(double degX, double degY);
	static void getTargets(shared_ptr<TargetEntity>& front, shared_ptr<TargetEntity>& right);
	static void checkTargets(bool& aliveFront, bool& aliveRight);

	static std::shared_ptr<FPSciApp>				s_app;
	static CFrame							s_cameraSpawnFrame;
	static std::shared_ptr<TestFakeInput>	s_fakeInput;
	static float							s_targetSpawnDistance;
};

std::shared_ptr<FPSciApp>			FPSciTests::s_app;
CFrame							FPSciTests::s_cameraSpawnFrame;
std::shared_ptr<TestFakeInput>	FPSciTests::s_fakeInput;
float							FPSciTests::s_targetSpawnDistance = 3.0f;

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
	s_app->addWidget(s_fakeInput);

	// Prime the app and load the scene
	s_app->oneFrame();
	s_app->oneFrame();
	s_cameraSpawnFrame = s_app->activeCamera()->frame();

	assert(s_app->sess->presentationState == PresentationState::initial);

	// Fire to make the red target appear
	// TODO: there is an issue where the app misses the event entirely if it's down and up in the same frame
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();
	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();

	assert(s_app->sess->presentationState == PresentationState::feedback);

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
}

void FPSciTests::respawnTargets()
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
}

void FPSciTests::rotateCamera(double degX, double degY)
{
	double metersPer360 = s_app->currentUser()->cmp360 / 100.0;
	s_fakeInput->window().injectMove(metersPer360 * degX / 360.0, metersPer360 * degY / 360.0);
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
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);

	respawnTargets();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_TRUE(aliveFront);
	EXPECT_TRUE(aliveRight);
}

TEST_F(FPSciTests, KillTargetFront) {
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);

	respawnTargets();

	// Kill the front target - just fire
	zeroCameraRotation();
	s_fakeInput->window().injectFire();
	s_app->oneFrame();
	s_app->oneFrame();
	s_app->oneFrame();
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_EQ(s_app->sess->targetArray().size(), 2) << "We shot once. There should be two targets left.";
	EXPECT_FALSE(aliveFront);
	EXPECT_TRUE(aliveRight);
}

TEST_F(FPSciTests, KillTargetFrontHoldclick) {
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
	respawnTargets();
	zeroCameraRotation();
	s_fakeInput->window().injectMouseDown(0);
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_FALSE(aliveFront);

	s_fakeInput->window().injectMouseUp(0);
	s_app->oneFrame();
}

TEST_F(FPSciTests, KillTargetRightRotate) {
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
	respawnTargets();

	// Kill the right target by rotating to line it up
	zeroCameraRotation();
	rotateCamera(30.0, 0);
	s_app->oneFrame();
	s_fakeInput->window().injectFire();
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_EQ(s_app->sess->targetArray().size(), 2) << "We shot once. There should be two targets left.";
	EXPECT_TRUE(aliveFront);
	EXPECT_FALSE(aliveRight);
}

TEST_F(FPSciTests, KillTargetRightTranslate) {
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
	respawnTargets();

	zeroCameraRotation();
	auto player = getPlayer();
	ASSERT_TRUE(notNull(player));

	// Kill the right target by moving to line it up
	const float moveX = 2.5f;
	*player->moveRate = (float)(moveX / fixedTestDeltaTime());
	*player->moveScale = Vector2::one();
	s_fakeInput->window().injectKeyDown(GKey('d'));
	s_app->oneFrame();
	s_fakeInput->window().injectKeyUp(GKey('d'));

	s_fakeInput->window().injectFire();
	s_app->oneFrame();

	bool aliveFront, aliveRight;
	checkTargets(aliveFront, aliveRight);
	EXPECT_EQ(s_app->simStepDuration(), GApp::MATCH_REAL_TIME_TARGET);
	EXPECT_EQ(s_app->sess->targetArray().size(), 2) << "We shot once. There should be two targets left.";
	EXPECT_TRUE(aliveFront);
	EXPECT_FALSE(aliveRight);
}

TEST_F(FPSciTests, ResetCamera) {
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);

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
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
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
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
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
	EXPECT_EQ(s_app->sess->presentationState, PresentationState::task);
	respawnTargets();
	zeroCameraRotation();

	int frames = 3;
	float damagePerFrame = 0.1f;

	s_app->sessConfig->weapon.autoFire = true;
	s_app->sessConfig->weapon.damagePerSecond = (float)(damagePerFrame / fixedTestDeltaTime());

	s_fakeInput->window().injectMouseDown(0);
	for (int i = 0; i < frames; ++i)
		s_app->oneFrame();
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
