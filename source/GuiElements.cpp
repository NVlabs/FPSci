#include "GuiElements.h"
#include "WaypointManager.h"
#include "FPSciApp.h"

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

WaypointDisplay::WaypointDisplay(FPSciApp* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints) :
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
		addPane->addButton("Drop waypoint", m_app->waypointManager, &WaypointManager::dropWaypoint);
		auto c = addPane->addNumberBox("Delay", &m_app->waypointManager->waypointDelay, "s");
		c->setCaptionWidth(40.0f);
		c->setWidth(120.0f);
		addPane->addNumberBox("Height Offset", &m_app->waypointManager->waypointVertOffset, "m")->setWidth(150.0f);
	}; addPane->endRow();

	// Removing waypoints
	auto removePane = pane->addPane("Remove Waypoints");
	removePane->beginRow(); {
		removePane->addButton("Remove waypoint", m_app->waypointManager, &WaypointManager::removeHighlighted);
		removePane->addButton("Remove last", m_app->waypointManager, &WaypointManager::removeLastWaypoint);
		removePane->addButton("Clear all", m_app->waypointManager, &WaypointManager::clearWaypoints);
	} removePane->endRow();

	// File control
	auto filePane = pane->addPane("File Input/Output");
	filePane->beginRow(); {
		filePane->addButton("Load", m_app->waypointManager, &WaypointManager::loadWaypoints);
		filePane->addButton("Save", m_app->waypointManager, &WaypointManager::exportWaypoints);
		auto t = filePane->addTextBox("Filename", &m_app->waypointManager->exportFilename);
		t->setCaptionWidth(60.0f);
		t->setWidth(180.0f);
	} filePane->endRow();

	// Preview
	auto previewPane = pane->addPane("Preview");
	previewPane->beginRow(); {
		previewPane->addButton("Preview", m_app->waypointManager, &WaypointManager::previewWaypoints);
		previewPane->addButton("Stop Preview", m_app->waypointManager, &WaypointManager::stopPreview);
	} previewPane->endRow();

	// Recording
	auto recordPane = pane->addPane("Recording");
	recordPane->beginRow(); {
		recordPane->addCheckBox("Record motion", &m_app->waypointManager->recordMotion);
		recordPane->addDropDownList("Record Mode",
			Array<GuiText> {GuiText("Fixed Distance"), GuiText("Fixed Time")},
			&m_app->waypointManager->recordMode);
	} recordPane->endRow();
	pane->beginRow();{
		auto c = recordPane->addNumberBox("Interval", &m_app->waypointManager->recordInterval);
		c->setCaptionWidth(50.0f);
		c->setWidth(120.0f);
		c = recordPane->addNumberBox("Time Scale", &m_app->waypointManager->recordTimeScaling, "x");
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
		auto c = movePane->addNumberBox("Movement Acceleration", &(config.player.movementAcceleration), "", GuiTheme::LINEAR_SLIDER, 0.001f, 1.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width * 0.95f);
	}movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Movement Deceleration", &(config.player.movementDeceleration), "", GuiTheme::LINEAR_SLIDER, 0.001f, 1.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width * 0.95f);
	}movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Sprint Multiplier", &(config.player.sprintMultiplier), "x", GuiTheme::LINEAR_SLIDER, 1.0f, 10.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width * 0.95f);
	}movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Headbob Amplitude", &(config.player.headBobAmplitude), "x", GuiTheme::LINEAR_SLIDER, 0.0f, 0.5f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width * 0.95f);
	}movePane->endRow();
	movePane->beginRow(); {
		auto c = movePane->addNumberBox("Headbob Frequency", &(config.player.headBobFrequency), "x", GuiTheme::LINEAR_SLIDER, 0.0f, 3.0f);
		c->setCaptionWidth(width / 2);
		c->setWidth(width * 0.95f);
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

RenderControls::RenderControls(FPSciApp* app, SessionConfig& config, bool& drawFps, const int numReticles, float& brightness,
	const shared_ptr<GuiTheme>& theme, const int maxFrameDelay, const float minFrameRate, const float maxFrameRate, float width, float height) :
	GuiWindow("Render Controls", theme, Rect2D::xywh(5,5,width,height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE), m_app(app)
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
	}framePane->endRow();
	framePane->beginRow(); {
		auto c = framePane->addNumberBox("Framerate", &(config.render.frameRate), "fps", GuiTheme::LINEAR_SLIDER, minFrameRate, maxFrameRate, 1.0f);
		c->setWidth(width*0.95f);
	} framePane->endRow();
	framePane->beginRow(); {
		auto c = framePane->addNumberBox("Display Lag", &(config.render.frameDelay), "f", GuiTheme::LINEAR_SLIDER, 0, maxFrameDelay);
		c->setWidth(width*0.95f);
	}framePane->endRow();

	auto menuPane = pane->addPane("User Menu");
	menuPane->beginRow(); {
		m_storedMenuConfig.allowReticleChange = true;
		m_storedMenuConfig.allowReticleChangeTimeChange = true;
		m_showFullUserMenuBtn = menuPane->addButton("Show Full User Menu", this, &RenderControls::updateUserMenu);
		if (config.menu.allowAnyChange()) {
			// Default setup already allows any change
			m_showFullUserMenuBtn->setEnabled(false);
			m_showFullUserMenu = true;
		}
	} menuPane->endRow();

	auto otherPane = pane->addPane("Other");
	otherPane->beginRow();{
		auto c = otherPane->addNumberBox("Brightness", &brightness, "x", GuiTheme::LOG_SLIDER, 0.01f, 2.0f);
		c->setWidth(width*0.95f);
	} otherPane->endRow();

	pack();
	moveTo(Vector2(0, 300));
}

