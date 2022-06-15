/**
  \file maxPerf/FPSciNetworkApp.h

 */
#pragma once
#include <G3D/G3D.h>



class NoWindow : public OSWindow {
public:
    NoWindow(const OSWindow::Settings& settings) : OSWindow() {}
    
    static NoWindow* create(const OSWindow::Settings& = Settings());

    void setInputCapture(bool) override;
	void setMouseVisible(bool) override;
    void getSettings(OSWindow::Settings&) const override;
    int width() const override;
    int height() const override;
    Rect2D fullRect() const override;
	Rect2D clientRect() const override;
    void setFullRect(const Rect2D&) override;
	void setClientRect(const Rect2D&) override;
    void getDroppedFilenames(Array<String>&) override;
    void setFullPosition(int, int) override;
    void setClientPosition(int, int) override;
    bool hasFocus() const override;
    String getAPIVersion() const override;
    String getAPIName() const override;
    String className() const override;
    void setGammaRamp(const Array<uint16>&) override;
    void setCaption(const String&) override;
    int numJoysticks() const override;
    String joystickName(unsigned int) const override;
    String caption() override;
    void swapGLBuffers() override;
    void setRelativeMousePosition(double, double) override;
    void setRelativeMousePosition(const Vector2&) override;
    void getRelativeMouseState(Vector2&, uint8&) const override;
    void getRelativeMouseState(int&, int&, uint8&) const override;
    void getRelativeMouseState(double&, double&, uint8&) const override;
    void getJoystickState(unsigned int, Array<float>&, Array<bool>&) const override;
    void getGameControllerState(unsigned int, Array<float>&, Array<bool>&) const override;


protected:
    void setAsCurrentGraphicsContext() const override;
    String _clipboardText() const override;
    void _setClipboardText(const String&) const override;
    
	
};


class FPSciNetworkApp : public GApp {

public:
    FPSciNetworkApp(const GApp::Settings& settings) : GApp(settings, (OSWindow*)NoWindow::create(), nullptr, true) { }

};