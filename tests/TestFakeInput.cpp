#include "TestFakeInput.h"

void FakeWindow::injectEvents(FakeInputEvent events)
{
	injectMove(events.mouseDeltaMeters.x, events.mouseDeltaMeters.y);

	if (events.fire) {
		injectFire();
	}

	if (events.ready) {
		injectReady();
	}
}

// This function doesn't work because the mouse down gets lost without a frame between it and mouse up
void FakeWindow::injectFire()
{
	injectMouseDown(0);
	injectMouseUp(0);
}

void FakeWindow::injectMouseDown(int button)
{
	GEvent down;
	down.button.button = button;
	down.type = GEventType::MOUSE_BUTTON_DOWN;
	down.button.state = GButtonState::PRESSED;
	m_eventQueue.pushBack(down);
}

void FakeWindow::injectMouseUp(int button)
{
	GEvent up;
	up.button.button = button;
	up.type = GEventType::MOUSE_BUTTON_UP;
	up.button.state = GButtonState::RELEASED;
	m_eventQueue.pushBack(up);
}

void FakeWindow::injectKeyDown(GKey key)
{
	GEvent down;
	down.type = GEventType::KEY_DOWN;
	down.key.keysym.sym = (GKey::Value)(int)key;
	down.key.state = GButtonState::PRESSED;
	m_eventQueue.pushBack(down);
}

void FakeWindow::injectKeyUp(GKey key)
{
	GEvent up;
	up.type = GEventType::KEY_UP;
	up.key.keysym.sym = (GKey::Value)(int)key;
	up.key.state = GButtonState::RELEASED;
	m_eventQueue.pushBack(up);
}

void FakeWindow::injectReady()
{
	injectKeyDown(GKey::LSHIFT);
	injectKeyUp(GKey::LSHIFT);
}

void FakeWindow::injectMove(double mouseDeltaMetersX, double mouseDeltaMetersY)
{
	double dotsPerMeter = m_mouseDPI * 39.3701;
	m_x += mouseDeltaMetersX * dotsPerMeter;
	m_y += mouseDeltaMetersY * dotsPerMeter;
}

void FakeWindow::getMousePosition(int &x, int &y) const {
	x = (int)m_x;
	y = (int)m_y;
}

void FakeWindow::setInputCapture(bool c)
{
}

void FakeWindow::setMouseVisible(bool b)
{
}

void FakeWindow::setAsCurrentGraphicsContext() const
{
}

void FakeWindow::getSettings(OSWindow::Settings & settings) const
{
}

int FakeWindow::width() const
{
	return 0;
}

int FakeWindow::height() const
{
	return 0;
}

Rect2D FakeWindow::fullRect() const
{
	return Rect2D();
}

Rect2D FakeWindow::clientRect() const
{
	return Rect2D();
}

void FakeWindow::setFullRect(const Rect2D & dims)
{
}

void FakeWindow::setClientRect(const Rect2D & dims)
{
}

void FakeWindow::getDroppedFilenames(Array<String>& files)
{
}

void FakeWindow::setFullPosition(int x, int y)
{
}

void FakeWindow::setClientPosition(int x, int y)
{
}

bool FakeWindow::hasFocus() const
{
	return false;
}

String FakeWindow::getAPIVersion() const
{
	return String();
}

String FakeWindow::getAPIName() const
{
	return String();
}

String FakeWindow::className() const
{
	return String();
}

void FakeWindow::setGammaRamp(const Array<uint16>& gammaRamp)
{
}

void FakeWindow::setCaption(const String & caption)
{
}

int FakeWindow::numJoysticks() const
{
	return 0;
}

String FakeWindow::joystickName(unsigned int sticknum) const
{
	return String();
}

String FakeWindow::caption()
{
	return String();
}

void FakeWindow::swapGLBuffers()
{
}

void FakeWindow::setRelativeMousePosition(double x, double y)
{
}

void FakeWindow::setRelativeMousePosition(const Vector2 & p)
{
}

void FakeWindow::getRelativeMouseState(Vector2 & position, uint8 & mouseButtons) const
{
    position.x = (float)m_x;
    position.y = (float)m_y;
}

void FakeWindow::getRelativeMouseState(int & x, int & y, uint8 & mouseButtons) const
{
    getMousePosition(x, y);
}

void FakeWindow::getRelativeMouseState(double & x, double & y, uint8 & mouseButtons) const
{
    x = m_x;
    y = m_y;
}

void FakeWindow::getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button) const
{
}

String FakeWindow::_clipboardText() const
{
	return String();
}

void FakeWindow::_setClipboardText(const String & text) const
{
}

FakeUserInput::FakeUserInput(std::shared_ptr<FakeWindow> window) : UserInput(window.get()), m_fakeWindow(window)
{
}

TestFakeInput::TestFakeInput(std::shared_ptr<GApp> app, double mouseDPI) : m_app(app)
{
	m_fakeWindow = std::make_shared<FakeWindow>(mouseDPI);

	// Surprisingly userInput is public, but since it is it can be replaced
	// Keep the oringal alive since other objects such as FirstPersonManipulator take raw pointer references to it.
	// This is dangerous as the lifetime of TestFakeInput is not guaranteed.
	m_originalUserInput.reset(m_app->userInput);

	// Replace the app's UserInput with the fake one
	m_app->userInput = new FakeUserInput(m_fakeWindow);

	// Stop the app calling begin/end events on the fake UserInput
	m_app->manageUserInput = false;
}

void TestFakeInput::queueInput(const FakeInputEvent & input)
{
	m_frameInputs.pushBack(input);
}

void TestFakeInput::processGEventQueue()
{
    GEvent event;
    m_app->userInput->beginEvents();
    while (m_fakeWindow->pollEvent(event)) {
		m_app->userInput->processEvent(event);
	}
	m_app->userInput->endEvents();
}

void TestFakeInput::onAfterEvents()
{
	if (m_frameInputs.size()) {
		m_fakeWindow->injectEvents(m_frameInputs.popFront());
	}

	processGEventQueue();
}