void RenderControls::updateUserMenu() {
	MenuConfig tmp = m_app->sessConfig->menu;		// Store current config
	m_app->sessConfig->menu = m_storedMenuConfig;	// Swap the config w/ the stored version
	if (!m_showFullUserMenu) {
		m_showFullUserMenu = true;
		m_showFullUserMenuBtn->setCaption("Hide Full User Menu");
	}
	else {
		m_showFullUserMenu = false;
		m_showFullUserMenuBtn->setCaption("Show Full User Menu");
	}
	m_storedMenuConfig = tmp;						// Update the stored config
	m_app->updateUserMenu = true;					// Set the semaphore to update the user menu
}

WeaponControls::WeaponControls(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width, float height) :
	GuiWindow("Weapon Controls", theme, Rect2D::xywh(5, 5, width, height), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE), m_config(config)
{
	// Create the GUI pane
	GuiPane* pane = GuiWindow::pane();

	const float cbWidth = 110.f;
	const float nbWidth = 300.f;

	pane->beginRow(); {
		pane->addNumberBox("Max Ammo", &(config.maxAmmo), "", GuiTheme::NO_SLIDER, 0, 100000, 1);
	} pane->endRow();
	pane->beginRow();{
		pane->addCheckBox("Autofire", &(config.autoFire))->setWidth(cbWidth);
		auto n = pane->addNumberBox("Fire Period", &(config.firePeriod), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 10.0f, 0.001f);
		n->setWidth(nbWidth);
	} pane->endRow();
	pane->beginRow(); {
		pane->addCheckBox("Hitscan", &(config.hitScan))->setWidth(cbWidth);
		auto n = pane->addNumberBox("Damage", &(config.damagePerSecond), "health/s", GuiTheme::LINEAR_SLIDER, 0.0f, 100.0f, 0.01f);
		n->setWidth(nbWidth);
		n->setUnitsSize(50.0f);
	} pane->endRow();
	pane->beginRow(); {
		pane->addCheckBox("Show Decals", &(config.renderDecals))->setWidth(cbWidth);
		auto n = pane->addNumberBox("Miss Decals", &(config.missDecalCount), "decals", GuiTheme::LINEAR_SLIDER, 0, 1000, 1);
		n->setWidth(nbWidth);
		n->setUnitsSize(50.f);
	} pane->endRow();
	pane->beginRow(); {
		pane->addNumberBox("Kick Angle", &(config.kickAngleDegrees), "\xB0", GuiTheme::LINEAR_SLIDER, 0.f, 45.f, 0.1f);
		pane->addNumberBox("Kick Duration", &(config.kickDuration), "s", GuiTheme::LINEAR_SLIDER, 0.f, 2.f, 0.01f);
	} pane->endRow();
	pane->beginRow(); {
		pane->addNumberBox("Fire Spread", &(config.fireSpreadDegrees), "\xB0", GuiTheme::LINEAR_SLIDER, 0.f, 120.f, 0.1f);
		m_spreadShapeIdx = m_spreadShapes.findIndex(m_config.fireSpreadShape);
		pane->addDropDownList("Spread Shape", m_spreadShapes, &m_spreadShapeIdx, std::bind(&WeaponControls::updateFireSpreadShape, this));
	} pane->endRow();
	//pane->beginRow(); {
	//	auto c = pane->addLabel("Muzzle offset");
	//	c->setWidth(100.0f);
	//	auto n = pane->addNumberBox("X", &(config.muzzleOffset.x), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
	//	n->setCaptionWidth(10.0f);
	//	n->setWidth(150.0f);
	//	n = pane->addNumberBox("Y", &(config.muzzleOffset.y), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
	//	n->setCaptionWidth(10.0f);
	//	n->setWidth(150.0f);
	//	n = pane->addNumberBox("Z", &(config.muzzleOffset.z), "m", GuiTheme::LINEAR_SLIDER, -1.0f, 1.0f, 0.01f);
	//	n->setCaptionWidth(10.0f);
	//	n->setWidth(150.0f);
	//} pane->endRow();
	//pane->beginRow(); {
	//	pane->addCheckBox("Muzzle flash", &(config.renderMuzzleFlash));
	//} pane->endRow();

	pack();
	moveTo(Vector2(0, 720));
}

