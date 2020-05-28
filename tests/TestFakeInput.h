#pragma once
#include <G3D/G3D.h>

// This struct holds hard coded per-frame events to be injected into the app
struct FakeInputEvent {
	// Delta in meters
	Vector2 mouseDeltaMeters;

	bool fire;

	bool ready;
};

class FakeWindow : public OSWindow {
public:
	FakeWindow(double mouseDPI) : m_mouseDPI(mouseDPI) {}

	void injectEvents(FakeInputEvent events);
	void injectFire();
	void injectMouseDown(int button);
	void injectMouseUp(int button);
	void injectKeyDown(GKey key);
	void injectKeyUp(GKey key);
	void injectReady();
	void injectMove(double mouseDeltaMetersX, double mouseDeltaMetersY);

	void getMousePosition(int & x, int & y) const;

	// Inherited via OSWindow
	virtual void setInputCapture(bool c) override;
	virtual void setMouseVisible(bool b) override;
	virtual void setAsCurrentGraphicsContext() const override;
	virtual void getSettings(OSWindow::Settings & settings) const override;
	virtual int width() const override;
	virtual int height() const override;
	virtual Rect2D fullRect() const override;
	virtual Rect2D clientRect() const override;
	virtual void setFullRect(const Rect2D & dims) override;
	virtual void setClientRect(const Rect2D & dims) override;
	virtual void getDroppedFilenames(Array<String>& files) override;
	virtual void setFullPosition(int x, int y) override;
	virtual void setClientPosition(int x, int y) override;
	virtual bool hasFocus() const override;
	virtual String getAPIVersion() const override;
	virtual String getAPIName() const override;
	virtual String className() const override;
	virtual void setGammaRamp(const Array<uint16>& gammaRamp) override;
	virtual void setCaption(const String & caption) override;
	virtual int numJoysticks() const override;
	virtual String joystickName(unsigned int sticknum) const override;
	virtual String caption() override;
	virtual void swapGLBuffers() override;
	virtual void setRelativeMousePosition(double x, double y) override;
	virtual void setRelativeMousePosition(const Vector2 & p) override;
	virtual void getRelativeMouseState(Vector2 & position, uint8 & mouseButtons) const override;
	virtual void getRelativeMouseState(int & x, int & y, uint8 & mouseButtons) const override;
	virtual void getRelativeMouseState(double & x, double & y, uint8 & mouseButtons) const override;
	virtual void getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button) const override;
	virtual String _clipboardText() const override;
	virtual void _setClipboardText(const String & text) const override;

private:

	// Current position in pixels
	double m_x = 0.0;
	double m_y = 0.0;
	double m_mouseDPI;
};

// This class exists so the fake window exists until the fake userInput is deleted.
// Otherwise a regular UserInput object could be used.
class FakeUserInput : public UserInput {
public:
	FakeUserInput(std::shared_ptr<FakeWindow> window);
	std::shared_ptr<FakeWindow> m_fakeWindow;
};

// Add this class to the scene for automatic event injection
// It's a Widget to get a per-frame event before onUserInput is run
// Alternative is to run at the end of each frame and do nothing on the first
class TestFakeInput : public Widget {
protected:
	Queue<FakeInputEvent> m_frameInputs;
	std::shared_ptr<FakeWindow> m_fakeWindow;
	std::shared_ptr<GApp> m_app;
	std::shared_ptr<UserInput> m_originalUserInput;
public:
	// Replaces the given app's userInput with a fake one
	TestFakeInput(std::shared_ptr<GApp> app, double mouseDPI);

	FakeWindow& window() {return *m_fakeWindow;}

	void queueInput(const FakeInputEvent& input);

	void processGEventQueue();

	virtual void onAfterEvents() override;
};
