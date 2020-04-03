#pragma once

#include <G3D/G3D.h>

// Floating combat text floats in the location spawned
class FloatingCombatText : public VisibleEntity {
protected:
	shared_ptr<GFont> m_font;
	String m_text;
	float m_size;
	Color4 m_color;
	Color4 m_outline;
	Point3 m_offset;
	Point3 m_velocity;
	float m_fade;
	float m_timeout;
	RealTime m_created;

public:
	/** Create floating text that fades out over over timeout_s seconds */
	static shared_ptr<FloatingCombatText> create(String text, shared_ptr<GFont> font, float size, Color4 color, Color4 outlineColor, Point3 offset, Point3 velocity, float fade, float timeout_s) {
		return  createShared<FloatingCombatText>(text, font, size, color, outlineColor, offset, velocity, fade, timeout_s);
	}
	FloatingCombatText(String text, shared_ptr<GFont> font, float size, Color4 color, Color4 outlineColor, Point3 offset, Point3 velocity, float fade, float timeout_s) {
		m_text = text;
		m_font = font;
		m_size = size;
		m_color = color;
		m_outline = outlineColor;
		m_offset = offset;
		m_velocity = velocity;
		m_fade = fade;
		m_timeout = timeout_s;
		m_created = System::time();		// Capture the time at which this was created
	}

	bool draw(RenderDevice* rd, const Camera& camera, const Framebuffer& framebuffer) {
		// Abort if the timeout has expired (return false to remove this combat text from the tracked array)
		float time_existing = static_cast<float>(System::time() - m_created);
		if (time_existing > m_timeout) {
			return false;
		}

		// Project entity position into image space
		Rect2D viewport = Rect2D(framebuffer.vector2Bounds());
		Point3 position = camera.project(frame().translation, viewport);

		// Abort if the target is not in front of the camera 
		Vector3 diffVector = frame().translation - camera.frame().translation;
		if (camera.frame().lookRay().direction().dot(diffVector) < 0.0f) {
			return true;
		}
		// Abort if the target is not in the view frustum
		if (position == Point3::inf()) {
			return true;
		}
		position += m_offset;						// Apply (initial) offset in pixels
		position += time_existing * m_velocity;		// Update the position based on velocity

		// Apply the fade
		m_color.a *= m_fade;
		m_outline.a *= m_fade;

		// Draw the font
		m_font->draw2D(rd, m_text, position.xy(), m_size, m_color, m_outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		return true;
	}
};