void WeaponControls::updateFireSpreadShape() {
	m_config.fireSpreadShape = m_spreadShapes[m_spreadShapeIdx];
}

void MenuConfig::load(FPSciAnyTableReader reader, int settingsVersion) {
	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("showMenuLogo", showMenuLogo);
		reader.getIfPresent("showExperimentSettings", showExperimentSettings);
		reader.getIfPresent("showUserSettings", showUserSettings);
		reader.getIfPresent("allowSessionChange", allowSessionChange);
		reader.getIfPresent("allowUserAdd", allowUserAdd);
		reader.getIfPresent("requireUserAdd", requireUserAdd);
		reader.getIfPresent("allowUserSettingsSave", allowUserSettingsSave);
		reader.getIfPresent("allowSensitivityChange", allowSensitivityChange);
		reader.getIfPresent("allowTurnScaleChange", allowTurnScaleChange);
		reader.getIfPresent("xTurnScaleAdjustMode", xTurnScaleAdjustMode);
		reader.getIfPresent("yTurnScaleAdjustMode", yTurnScaleAdjustMode);
		reader.getIfPresent("allowReticleChange", allowReticleChange);
		reader.getIfPresent("allowReticleIdxChange", allowReticleIdxChange);
		reader.getIfPresent("allowReticleSizeChange", allowReticleSizeChange);
		reader.getIfPresent("allowReticleColorChange", allowReticleColorChange);
		reader.getIfPresent("allowReticleChangeTimeChange", allowReticleChangeTimeChange);
		reader.getIfPresent("showReticlePreview", showReticlePreview);
		reader.getIfPresent("showMenuOnStartup", showMenuOnStartup);
		reader.getIfPresent("showMenuBetweenSessions", showMenuBetweenSessions);
		break;
	default:
		throw format("Did not recognize settings version: %d", settingsVersion);
		break;
	}
}

