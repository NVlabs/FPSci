#pragma once
#include <G3D/G3D.h>
#include "TargetEntity.h"

class App;

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
	App* m_app;

	WaypointDisplay(App* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints);
public:

	int getSelected() {
		return m_treeDisplay->m_selectedIdx;
	}

	void setSelected(int idx) {
		m_treeDisplay->m_selectedIdx = idx;
	}

	virtual void setManager(WidgetManager* manager);
	static shared_ptr<WaypointDisplay> create(App* app, const shared_ptr<GuiTheme>& theme, WaypointDisplayConfig config, shared_ptr<Array<Destination>> waypoints);
};