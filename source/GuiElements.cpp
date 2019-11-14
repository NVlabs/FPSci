#include "GuiElements.h"
#include "App.h"

bool WaypointDisplay::TreeDisplay::onEvent(const GEvent & event) {
	if (!m_visible) {
		return false;
	}
	WaypointDisplay* window = dynamic_cast<WaypointDisplay*>(this->window());
	Vector2 mousePositionDeBumped = event.mousePosition() - Vector2(window->m_scrollPane->horizontalOffset(), window->m_scrollPane->verticalOffset());
	if (event.type == GEventType::MOUSE_BUTTON_DOWN && (m_rect.contains(mousePositionDeBumped))) {
		float y = 0;
		for (int i = 0; i < m_waypoints->size(); ++i) {
			Destination d = (*m_waypoints)[i];
			if (Rect2D::xyxy(m_config.tree_indent, y, float(m_config.tree_display_width_px), y + m_config.tree_height).contains(event.mousePosition())) {
					m_selectedIdx = i;
					return true;
			}
			y += m_config.tree_height;
		}
	}
	return false;
}

WaypointDisplay::TreeDisplay::TreeDisplay(GuiWindow* w, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints) : GuiControl(w) {
	m_config = config;
	m_waypoints = waypoints;
	m_icon = GFont::fromFile(System::findDataFile("icon.fnt"));
}

void WaypointDisplay::TreeDisplay::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
	float y = 0;
	#define SHOW_TEXT(x, t) theme->renderLabel(Rect2D::xywh(x + m_config.tree_indent, y, float(m_config.tree_display_width_px), m_config.tree_height), (t), GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM, true, false);
	for (int i = 0; i < m_waypoints->size(); i++) {
		Destination d = (*m_waypoints)[i];
		// Handle selection here
		if (i == m_selectedIdx) {
			theme->renderSelection(Rect2D::xywh(0, y, float(m_config.tree_display_width_px), m_config.tree_height));
		}
		// Draw the fields
		float pos = m_config.tree_indent;
		SHOW_TEXT(pos, String(std::to_string(i)));
		pos += m_config.idx_column_width_px;
		SHOW_TEXT(pos, String(std::to_string(d.time)));
		pos += m_config.time_column_width_px;
		SHOW_TEXT(pos, d.position.toString());
		y += m_config.tree_height;
	}

	// Make sure that the window is large enough.  Has to be at least the height of the containing window
	// or we aren't guaranteed to have render called again
	y = max(y, m_gui->rect().height()) + 40;
	const_cast<WaypointDisplay::TreeDisplay*>(this)->setHeight(y);
	const_cast<GuiContainer*>(m_parent)->setHeight(y);
}

WaypointDisplay::WaypointDisplay(App* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints) :
	GuiWindow("Waypoint Manager",
		theme,
		Rect2D::xywh(0, 0, (float)config.tree_display_width_px + 10, (float)config.tree_display_height_px+10),
		GuiTheme::NORMAL_WINDOW_STYLE,
		GuiWindow::HIDE_ON_CLOSE)
{
	// Store the app pointer 
	m_app = app;

	// Create a pane
	GuiPane* pane = GuiWindow::pane();

	// Basic control
	auto addPane = pane->addPane("Add Waypoints");
	addPane->beginRow(); {
		addPane->addButton("Drop waypoint", m_app, &App::dropWaypoint);
		auto c = addPane->addNumberBox("Delay", &m_app->waypointDelay, "s");
		c->setCaptionWidth(40.0f);
		c->setWidth(120.0f);
		addPane->addNumberBox("Height Offset", &m_app->waypointVertOffset, "m")->setWidth(150.0f);
	}; addPane->endRow();

	// Removing waypoints
	auto removePane = pane->addPane("Remove Waypoints");
	removePane->beginRow(); {
		removePane->addButton("Remove waypoint", m_app, &App::removeHighlighted);
		removePane->addButton("Remove last", m_app, &App::removeLastWaypoint);
		removePane->addButton("Clear all", m_app, &App::clearWaypoints);
	} removePane->endRow();

	// File control
	auto filePane = pane->addPane("File Input/Output");
	filePane->beginRow(); {
		filePane->addButton("Load", m_app, &App::loadWaypoints);
		filePane->addButton("Save", m_app, &App::exportWaypoints);
		auto t = filePane->addTextBox("Filename", &m_app->waypointFile);
		t->setCaptionWidth(60.0f);
		t->setWidth(180.0f);
	} filePane->endRow();

	// Preview
	auto previewPane = pane->addPane("Preview");
	previewPane->beginRow(); {
		previewPane->addButton("Preview", m_app, &App::previewWaypoints);
		previewPane->addButton("Stop Preview", m_app, &App::stopPreview);
	} previewPane->endRow();

	// Recording
	auto recordPane = pane->addPane("Recording");
	recordPane->beginRow(); {
		recordPane->addCheckBox("Record motion", &m_app->recordMotion);
		recordPane->addDropDownList("Record Mode",
			Array<GuiText> {GuiText("Fixed Distance"), GuiText("Fixed Time")},
			&m_app->recordMode);
	} recordPane->endRow();
	pane->beginRow();{
		auto c = recordPane->addNumberBox("Interval", &m_app->recordInterval);
		c->setCaptionWidth(50.0f);
		c->setWidth(120.0f);
		c = recordPane->addNumberBox("Time Scale", &m_app->recordTimeScaling, "x");
		c->setCaptionWidth(80.0f);
		c->setWidth(150.0f);
	} recordPane->endRow();


	// Setup the row labels
	auto waypointPane = pane->addPane("Waypoints");
	GuiLabel* a = waypointPane->addLabel("Index"); a->setWidth(config.idx_column_width_px + config.tree_indent);
	GuiLabel* b = waypointPane->addLabel("Time"); b->setWidth(config.time_column_width_px + config.tree_indent); b->moveRightOf(a); a = b;
	b = waypointPane->addLabel("Position"); b->setWidth(config.xyz_column_width_px + config.tree_indent); b->moveRightOf(a); a = b;

	// Create the tree display
	m_treeDisplay = new TreeDisplay(this, config, waypoints);
	m_treeDisplay->moveBy(0, -5);		// Dunno why this happens...
	m_treeDisplay->setSize((float)config.tree_display_width_px, (float)config.tree_display_height_px);

	// Create the scroll pane
	m_scrollPane = waypointPane->addScrollPane(true, true);
	m_scrollPane->setSize((float)m_treeDisplay->rect().width()+10, (float)config.tree_display_height_px+10);
	m_scrollPane->viewPane()->addCustom(m_treeDisplay);
	pack();

	// Move to right location
	moveTo(Vector2(app->window()->width() - rect().width() - 10, 50));
}