Any MenuConfig::addToAny(Any a, const bool forceAll) const {
	MenuConfig def;
	if (forceAll || def.showMenuLogo != showMenuLogo)									a["showMenuLogo"] = showMenuLogo;
	if (forceAll || def.showExperimentSettings != showExperimentSettings)				a["showExperimentSettings"] = showExperimentSettings;
	if (forceAll || def.showUserSettings != showUserSettings)							a["showUserSettings"] = showUserSettings;
	if (forceAll || def.allowSessionChange != allowSessionChange)						a["allowSessionChange"] = allowSessionChange;
	if (forceAll || def.allowUserAdd != allowUserAdd)									a["allowUserAdd"] = allowUserAdd;
	if (forceAll || def.requireUserAdd != requireUserAdd)								a["requireUserAdd"] = requireUserAdd;
	if (forceAll || def.allowUserSettingsSave != allowUserSettingsSave)					a["allowUserSettingsSave"] = allowUserSettingsSave;
	if (forceAll || def.allowSensitivityChange != allowSensitivityChange)				a["allowSensitivityChange"] = allowSensitivityChange;
	if (forceAll || def.allowTurnScaleChange != allowTurnScaleChange)					a["allowTurnScaleChange"] = allowTurnScaleChange;
	if (forceAll || def.xTurnScaleAdjustMode != xTurnScaleAdjustMode)					a["xTurnScaleAdjustMode"] = xTurnScaleAdjustMode;
	if (forceAll || def.yTurnScaleAdjustMode != yTurnScaleAdjustMode)					a["yTurnScaleAdjustMode"] = yTurnScaleAdjustMode;
	if (forceAll || def.allowReticleChange != allowReticleChange)						a["allowReticleChange"] = allowReticleChange;
	if (forceAll || def.allowReticleIdxChange != allowReticleIdxChange)					a["allowReticleIdxChange"] = allowReticleIdxChange;
	if (forceAll || def.allowReticleSizeChange != allowReticleSizeChange)				a["allowReticleSizeChange"] = allowReticleSizeChange;
	if (forceAll || def.allowReticleColorChange != allowReticleColorChange)				a["allowReticleColorChange"] = allowReticleColorChange;
	if (forceAll || def.allowReticleChangeTimeChange != allowReticleChangeTimeChange)	a["allowReticleChangeTimeChange"] = allowReticleChangeTimeChange;
	if (forceAll || def.showReticlePreview != showReticlePreview)						a["showReticlePreview"] = showReticlePreview;
	if (forceAll || def.showMenuOnStartup != showMenuOnStartup)							a["showMenuOnStartup"] = showMenuOnStartup;
	if (forceAll || def.showMenuBetweenSessions != showMenuBetweenSessions)				a["showMenuBetweenSessions"] = showMenuBetweenSessions;
	return a;
}

bool MenuConfig::allowAnyChange() const {
	return allowSensitivityChange && allowTurnScaleChange &&
		allowReticleChange && allowReticleIdxChange && allowReticleColorChange && allowReticleSizeChange && allowReticleChangeTimeChange;
}

