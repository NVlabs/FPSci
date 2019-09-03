#include "PlayerControls.h"
#include "App.h"

PlayerControls::PlayerControls(App *app, const shared_ptr<GuiTheme>& theme, float width, float height) :
	GuiWindow("Player Controls", theme, Rect2D::xywh(5, 5, width, height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE)
{
	m_app = app;

	// Create the GUI pane
	GuiPane* pane = GuiWindow::pane();

	pane->beginRow(); {
		auto  c = pane->addNumberBox("Player Height", &m_app->experimentConfig.playerHeight, "m", GuiTheme::LINEAR_SLIDER, 0.2f, 3.0f);
		c->setCaptionWidth(width/2);
		c->setWidth(width*0.95);
	} pane->endRow();
	pane->beginRow(); {
		auto c = pane->addNumberBox("Player Crouch Height", &m_app->experimentConfig.crouchHeight, "m", GuiTheme::LINEAR_SLIDER, 0.2f, 3.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95);
	} pane->endRow();
	pane->beginRow(); {
		auto c = pane->addNumberBox("Move Rate", &m_app->experimentConfig.moveRate, "m/s", GuiTheme::LINEAR_SLIDER, 0.0f, 30.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95);
	}pane->endRow();
	pane->beginRow(); {
		pane->addButton("Set Start Position", m_app, &App::exportScene);
	} pane->endRow();
}

shared_ptr<PlayerControls> PlayerControls::create(App* app, const shared_ptr<GuiTheme>& theme, float width, float height) {
	return createShared<PlayerControls>(app, theme, width, height);
}