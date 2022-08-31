#pragma once
#include <G3D/G3D.h>
#include "UserConfig.h"
#include "UserStatus.h"
#include "TargetEntity.h"
#include "Weapon.h"
#include "FPSciAnyTableReader.h"

class FPSciApp;
class FpsConfig;

class MenuConfig {
public:
	// Menu controls
	bool showMenuLogo = true;									///< Show the FPSci logo in the user menu
	bool showExperimentSettings = true;							///< Show the experiment settings options (session/user selection)
	bool showUserSettings = true;								///< Show the user settings options (master switch)
	bool allowSessionChange = true;								///< Allow the user to change the session with the menu drop-down
	bool allowUserAdd = false;									///< Allow the user to add a new user to the experiment
	bool requireUserAdd = false;								///< Require a new user to be created
	bool allowUserSettingsSave = true;							///< Allow the user to save settings changes
	bool allowSensitivityChange = true;							///< Allow in-game sensitivity change		

	bool allowTurnScaleChange = true;							///< Allow the user to apply X/Y turn scaling
	String xTurnScaleAdjustMode = "None";						///< X turn scale adjustment mode (can be "None" or "Slider")
	String yTurnScaleAdjustMode = "Invert";						///< Y turn scale adjustment mode (can be "None", "Invert", or "Slider")

	bool allowReticleChange = false;							///< Allow the user to adjust their crosshair
	bool allowReticleIdxChange = true;							///< If reticle change is allowed, allow index change
	bool allowReticleSizeChange = true;							///< If reticle change is allowed, allow size change
	bool allowReticleColorChange = true;						///< If reticle change is allowed, allow color change
	bool allowReticleChangeTimeChange = false;					///< Allow the user to change the reticle change time
	bool showReticlePreview = true;								///< Show a preview of the reticle

	bool showMenuOnStartup = true;								///< Show the user menu on startup?
	bool showMenuBetweenSessions = true;						///< Show the user menu between session?

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, const bool forceAll = false) const;
	bool allowAnyChange() const;
};

struct WaypointDisplayConfig {
	// Formatting parameters
	int tree_display_width_px = 400;
	int tree_display_height_px = 400;
	float tree_height = 15;
	float tree_indent = 16;
	float idx_column_width_px = 50;
	float time_column_width_px = 100;
	float xyz_column_width_px = 250;

	WaypointDisplayConfig(	int width = 400, 
							int height = 400, 
							float line_height=15, 
							float indent = 16, 
							float idx_column_width = 50, 
							float time_column_width = 100, 
							float xyz_column_width = 250) {
		tree_display_width_px = width;
		tree_display_height_px = height;
		tree_height = line_height;
		tree_indent = indent;
		idx_column_width_px = idx_column_width;
		time_column_width_px = time_column_width;
		xyz_column_width_px = xyz_column_width;
	}
};

class WaypointDisplay : public GuiWindow {
protected:

	// Tree class (copied from ProfilerWindow)
	class TreeDisplay : public GuiControl {
	public:
		shared_ptr<GFont> m_icon;
		int m_selectedIdx = -1;
		WaypointDisplayConfig m_config;
		shared_ptr<Array<Destination>> m_waypoints;

		virtual bool onEvent(const GEvent& event) override;
		TreeDisplay(GuiWindow* w, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints);
		virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;

	};

	GuiScrollPane*	m_scrollPane;
	TreeDisplay*	m_treeDisplay;
	FPSciApp* m_app;

	WaypointDisplay(FPSciApp* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints);
public:

	int getSelected() {
		return m_treeDisplay->m_selectedIdx;
	}

	void setSelected(int idx) {
		m_treeDisplay->m_selectedIdx = idx;
	}

	virtual void setManager(WidgetManager* manager);
	static shared_ptr<WaypointDisplay> create(FPSciApp* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints) {
		return createShared<WaypointDisplay>(app, theme, config, waypoints);

	}
};

class PlayerControls : public GuiWindow {
protected:
	PlayerControls(FpsConfig& config, std::function<void()> exportCallback,
		const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);

public:
	static shared_ptr<PlayerControls> create(FpsConfig& config, std::function<void()> exportCallback,
		const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f) {
		return createShared<PlayerControls>(config, exportCallback, theme, width, height);
	}
};

class RenderControls : public GuiWindow {
protected:
	FPSciApp* m_app = nullptr;
	GuiButton* m_showFullUserMenuBtn = nullptr;
	MenuConfig m_storedMenuConfig;
	bool	m_showFullUserMenu = false;

	void updateUserMenu(void);