////////////////////////
/// USER MENU
///////////////////////
UserMenu::UserMenu(FPSciApp* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect) :
	GuiWindow("", theme, rect, GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::NO_CLOSE), m_app(app), m_users(users), m_userStatus(userStatus), m_config(config)
{
	m_reticlePreviewTexture = Texture::createEmpty("FPSci::ReticlePreview", m_app->reticleTexture->width(), m_app->reticleTexture->height());
	m_reticleBuffer = Framebuffer::create(m_reticlePreviewTexture);

	m_parent = pane();	

	GuiTextureBox* logoTb = nullptr;

	// Add logo
	if (config.showMenuLogo) {
		auto logo = Texture::fromFile("material/FPSciBanner.png", Texture::Encoding(), Texture::DIM_2D, false, Texture::Preprocess::defaults(), true);
		logoTb = m_parent->addTextureBox(m_app, "", logo, true);
		logoTb->setSize(Vector2(logo->width()+4.f, logo->height()+5.f));
		// It doesn't make sense, but this specific zoom value appears to do better than 1.0f
		logoTb->setViewZoom(0.9999f);
		logoTb->setEnabled(false);
	}

	// Experiment Settings Pane
	m_ddCurrUserIdx = m_users.getUserIndex(m_userStatus.currentUser);
	m_expPane = m_parent->addPane("Experiment Settings");
	m_expPane->setCaptionHeight(40);

	// Only draw experiment selection box in developer mode
	if (app->startupConfig.developerMode) {
		m_expPane->beginRow(); {
			m_expPane->addDropDownList("Experiment", app->experimentNames(), &(app->experimentIdx));
			m_expPane->addButton("Select Experiment", this, &UserMenu::updateExperimentPress);
		} m_expPane->endRow();
	}

	const bool needUser = config.requireUserAdd && !m_app->userAdded;
	m_expPane->beginRow(); {
		if (needUser) {
			m_expPane->addLabel("Add a new user");
		}
		else {
			m_userDropDown = m_expPane->addDropDownList("User", m_users.getIds(), &m_ddCurrUserIdx);
			m_expPane->addButton("Select User", this, &UserMenu::updateUserPress);
		}
	} m_expPane->endRow();
	if (m_config.allowUserAdd || m_config.requireUserAdd) {
		m_expPane->beginRow(); {
			m_expPane->addTextBox("New User", &m_newUser);
			m_expPane->addButton("+", this, &UserMenu::addUserPress)->setWidth(20.0f);
			m_newUserFeedback = m_expPane->addLabel("");
			m_newUserFeedback->setWidth(70.f);
		} m_expPane->endRow();
	}
	GuiButton* addBtn;
	m_expPane->beginRow(); {
		m_sessDropDown = m_expPane->addDropDownList("Session", Array<String>({}), &m_ddCurrSessIdx);
		updateSessionDropDown();
		addBtn = m_expPane->addButton("Select Session", this, &UserMenu::updateSessionPress);
	} m_expPane->endRow();
	m_sessDropDown->setVisible(m_config.allowSessionChange);
	addBtn->setVisible(m_config.allowSessionChange);

	// Hide the experiment settings if not requested to be drawn
	if (!config.showExperimentSettings) { 
		m_expPane->setVisible(false);
		m_expPane->setHeight(0.f);
	}

	// User Settings Pane
	if (config.showUserSettings && !needUser) {
		m_currentUserPane = m_parent->addPane("Current User Settings");
		drawUserPane(config, m_users.users[m_users.getUserIndex(m_userStatus.currentUser)]);
	}

	// Resume/Quite Pane
	GuiButton* resumeBtn = nullptr;
	GuiButton* quitBtn = nullptr;
	m_resumeQuitPane = m_parent->addPane();
	m_resumeQuitPane->beginRow(); {
		const Vector2 resumeQuitBtnSize = { 100.f, 40.f };
		// Create resume and quit buttons
		resumeBtn = m_resumeQuitPane->addButton("Resume", this, &UserMenu::resumePress, GuiTheme::TOOL_BUTTON_STYLE);
		resumeBtn->setSize(resumeQuitBtnSize);
		resumeBtn->setEnabled(!needUser);
		quitBtn = m_resumeQuitPane->addButton("Quit", m_app, &FPSciApp::quitRequest, GuiTheme::TOOL_BUTTON_STYLE);
		quitBtn->setSize(resumeQuitBtnSize);
	} m_resumeQuitPane->endRow();

	// Pack the window here (size for elements)
	pack();

	// Centering for (non-updated) menu elements
	if(logoTb) logoTb->moveBy({ bounds().width() / 2.f - logoTb->rect().width() / 2.f - 5.f, 0.f });
	
	// Position quit button on the right
	quitBtn->moveBy({ bounds().width() - 2.f * quitBtn->rect().width() - 15.f, 0.f });
	m_resumeQuitPane->pack();
}