void WaypointDisplay::setManager(WidgetManager *manager) {
	GuiWindow::setManager(manager);
	if (manager) {
		// Move to the upper right
		///float osWindowWidth = (float)manager->window()->width();
		///setRect(Rect2D::xywh(osWindowWidth - rect().width(), 40, rect().width(), rect().height()));
	}
}

PlayerControls::PlayerControls(SessionConfig& config, std::function<void()> exportCallback,
	const shared_ptr<GuiTheme>& theme, float width, float height) :
	GuiWindow("Player Controls", theme, Rect2D::xywh(5, 5, width, height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE)
{
	// Create the GUI pane
	GuiPane* pane = GuiWindow::pane();
	auto heightPane = pane->addPane("Height");
	heightPane->beginRow(); {
		auto  c = heightPane->addNumberBox("Player Height", &(config.player.height), "m", GuiTheme::LINEAR_SLIDER, 0.2f, 3.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	} heightPane->endRow();
	heightPane->beginRow(); {
		auto c = heightPane->addNumberBox("Player Crouch Height", &(config.player.crouchHeight), "m", GuiTheme::LINEAR_SLIDER, 0.2f, 3.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	} heightPane->endRow();

	auto movePane = pane->addPane("Movement");
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Move Rate", &(config.player.moveRate), "m/s", GuiTheme::LINEAR_SLIDER, 0.0f, 30.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	}movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Jump Velocity", &(config.player.jumpVelocity), "m/s", GuiTheme::LINEAR_SLIDER, 0.0f, 50.0f, 0.1f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	} movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Jump Interval", &(config.player.jumpInterval), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 10.0f, 0.1f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	} movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addCheckBox("Jump Requires Contact?", &(config.player.jumpTouch));
		c->setCaptionWidth(width / 2);
		c->setWidth(width*0.95f);
	} movePane->endRow();

	auto positionPane = pane->addPane("Position");
	positionPane->beginRow(); {
		positionPane->addButton("Set Start Position", exportCallback);
	} positionPane->endRow();

	pack();
	moveTo(Vector2(440, 300));
}

RenderControls::RenderControls(SessionConfig& config, UserConfig& user, bool& drawFps, bool& turbo, const int numReticles, float& brightness,
	const shared_ptr<GuiTheme>& theme, float width, float height) :
	GuiWindow("Render Controls", theme, Rect2D::xywh(5,5,width,height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE)
{
	// Create the GUI pane
	GuiPane* pane = GuiWindow::pane();

	auto drawPane = pane->addPane("Drawing");
	drawPane->beginRow(); {
		auto cb = drawPane->addCheckBox("Show Weapon", &(config.weapon.renderModel));
		cb->setEnabled(!config.weapon.modelSpec.filename.empty());
		drawPane->addCheckBox("Show Bullets", &(config.weapon.renderBullets));
		drawPane->addCheckBox("Show cooldown", &config.hud.renderWeaponStatus);
	}drawPane->endRow();
	drawPane->beginRow(); {
		drawPane->addCheckBox("Show HUD", &(config.hud.enable));
		drawPane->addCheckBox("Show Banner", &(config.hud.showBanner));
		drawPane->addCheckBox("Show Ammo", &(config.hud.showAmmo));
		drawPane->addCheckBox("Show Health", &(config.hud.showPlayerHealthBar));
	} drawPane->endRow();

	
	auto framePane = pane->addPane("Frame Rate/Delay");
	framePane->beginRow(); {
		framePane->addCheckBox("Show FPS", &drawFps);
		framePane->addCheckBox("Turbo mode", &turbo);
	}framePane->endRow();
	framePane->beginRow(); {
		auto c = framePane->addNumberBox("Framerate", &(config.render.frameRate), "fps", GuiTheme::LINEAR_SLIDER, 1.0f, 1000.0f, 1.0f);
		c->setWidth(width*0.95f);
	} framePane->endRow();
	framePane->beginRow(); {
		auto c = framePane->addNumberBox("Display Lag", &(config.render.frameDelay), "f", GuiTheme::LINEAR_SLIDER, 0, 60);
		c->setWidth(width*0.95f);
	}framePane->endRow();


	auto reticlePane = pane->addPane("Reticle");
	reticlePane->beginRow(); {
		auto c = reticlePane->addNumberBox("Reticle", &(user.reticleIndex), "", GuiTheme::LINEAR_SLIDER, 0, numReticles, 1);
		c->setWidth(width*0.95f);
	} reticlePane->endRow();

	reticlePane->beginRow(); {
		auto c = reticlePane->addNumberBox("Reticle Scale Min", &(user.reticleScale[0]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
		c->setCaptionWidth(120.0f);
		c->setWidth(width*0.95f);
	} reticlePane->endRow();

	reticlePane->beginRow(); {
		auto c = reticlePane->addNumberBox("Reticle Scale Max", &(user.reticleScale[1]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
		c->setCaptionWidth(120.0f);
		c->setWidth(width*0.95f);
	} reticlePane->endRow();

	reticlePane->beginRow(); {
		auto l = reticlePane->addLabel("Reticle Color Min");
		l->setWidth(100.0f);
		auto c = reticlePane->addSlider("R", &(user.reticleColor[0].r), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("G", &(user.reticleColor[0].g), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("B", &(user.reticleColor[0].b), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("A", &(user.reticleColor[0].a), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
	} reticlePane->endRow();
	reticlePane->beginRow(); {
		auto l = reticlePane->addLabel("Reticle Color Max");
		l->setWidth(100.0f);
		auto c = reticlePane->addSlider("R", &(user.reticleColor[1].r), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("G", &(user.reticleColor[1].g), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("B", &(user.reticleColor[1].b), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
		c = reticlePane->addSlider("A", &(user.reticleColor[1].a), 0.0f, 1.0f);
		c->setCaptionWidth(10.0f);
		c->setWidth(80.0f);
	} reticlePane->endRow();
	reticlePane->beginRow(); {
		auto c = reticlePane->addNumberBox("Reticle Shrink Time", &(user.reticleShrinkTimeS), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 5.0f, 0.01f);
		c->setCaptionWidth(150.0f);
		c->setWidth(width*0.95f);
	} reticlePane->endRow();

	auto otherPane = pane->addPane("Other");
	otherPane->beginRow();{
		auto c = otherPane->addNumberBox("Brightness", &brightness, "x", GuiTheme::LOG_SLIDER, 0.01f, 2.0f);
		c->setWidth(width*0.95f);
	} otherPane->endRow();

	pack();
	moveTo(Vector2(0, 300));
}

WeaponControls::WeaponControls(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width, float height) : 
	GuiWindow("Weapon Controls", theme, Rect2D::xywh(5, 5, width, height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE)
{
	// Create the GUI pane
	GuiPane* pane = GuiWindow::pane();

	pane->beginRow(); {
		pane->addNumberBox("Max Ammo", &(config.maxAmmo), "", GuiTheme::NO_SLIDER, 0, 100000, 1);
		pane->addNumberBox("Fire Period", &(config.firePeriod), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 10.0f, 0.1f);
		pane->addCheckBox("Autofire", &(config.autoFire));
	} pane->endRow();
	pane->beginRow(); {
		auto n = pane->addNumberBox("Damage", &(config.damagePerSecond), "health/s", GuiTheme::LINEAR_SLIDER, 0.0f, 100.0f, 0.1f);
		n->setWidth(300.0f);
		n->setUnitsSize(50.0f);
	} pane->endRow();
	pane->beginRow(); {
		auto c = pane->addLabel("Muzzle offset");
		c->setWidth(100.0f);
		auto n = pane->addNumberBox("X", &(config.muzzleOffset.x), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
		n->setCaptionWidth(10.0f);
		n->setWidth(150.0f);
		n = pane->addNumberBox("Y", &(config.muzzleOffset.y), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
		n->setCaptionWidth(10.0f);
		n->setWidth(150.0f);
		n = pane->addNumberBox("Z", &(config.muzzleOffset.z), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
		n->setCaptionWidth(10.0f);
		n->setWidth(150.0f);
	} pane->endRow();
	pane->beginRow(); {
		pane->addCheckBox("Muzzle flash", &(config.renderMuzzleFlash));
	} pane->endRow();

	pack();
	moveTo(Vector2(0, 720));
}