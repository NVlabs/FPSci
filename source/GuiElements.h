#pragma once
#include <G3D/G3D.h>
#include "ConfigFiles.h"
#include "TargetEntity.h"

class FPSciApp;

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
	PlayerControls(SessionConfig& config, std::function<void()> exportCallback,
		const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);

public:
	static shared_ptr<PlayerControls> create(SessionConfig& config, std::function<void()> exportCallback,
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

	RenderControls(FPSciApp* app, SessionConfig& config, bool& drawFps, bool& turbo, const int numReticles, float& brightness,
		const shared_ptr<GuiTheme>& theme, const int maxFrameDelay = 360, const float minFrameRate = 1.0f, const float maxFrameRate=1000.0f, float width=400.0f, float height=10.0f);
public:
	static shared_ptr<RenderControls> create(FPSciApp* app, SessionConfig& config, bool& drawFps, bool& turbo, const int numReticles, float& brightness,
		const shared_ptr<GuiTheme>& theme, const int maxFrameDelay = 360, const float minFrameRate = 1.0f, const float maxFrameRate=1000.0f, float width = 400.0f, float height = 10.0f) {
		return createShared<RenderControls>(app, config, drawFps, turbo, numReticles, brightness, theme, maxFrameDelay, minFrameRate, maxFrameRate, width, height);
	}
};

class WeaponControls : public GuiWindow {
protected:
	WeaponControls(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f);
public:
	static shared_ptr<WeaponControls> create(WeaponConfig& config, const shared_ptr<GuiTheme>& theme, float width = 400.0f, float height = 10.0f) {
		return createShared<WeaponControls>(config, theme, width, height);
	}
};

class UserMenu : public GuiWindow {
protected:
	FPSciApp* m_app = nullptr;									///< Store the app here
	UserTable& m_users;										///< User table
	UserStatusTable& m_userStatus;							///< User status table
	MenuConfig m_config;									///< Menu configuration

	GuiPane* m_parent				= nullptr;				///< Parent pane
	GuiPane* m_expPane				= nullptr;				///< Pane for session/user selection
	GuiPane* m_currentUserPane		= nullptr;				///< Pane for current user controls
	GuiPane* m_reticlePreviewPane	= nullptr;				///< Reticle preview pane
	GuiPane* m_resumeQuitPane		= nullptr;				///< Pane for resume/quit buttons

	GuiDropDownList* m_userDropDown = nullptr;				///< Dropdown menu for user selection
	GuiDropDownList* m_sessDropDown = nullptr;				///< Dropdown menu for session selection

	shared_ptr<Texture> m_reticlePreviewTexture;			///< Reticle preview texture
	shared_ptr<Framebuffer> m_reticleBuffer;				///< Reticle preview framebuffer

	int m_ddCurrUserIdx = 0;								///< Current user index
	int m_ddCurrSessIdx = 0;								///< Current session index
	int m_lastUserIdx = -1;									///< Previously selected user in the drop-down

	const Vector2 m_btnSize = { 100.f, 30.f };				///< Default button size
	const Vector2 m_reticlePreviewSize = { 150.f, 150.f };	///< Reticle texture preview size
	const float m_sliderWidth = 300.f;						///< Default width for (non-RGB) sliders
	const float m_rgbSliderWidth = 80.f;					///< Default width for RGB sliders

	UserMenu(FPSciApp* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect);

	/** Creates a GUI Pane for the specified user allowing changeable paramters to be changed */
	void drawUserPane(const MenuConfig& config, UserConfig& user);

	void updateUserPress();
	void updateSessionPress();

public:
	static shared_ptr<UserMenu> create(FPSciApp* app, UserTable& users, UserStatusTable& userStatus, MenuConfig& config, const shared_ptr<GuiTheme>& theme, const Rect2D& rect) {
		return createShared<UserMenu>(app, users, userStatus, config, theme, rect);
	}

	void setVisible(bool visibile);
	/** Resets session drop down clearing completed sessions and adding any new sessions. */
	Array<String> updateSessionDropDown();
	void updateReticlePreview();

	void toggleVisibliity() {
		setVisible(!visible());
	}

	void setSelectedSession(const String& id) {
		m_sessDropDown->setSelectedValue(id);
	}

	String selectedSession() const {
		if (m_ddCurrSessIdx == -1) return "";
		return m_sessDropDown->get(m_ddCurrSessIdx);
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
};