void UserMenu::drawUserPane(const MenuConfig& config, UserConfig& user)
{
	// Basic user info
	m_currentUserPane->beginRow(); {
		m_currentUserPane->addLabel(format("Current User: %s", user.id.c_str()))->setHeight(30.0);
	} m_currentUserPane->endRow();

	const double captionWidth = 70.0;
	const double unitSize = 50.0;
	auto sensPane = m_currentUserPane->addPane("Mouse Settings", GuiTheme::ORNATE_PANE_STYLE);
	sensPane->beginRow(); {
		auto dpiDisplay = sensPane->addNumberBox("DPI", &user.mouseDPI, "", GuiTheme::NO_SLIDER, 1.0, 100000.0, 1.0);
		dpiDisplay->setCaptionWidth(captionWidth);
		dpiDisplay->setWidth(150.0);
		dpiDisplay->setEnabled(false);
	} sensPane->endRow();
	sensPane->beginRow(); {
		auto sensitivityNb = sensPane->addNumberBox("Sensitivity", &(user.mouseDegPerMm), "\xB0/mm", GuiTheme::LOG_SLIDER, 0.01, 100.0, 0.01);
		sensitivityNb->setWidth(300.0);
		sensitivityNb->setCaptionWidth(captionWidth);
		sensitivityNb->setUnitsSize(unitSize);
		sensitivityNb->setEnabled(config.allowSensitivityChange);
	} sensPane->endRow();
	sensPane->beginRow(); {
		auto cmp360Nb = sensPane->addNumberBox("", &m_cmp360, "cm/360\xB0", GuiTheme::NO_SLIDER, 0.0, 3600.0, 0.1);
		cmp360Nb->setWidth(180.0);
		cmp360Nb->setCaptionWidth(captionWidth);
		cmp360Nb->setUnitsSize(unitSize);
		cmp360Nb->setEnabled(false);
	} sensPane->endRow();
	if (config.allowTurnScaleChange) {
		// X turn scale
		if (config.xTurnScaleAdjustMode != "None") {
			sensPane->beginRow(); {
				sensPane->addNumberBox("Turn Scale X", &(user.turnScale.x), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f)->setWidth(m_sliderWidth);
			} sensPane->endRow();
		}
		// Y turn scale
		if (config.yTurnScaleAdjustMode != "None") {
			sensPane->beginRow(); {
				if (config.yTurnScaleAdjustMode == "Slider") {
					sensPane->addNumberBox("Turn Scale Y", &(user.turnScale.y), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f)->setWidth(m_sliderWidth);
				}
				else if (config.yTurnScaleAdjustMode == "Invert") {
					sensPane->addCheckBox("Invert Y", &(user.invertY));

				}
			} sensPane->endRow();
		}
	}

	// Reticle configuration
	if (config.allowReticleChange) {
		auto reticleControlPane = m_currentUserPane->addPane("Reticle Control", GuiTheme::ORNATE_PANE_STYLE);
		const float reticleCaptionWidth = 120.f;

		// Reticle index selection
		if (config.allowReticleIdxChange) {
			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle", &(user.reticle.index), "", GuiTheme::LINEAR_SLIDER, 0, m_app->numReticles, 1);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();
		}

		// Reticle size selection
		if (config.allowReticleSizeChange) {
			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle Scale Min", &(user.reticle.scale[0]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();

			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle Scale Max", &(user.reticle.scale[1]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();
		}

		// Reticle color selection
		if (config.allowReticleColorChange) {
			const float rgbCaptionWidth = 10.f;
			reticleControlPane->beginRow(); {
				reticleControlPane->addLabel("Reticle Color Min")->setWidth(120.f);
				auto r = reticleControlPane->addSlider("R", &(user.reticle.color[0].r), 0.0f, 1.0f);
				r->setCaptionWidth(rgbCaptionWidth);
				r->setWidth(m_rgbSliderWidth);
				auto g = reticleControlPane->addSlider("G", &(user.reticle.color[0].g), 0.0f, 1.0f);
				g->setCaptionWidth(rgbCaptionWidth);
				g->setWidth(m_rgbSliderWidth);
				g->moveRightOf(r, rgbCaptionWidth);
				auto b = reticleControlPane->addSlider("B", &(user.reticle.color[0].b), 0.0f, 1.0f);
				b->setCaptionWidth(rgbCaptionWidth);
				b->setWidth(m_rgbSliderWidth);
				b->moveRightOf(g, rgbCaptionWidth);
				auto a = reticleControlPane->addSlider("A", &(user.reticle.color[0].a), 0.0f, 1.0f);
				a->setCaptionWidth(rgbCaptionWidth);
				a->setWidth(m_rgbSliderWidth);
				a->moveRightOf(b, rgbCaptionWidth);
			} reticleControlPane->endRow();
			reticleControlPane->beginRow(); {
				reticleControlPane->addLabel("Reticle Color Max")->setWidth(120.f);
				auto r = reticleControlPane->addSlider("R", &(user.reticle.color[1].r), 0.0f, 1.0f);
				r->setCaptionWidth(rgbCaptionWidth);
				r->setWidth(m_rgbSliderWidth);
				auto g = reticleControlPane->addSlider("G", &(user.reticle.color[1].g), 0.0f, 1.0f);
				g->setCaptionWidth(rgbCaptionWidth);
				g->setWidth(m_rgbSliderWidth);
				g->moveRightOf(r, rgbCaptionWidth);
				auto b = reticleControlPane->addSlider("B", &(user.reticle.color[1].b), 0.0f, 1.0f);
				b->setCaptionWidth(rgbCaptionWidth);
				b->setWidth(m_rgbSliderWidth);
				b->moveRightOf(g, rgbCaptionWidth);
				auto a = reticleControlPane->addSlider("A", &(user.reticle.color[1].a), 0.0f, 1.0f);
				a->setCaptionWidth(rgbCaptionWidth);
				a->setWidth(m_rgbSliderWidth);
				a->moveRightOf(b, rgbCaptionWidth);
			} reticleControlPane->endRow();
			if (config.allowReticleChangeTimeChange) {
				reticleControlPane->beginRow(); {
					auto c = reticleControlPane->addNumberBox("Reticle Change Time", &(user.reticle.changeTimeS), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 5.0f, 0.01f);
					c->setCaptionWidth(150.0f);
					c->setWidth(m_sliderWidth);
				} reticleControlPane->endRow();
			}
		}

		// Draw a preview of the reticle here
		if (config.allowReticleChange && config.showReticlePreview) {
			m_reticlePreviewPane = m_currentUserPane->addPane("Reticle Preview");
			updateReticlePreview();
			m_reticlePreviewPane->moveRightOf(reticleControlPane);
		}
	}

	// Allow the user to save their settings?
	if (config.allowUserSettingsSave) {
		m_currentUserPane->beginRow(); {
			m_currentUserPane->addButton("Save settings", m_app, &FPSciApp::saveUserConfig)->setSize(m_btnSize);
		} m_currentUserPane->endRow();
	}

	m_currentUserPane->pack();
}

