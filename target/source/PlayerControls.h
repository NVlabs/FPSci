#pragma once
#include <G3D/G3D.h>

class App;

class PlayerControls : public GuiWindow {
protected:
	App *m_app;
	PlayerControls(App *app, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);

public:
	static shared_ptr<PlayerControls> create(App* app, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);
};