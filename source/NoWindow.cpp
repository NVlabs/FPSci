#include "NoWindow.h"

NoWindow* NoWindow::create(const OSWindow::Settings& s) {
    NoWindow* window = new NoWindow(s);
    return window;
}


void NoWindow::setInputCapture(bool b) { return; }
void NoWindow::setMouseVisible(bool) {}
void NoWindow::getSettings(OSWindow::Settings&) const {}
int NoWindow::width() const {
    return -1;
}
int NoWindow::height() const {
    return -1;
}
Rect2D NoWindow::fullRect() const {
    return Rect2D();
}
Rect2D NoWindow::clientRect() const {
    return Rect2D();
}
void NoWindow::setFullRect(const Rect2D&) {}
void NoWindow::setClientRect(const Rect2D&) {}
void NoWindow::getDroppedFilenames(Array<String>&) {}
void NoWindow::setFullPosition(int, int) {}
void NoWindow::setClientPosition(int, int) {}
bool NoWindow::hasFocus() const {
    return false;
}
String NoWindow::getAPIVersion() const {
    return String();
}
String NoWindow::getAPIName() const {
    return String();
}
String NoWindow::className() const {
    return String();
}
void NoWindow::setGammaRamp(const Array<uint16>&) {}
void NoWindow::setCaption(const String&) {}
int NoWindow::numJoysticks() const {
    return -1;
}
String NoWindow::joystickName(unsigned int) const {
    return String();
}
String NoWindow::caption() {
    return String();
}
void NoWindow::swapGLBuffers() {}
void NoWindow::setRelativeMousePosition(double, double) {}
void NoWindow::setRelativeMousePosition(const Vector2&) {}
void NoWindow::getRelativeMouseState(Vector2&, uint8&) const {}
void NoWindow::getRelativeMouseState(int&, int&, uint8&) const {}
void NoWindow::getRelativeMouseState(double&, double&, uint8&) const {}
void NoWindow::getJoystickState(unsigned int, Array<float>&, Array<bool>&) const {}
void NoWindow::getGameControllerState(unsigned int, Array<float>&, Array<bool>&) const {}

void NoWindow::setAsCurrentGraphicsContext() const {}
String NoWindow::_clipboardText() const {
    return String();
}
void NoWindow::_setClipboardText(const String&) const {}