void UserMenu::updateExperimentPress() {
	m_app->reinitExperiment = true;				// Set the reinit semamphore to avoid event handling problems
}

Array<String> UserMenu::updateSessionDropDown() {
	// Create updated session list
	String userId = m_userStatus.currentUser;
	shared_ptr<UserSessionStatus> userStatus = m_userStatus.getUserStatus(userId);
	// If we have a user that doesn't have specified sessions
	if (userStatus == nullptr) {
		// Create a new user session status w/ no progress and default order
		logPrintf("User %s not found. Creating a new user w/ default session ordering.\n", userId);
		UserSessionStatus newStatus = UserSessionStatus();
		newStatus.id = userId;
		m_app->experimentConfig.getSessionIds(newStatus.sessionOrder);
		m_userStatus.userInfo.append(newStatus);
		userStatus = m_userStatus.getUserStatus(userId);
		m_app->saveUserStatus();
	}

	Array<String> remainingSess = {};
	if (m_userStatus.allowRepeat) {
		remainingSess = userStatus->sessionOrder;
		for (int i = 0; i < userStatus->completedSessions.size(); i++) {
			if (remainingSess.contains(userStatus->completedSessions[i])) {
				int idx = remainingSess.findIndex(userStatus->completedSessions[i]);
				remainingSess.remove(idx, 1);
			}
		}
	}
	else {
		for (int i = 0; i < userStatus->sessionOrder.size(); i++) {
			if (!userStatus->completedSessions.contains(userStatus->sessionOrder[i])) {
				// user hasn't (ever) completed this session
				remainingSess.append(userStatus->sessionOrder[i]);
			}
		}
	}
	m_sessDropDown->setList(remainingSess);

	if (m_app->sessConfig->logger.logSessDDUpdate) {
		// Print update to log each time we update the drop down
		logPrintf("Updated %s's session drop down to:\n", userId);
		for (String id : remainingSess) {
			logPrintf("\t%s\n", id);
		}
	}

	// Make sure there's an empty session in the list
	if (remainingSess.size() == 0) {
		remainingSess.append("");
	}

	return remainingSess;
}