	RenderControls(FPSciApp* app, FpsConfig& config, bool& drawFps, const int numReticles, float& brightness,
		const shared_ptr<GuiTheme>& theme, const int maxFrameDelay = 360, const float minFrameRate = 1.0f, const float maxFrameRate=1000.0f, float width=400.0f, float height=10.0f);
public:
	static shared_ptr<RenderControls> create(FPSciApp* app, FpsConfig& config, bool& drawFps, const int numReticles, float& brightness,
		const shared_ptr<GuiTheme>& theme, const int maxFrameDelay = 360, const float minFrameRate = 1.0f, const float maxFrameRate=1000.0f, float width = 400.0f, float height = 10.0f) {
		return createShared<RenderControls>(app, config, drawFps, numReticles, brightness, theme, maxFrameDelay, minFrameRate, maxFrameRate, width, height);
	}
};

class WeaponControls : public GuiWindow {
protected:
	int	m_spreadShapeIdx = 0;											// Index of fire spread shape
	const Array<String> m_spreadShapes = { "uniform", "gaussian" };		// Optional shapes to select from
	void updateFireSpreadShape(void);

	WeaponConfig& m_config;

	WeaponControls(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);
public:
	static shared_ptr<WeaponControls> create(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f) {
		return createShared<WeaponControls>(config, theme, width, height);
	}
};


class UserMenu : public GuiWindow {
protected:
	FPSciApp* m_app = nullptr;									///< Store the app here
	UserTable& m_users;											///< User table
	UserStatusTable& m_userStatus;								///< User status table
	MenuConfig m_config;										///< Menu configuration

	GuiPane* m_parent					= nullptr;				///< Parent pane
	GuiPane* m_expPane					= nullptr;				///< Pane for session/user selection
	GuiPane* m_currentUserPane			= nullptr;				///< Pane for current user controls
	GuiPane* m_reticlePreviewPane		= nullptr;				///< Reticle preview pane
	GuiPane* m_resumeQuitPane			= nullptr;				///< Pane for resume/quit buttons

	GuiDropDownList* m_userDropDown		= nullptr;				///< Dropdown menu for user selection
	GuiDropDownList* m_sessDropDown		= nullptr;				///< Dropdown menu for session selection
	GuiLabel* m_newUserFeedback			= nullptr;				///< Feedback field for new user

	shared_ptr<Texture> m_reticlePreviewTexture;				///< Reticle preview texture
	shared_ptr<Framebuffer> m_reticleBuffer;					///< Reticle preview framebuffer

	int m_ddCurrUserIdx = 0;									///< Current user index
	int m_ddCurrSessIdx = 0;									///< Current session index
	int m_ddLastUserIdx = -1;									///< Previously selected user in the drop-down

	String m_newUser;											///< New user string

	double	m_cmp360;											///< cm/360° setting

	const Vector2 m_btnSize = { 100.f, 30.f };					///< Default button size
	const Vector2 m_reticlePreviewSize = { 150.f, 150.f };		///< Reticle texture preview size
	const float m_sliderWidth = 300.f;							///< Default width for (non-RGB) sliders
	const float m_rgbSliderWidth = 80.f;						///< Default width for RGB sliders

	UserMenu(FPSciApp* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect);

	/** Creates a GUI Pane for the specified user allowing changeable paramters to be changed */
	void drawUserPane(const MenuConfig& config, UserConfig& user);

	void updateUser(const String& id);
	void updateUserPress();
	void addUserPress();
	void updateSessionPress();
	void updateExperimentPress();
	void resumePress();

public:
	static shared_ptr<UserMenu> create(FPSciApp* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect) {
		return createShared<UserMenu>(app, users, userStatus, config, theme, rect);
	}

	void setVisible(bool visibile);
	/** Resets session drop down clearing completed sessions and adding any new sessions. */
	Array<String> updateSessionDropDown();
	void updateReticlePreview();

	void setSelectedSession(const String& id) {
		m_sessDropDown->setSelectedValue(id);
	}

	const String selectedSession() const {
		if (m_ddCurrSessIdx == -1 || m_ddCurrSessIdx >= m_sessDropDown->numElements()) return "";
		return m_sessDropDown->get(m_ddCurrSessIdx).text();
	}

	int sessionsForSelectedUser() const {
		return m_sessDropDown->numElements();
	}

	String selectedUserID() const {
		return m_userDropDown->get(m_ddCurrUserIdx);
	}

	shared_ptr<UserConfig> getCurrUser() {
		return m_users.getUserById(selectedUserID());
	}

	/** updates the displayed cmp360 value based on the current setting */
	void updateCmp360();
};