#include "GuiElements.h"
#include "WaypointManager.h"
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
	const shared_ptr<GuiTheme>& theme, const int maxFrameDelay, const float minFrameRate, const float maxFrameRate, float width, float height) :
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
		auto c = framePane->addNumberBox("Framerate", &(config.render.frameRate), "fps", GuiTheme::LINEAR_SLIDER, minFrameRate, maxFrameRate, 1.0f);
		c->setWidth(width*0.95f);
	} framePane->endRow();
	framePane->beginRow(); {
		auto c = framePane->addNumberBox("Display Lag", &(config.render.frameDelay), "f", GuiTheme::LINEAR_SLIDER, 0, maxFrameDelay);
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

////////////////////////
/// USER MENU
///////////////////////
UserMenu::UserMenu(App* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect) :
	GuiWindow("", theme, rect, GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE), m_app(app), m_users(users), m_userStatus(userStatus), m_config(config)
{
	m_reticlePreviewTexture = Texture::createEmpty("FPSci::ReticlePreview", m_app->reticleTexture->width(), m_app->reticleTexture->height());
	m_reticleBuffer = Framebuffer::create(m_reticlePreviewTexture);

	m_parent = pane();
	updateMenu(config);
	pack();
}

void UserMenu::updateMenu(const MenuConfig& config) 
{	
	// Clear the menu
	m_parent->removeAllChildren();

	GuiTextureBox* logoTb = nullptr;

	// Add logo
	if (config.showMenuLogo) {
		auto logo = Texture::fromFile("material/FPSciLogo.png");
		logoTb = m_parent->addTextureBox(m_app, "", logo, true);
		logoTb->setSize(m_logoSize);
		logoTb->zoomToFit();
		logoTb->setEnabled(false);
	}

	// Experiment Settings Pane
	m_ddCurrUserIdx = m_users.getCurrentUserIndex();
	if (config.showExperimentSettings) {
		m_expPane = m_parent->addPane("Experiment Settings");
		m_expPane->setCaptionHeight(40);
		m_expPane->beginRow(); {
			m_userDropDown = m_expPane->addDropDownList("User", m_users.getIds(), &m_ddCurrUserIdx);
			m_expPane->addButton("Select User", this, &UserMenu::updateUserPress);
		} m_expPane->endRow();
		m_expPane->beginRow(); {
			m_sessDropDown = m_expPane->addDropDownList("Session", Array<String>({}), &m_ddCurrSessIdx);
			updateSessionDropDown();
			m_expPane->addButton("Select Session", this, &UserMenu::updateSessionPress);
		} m_expPane->endRow();
	}

	// User Settings Pane
	if (config.showUserSettings) {
		m_currentUserPane = m_parent->addPane("Current User Settings");
		updateUserPane(config);
	}

	// Resume/Quite Pane
	GuiButton* resumeBtn = nullptr;
	GuiButton* quitBtn = nullptr;
	m_resumeQuitPane = m_parent->addPane();
	m_resumeQuitPane->beginRow(); {
		const Vector2 resumeQuitBtnSize = { 100.f, 40.f };
		// Create resume and quit buttons
		resumeBtn = m_resumeQuitPane->addButton("Resume", this, &UserMenu::toggleVisibliity, GuiTheme::TOOL_BUTTON_STYLE);
		resumeBtn->setSize(resumeQuitBtnSize);
		quitBtn = m_resumeQuitPane->addButton("Quit", m_app, &App::quitRequest, GuiTheme::TOOL_BUTTON_STYLE);
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

void UserMenu::updateUserPane(const MenuConfig& config) 
{
	// Clear the pane
	m_currentUserPane->removeAllChildren();

	// Basic user info
	UserConfig* user = m_users.getCurrentUser();
	m_currentUserPane->beginRow(); {
		m_currentUserPane->addLabel(format("Current User: %s", m_users.currentUser))->setHeight(30.0);
	} m_currentUserPane->endRow();
	m_currentUserPane->beginRow(); {
		m_currentUserPane->addLabel(format("Mouse DPI: %f", user->mouseDPI));
	} m_currentUserPane->endRow();
	m_currentUserPane->beginRow(); {
		auto sensitivityNb = m_currentUserPane->addNumberBox("Mouse 360", &(user->cmp360), "cm", GuiTheme::LINEAR_SLIDER, 0.2, 100.0, 0.2);
		sensitivityNb->setWidth(300.0);
		sensitivityNb->setEnabled(config.allowSensitivityChange);
	} m_currentUserPane->endRow();

	if (config.allowTurnScaleChange) {
		// X turn scale
		if (config.xTurnScaleAdjustMode != "None") {
			m_currentUserPane->beginRow(); {
				m_currentUserPane->addNumberBox("Turn Scale X", &(user->turnScale.x), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f)->setWidth(m_sliderWidth);
			} m_currentUserPane->endRow();
		}
		// Y turn scale
		if (config.yTurnScaleAdjustMode != "None") {
			m_currentUserPane->beginRow(); {
				if (config.yTurnScaleAdjustMode == "Slider") {
					m_currentUserPane->addNumberBox("Turn Scale Y", &(user->turnScale.y), "x", GuiTheme::LINEAR_SLIDER, -10.0f, 10.0f, 0.1f)->setWidth(m_sliderWidth);
				}
				else if (config.yTurnScaleAdjustMode == "Invert") {
					m_currentUserPane->addCheckBox("Invert Y", &(user->invertY));

				}
			} m_currentUserPane->endRow();
		}
	}

	// Reticle configuration
	if (config.allowReticleChange) {
		auto reticleControlPane = m_currentUserPane->addPane("Reticle Control");
		const float reticleCaptionWidth = 120.f;

		// Reticle index selection
		if (config.allowReticleIdxChange) {
			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle", &(user->reticleIndex), "", GuiTheme::LINEAR_SLIDER, 0, m_app->numReticles, 1);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();
		}

		// Reticle size selection
		if (config.allowReticleSizeChange) {
			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle Scale Min", &(user->reticleScale[0]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();

			reticleControlPane->beginRow(); {
				auto c = reticleControlPane->addNumberBox("Reticle Scale Max", &(user->reticleScale[1]), "x", GuiTheme::LINEAR_SLIDER, 0.01f, 3.0f, 0.01f);
				c->setCaptionWidth(reticleCaptionWidth);
				c->setWidth(m_sliderWidth);
			} reticleControlPane->endRow();
		}

		// Reticle color selection
		if (config.allowReticleColorChange) {
			const float rgbCaptionWidth = 10.f;
			reticleControlPane->beginRow(); {
				reticleControlPane->addLabel("Reticle Color Min")->setWidth(120.f);
				auto r = reticleControlPane->addSlider("R", &(user->reticleColor[0].r), 0.0f, 1.0f);
				r->setCaptionWidth(rgbCaptionWidth);
				r->setWidth(m_rgbSliderWidth);
				auto g = reticleControlPane->addSlider("G", &(user->reticleColor[0].g), 0.0f, 1.0f);
				g->setCaptionWidth(rgbCaptionWidth);
				g->setWidth(m_rgbSliderWidth);
				g->moveRightOf(r, rgbCaptionWidth);
				auto b = reticleControlPane->addSlider("B", &(user->reticleColor[0].b), 0.0f, 1.0f);
				b->setCaptionWidth(rgbCaptionWidth);
				b->setWidth(m_rgbSliderWidth);
				b->moveRightOf(g, rgbCaptionWidth);
				auto a = reticleControlPane->addSlider("A", &(user->reticleColor[0].a), 0.0f, 1.0f);
				a->setCaptionWidth(rgbCaptionWidth);
				a->setWidth(m_rgbSliderWidth);
				a->moveRightOf(b, rgbCaptionWidth);
			} reticleControlPane->endRow();
			reticleControlPane->beginRow(); {
				reticleControlPane->addLabel("Reticle Color Max")->setWidth(120.f);
				auto r = reticleControlPane->addSlider("R", &(user->reticleColor[1].r), 0.0f, 1.0f);
				r->setCaptionWidth(rgbCaptionWidth);
				r->setWidth(m_rgbSliderWidth);
				auto g = reticleControlPane->addSlider("G", &(user->reticleColor[1].g), 0.0f, 1.0f);
				g->setCaptionWidth(rgbCaptionWidth);
				g->setWidth(m_rgbSliderWidth);
				g->moveRightOf(r, rgbCaptionWidth);
				auto b = reticleControlPane->addSlider("B", &(user->reticleColor[1].b), 0.0f, 1.0f);
				b->setCaptionWidth(rgbCaptionWidth);
				b->setWidth(m_rgbSliderWidth);
				b->moveRightOf(g, rgbCaptionWidth);
				auto a = reticleControlPane->addSlider("A", &(user->reticleColor[1].a), 0.0f, 1.0f);
				a->setCaptionWidth(rgbCaptionWidth);
				a->setWidth(m_rgbSliderWidth);
				a->moveRightOf(b, rgbCaptionWidth);
			} reticleControlPane->endRow();
			if (config.allowReticleTimeChange) {
				reticleControlPane->beginRow(); {
					auto c = reticleControlPane->addNumberBox("Reticle Shrink Time", &(user->reticleShrinkTimeS), "s", GuiTheme::LINEAR_SLIDER, 0.0f, 5.0f, 0.01f);
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
			m_currentUserPane->addButton("Save settings", m_app, &App::userSaveButtonPress)->setSize(m_btnSize);
		} m_currentUserPane->endRow();
	}

	m_currentUserPane->pack();
	pack();
}

Array<String> UserMenu::updateSessionDropDown() {
	// Create updated session list
	String userId = m_users.getCurrentUser()->id;
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
		m_userStatus.toAny().save("userstatus.Any");
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

	// Print message to log
	logPrintf("Updated %s's session drop down to:\n", userId);
	for (String id : remainingSess) {
		logPrintf("\t%s\n", id);
	}

	return remainingSess;
}

void UserMenu::updateUserPress() {
	if (m_lastUserIdx != m_ddCurrUserIdx) {
		// Update user ID
		String userId = m_userDropDown->get(m_ddCurrUserIdx);
		m_users.currentUser = userId;
		m_lastUserIdx = m_ddCurrUserIdx;
		updateUserPane(m_config);
		
		// Update (selected) sessions
		String sessId = updateSessionDropDown()[0];
		if (m_sessDropDown->numElements() > 0) m_app->updateSession(sessId);
	}
	//updateSessionDropDown();
}

void UserMenu::updateReticlePreview() {
	if (!m_reticlePreviewPane) return;
	// Clear the pane
	m_reticlePreviewPane->removeAllChildren();
	// Redraw the preview
	shared_ptr<Texture> reticleTex = m_app->reticleTexture;
	Color4 rColor = m_users.getCurrentUser()->reticleColor[0];

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
}

void UserMenu::updateSessionPress() {
	m_app->updateSession(selectedSession());
}

void UserMenu::setVisible(bool enable) {
	GuiWindow::setVisible(enable);
	// Set view control (direct) vs pointer (indirect) based on window visibility
	m_app->setDirectMode(!enable);
}