void UserMenu::updateUserPress() {
	if (m_ddLastUserIdx != m_ddCurrUserIdx) {
		String userId = m_userDropDown->get(m_ddCurrUserIdx);
		updateUser(userId);
		m_ddLastUserIdx = m_ddCurrUserIdx;
	}
}

void UserMenu::updateUser(const String& id) {
	// Update the current user and save to the user config file
	m_userStatus.currentUser = id;
	m_app->saveUserStatus();
	// Update (selected) sessions
	const String sessId = updateSessionDropDown()[0];
	m_app->updateSession(sessId);
}

void UserMenu::addUserPress() {
	// Check for unique user name requirement
	if (m_newUser.empty()) {
		m_newUserFeedback->setCaption("Empty!");
		return;
	}
	else if (m_users.requireUnique && m_users.getIds().contains(m_newUser)) {
		m_newUser = "";
		m_newUserFeedback->setCaption("In use!");
		return;
	}
	m_newUserFeedback->setCaption("");		// Clear the user feedback caption on success

	// Create new user config
	UserConfig user = m_users.defaultUser;
	user.id = m_newUser;
	
	// Add user config to table and save
	m_users.users.append(user);
	m_app->saveUserConfig();

	// Create new user status
	UserSessionStatus status = m_userStatus.userInfo.last();		// Start by coping over last user
	status.id = m_newUser;											// Update the user ID
	status.completedSessions.clear();								// Empty any completed sessions from previous user
	// Inherit default session order (if available)
	if (m_userStatus.defaultSessionOrder.length() > 0) { status.sessionOrder = m_userStatus.defaultSessionOrder;  }
	// Randomize if requested
	if (m_userStatus.randomizeDefaults) { status.sessionOrder.randomize(); }
	
	// Add user status, set as current, and save
	m_userStatus.userInfo.append(status);
	m_userStatus.currentUser = m_newUser;
	m_app->saveUserStatus();
	m_app->userAdded = true;

	logPrintf("Added new user: %s\n", m_newUser);

	// Add user to dropdown then update the user/session
	if(notNull(m_userDropDown)) m_userDropDown->append(m_newUser);
	m_ddCurrUserIdx = m_users.users.length() - 1;
	
	updateUser(m_newUser);
}

void UserMenu::updateReticlePreview() {
	if (!m_reticlePreviewPane) return;
	// Clear the pane
	m_reticlePreviewPane->removeAllChildren();
	// Redraw the preview
	shared_ptr<Texture> reticleTex = m_app->reticleTexture;
	Color4 rColor = m_users.getUserById(m_userStatus.currentUser)->reticle.color[0];

	RenderDevice* rd = m_app->renderDevice;
	rd->push2D(m_reticleBuffer); {
		Args args;
		args.setMacro("HAS_TEXTURE", 1);
		args.setUniform("textureMap", reticleTex, Sampler::video());
		args.setUniform("color", rColor);
		debugAssertGLOk();

		args.setUniform("gammaAdjust", 1.0f);
		args.setRect(reticleTex->rect2DBounds(), 0);
		LAUNCH_SHADER_WITH_HINT("unlit.*", args, "ReticlePreview");
	} rd->pop2D();

	auto preview = m_reticlePreviewPane->addTextureBox(m_app, m_reticleBuffer->texture(Framebuffer::AttachmentPoint::COLOR0), true);
	preview->setSize(m_reticlePreviewSize);
	preview->zoomToFit();
	m_reticlePreviewPane->pack();
	m_currentUserPane->pack();
}

void UserMenu::updateSessionPress() {
	m_app->updateSession(selectedSession());
}

void UserMenu::updateCmp360() {
	const UserConfig user = m_users.users[m_users.getUserIndex(m_userStatus.currentUser)];
	m_cmp360 = 36.0/user.mouseDegPerMm;
}

void UserMenu::setVisible(bool enable) {
	GuiWindow::setVisible(enable);
}

void UserMenu::resumePress() {
	if (m_config.requireUserAdd && !m_app->userAdded) return;
	setVisible(!visible());
	m_app->setMouseInputMode(visible() ? FPSciApp::MouseInputMode::MOUSE_CURSOR : FPSciApp::MouseInputMode::MOUSE_FPM);
}