/** \file FPSciAppGraphics.cpp */
#include "FPSciApp.h"
#include "WaypointManager.h"

void FPSciApp::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
	debugAssertGLOk();

	rd->pushState(); {
		debugAssert(notNull(activeCamera()));
		rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
		debugAssertGLOk();
		onGraphics3D(rd, posed3D);
	} rd->popState();

	if (notNull(screenCapture())) {
		screenCapture()->onAfterGraphics3D(rd);
	}

	rd->push2D(); {
		onGraphics2D(rd, posed2D);
	} rd->pop2D();

	if (notNull(screenCapture())) {
		screenCapture()->onAfterGraphics2D(rd);
	}
}

void FPSciApp::updateShaderBuffers() {
	// Parameters for update/resize of buffers
	const int width = renderDevice->width();
	const int height = renderDevice->height();
	const ImageFormat* framebufferFormat = m_framebuffer->texture(0)->format();

	// 2D buffers (input and output)
	if (!sessConfig->render.shader2D.empty()) {
		m_buffer2D = Framebuffer::create(Texture::createEmpty("FPSci::2DBuffer", width, height, ImageFormat::RGBA8(), Texture::DIM_2D, true));
		m_shader2DOutput = Framebuffer::create(Texture::createEmpty("FPSci::2DShaderPass::Output", width, height));
	}

	// 3D shader output (use popped framebuffer as input)
	if (!sessConfig->render.shader3D.empty()) {
		m_shader3DOutput = Framebuffer::create(Texture::createEmpty("FPSci::3DShaderPass::Output", width, height, framebufferFormat));
	}

	// Composite buffer (input and output)
	if (!sessConfig->render.shaderComposite.empty()) {
		m_bufferComposite = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Input", width, height, framebufferFormat, Texture::DIM_2D, true));
		m_shaderCompositeOutput = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Output", width, height, framebufferFormat));
	}
}

void FPSciApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) {

    if (displayLagFrames > 0) {
		// Need one more frame in the queue than we have frames of delay, to hold the current frame
		if (m_ldrDelayBufferQueue.size() <= displayLagFrames) {
			// Allocate new textures
			for (int i = displayLagFrames - m_ldrDelayBufferQueue.size(); i >= 0; --i) {
				m_ldrDelayBufferQueue.push(Framebuffer::create(Texture::createEmpty(format("Delay buffer %d", m_ldrDelayBufferQueue.size()), rd->width(), rd->height(), ImageFormat::RGB8())));
			}
			debugAssert(m_ldrDelayBufferQueue.size() == displayLagFrames + 1);
		}

		// When the display lag changes, we must be sure to be within range
		m_currentDelayBufferIndex = min(displayLagFrames, m_currentDelayBufferIndex);

		rd->pushState(m_ldrDelayBufferQueue[m_currentDelayBufferIndex]);
	}

	scene()->lightingEnvironment().ambientOcclusionSettings.enabled = !emergencyTurbo;
	playerCamera->filmSettings().setAntialiasingEnabled(!emergencyTurbo);
	playerCamera->filmSettings().setBloomStrength(emergencyTurbo ? 0.0f : 0.5f);

	GApp::onGraphics3D(rd, surface);

	if (displayLagFrames > 0) {
		// Display the delayed frame
		rd->popState();
		// Render into the frame buffer or the composite shader input buffer (if provided)
		rd->push2D(); {
			// Advance the pointer to the next, which is also the oldest frame
			m_currentDelayBufferIndex = (m_currentDelayBufferIndex + 1) % (displayLagFrames + 1);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrDelayBufferQueue[m_currentDelayBufferIndex]->texture(0), Sampler::buffer());
		} rd->pop2D();
	}
}


void FPSciApp::onPostProcessHDR3DEffects(RenderDevice* rd) {
	// Put elements that should be delayed along w/ 3D here
	if (!sessConfig->render.shader2D.empty()) {
		// If rendering into a split (offscreen) 2D buffer, then clear it/render to it here
		m_buffer2D->texture(0)->clear();
		rd->push2D(m_buffer2D);
	}
	else {
		// Otherwise render to the framebuffer
		rd->push2D();
	}
	{
		rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
		const float scale = rd->viewport().width() / 1920.0f;

		// Draw target health bars
		if (sessConfig->targetView.showHealthBars) {
			for (auto const& target : sess->targetArray()) {
				target->drawHealthBar(rd, *activeCamera(), *m_framebuffer,
					sessConfig->targetView.healthBarSize,
					sessConfig->targetView.healthBarOffset,
					sessConfig->targetView.healthBarBorderSize,
					sessConfig->targetView.healthBarColors,
					sessConfig->targetView.healthBarBorderColor);
			}
		}

		// Draw the combat text
		if (sessConfig->targetView.showCombatText) {
			Array<int> toRemove;
			for (int i = 0; i < m_combatTextList.size(); i++) {
				bool remove = !m_combatTextList[i]->draw(rd, *playerCamera, *m_framebuffer);
				if (remove) m_combatTextList[i] = nullptr;		// Null pointers to remove
			}
			// Remove the expired elements here
			m_combatTextList.removeNulls();
		}

		if (sessConfig->clickToPhoton.enabled && sessConfig->clickToPhoton.mode == "total") {
			drawClickIndicator(rd, "total");
		}

		// Draw the HUD here
		if (sessConfig->hud.enable) {
			drawHUD(rd);
		}
	}rd->pop2D();

	if (!sessConfig->render.shader3D.empty() && m_shaderTable.containsKey(sessConfig->render.shader3D)) {
		BEGIN_PROFILER_EVENT_WITH_HINT("3D Shader Pass", "Time to run the post-3D shader pass");

		rd->push2D(m_shader3DOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_framebuffer->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_lastTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shader3D], args);
			m_lastTime = iTime;
		} rd->pop2D();

		// Copy the shader output buffer into the framebuffer
		sessConfig->render.shaderComposite.empty() ? rd->push2D() : rd->push2D(m_bufferComposite); {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shader3DOutput->texture(0), Sampler::buffer());
		} rd->pop2D();
		END_PROFILER_EVENT();
	}

	GApp::onPostProcessHDR3DEffects(rd);
}

void FPSciApp::drawClickIndicator(RenderDevice *rd, String mode) {
	// Click to photon latency measuring corner box
	if (sessConfig->clickToPhoton.enabled) {
		float boxLeft = 0.0f;
		// Paint both sides by the width of latency measuring box.
		Point2 latencyRect = sessConfig->clickToPhoton.size * Point2((float)rd->viewport().width(), (float)rd->viewport().height());
		float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		if (guardband) {
			boxLeft += guardband;
		}
		float boxTop = rd->viewport().height()*sessConfig->clickToPhoton.vertPos - latencyRect.y / 2;
		if (sessConfig->clickToPhoton.mode == "both") {
			boxTop = (mode == "minimum") ? boxTop - latencyRect.y : boxTop + latencyRect.y;
		}
		if (sessConfig->clickToPhoton.side == "right") {
			boxLeft = (float)rd->viewport().width() - (guardband + latencyRect.x);
		}
		// Draw the "active" box
		Color3 boxColor;
		if (sessConfig->clickToPhoton.mode == "frameRate") {
			boxColor = (frameToggle) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
			frameToggle = !frameToggle;
		}
		else boxColor = (shootButtonUp) ? sessConfig->clickToPhoton.colors[0] : sessConfig->clickToPhoton.colors[1];
		Draw::rect2D(Rect2D::xywh(boxLeft, boxTop, latencyRect.x, latencyRect.y), rd, boxColor);
	}
}

void FPSciApp::drawHUD(RenderDevice *rd) {
	// Scale is used to position/resize the "score banner" when the window changes size in "windowed" mode (always 1 in fullscreen mode).
	const Vector2 scale = rd->viewport().wh() / displayRes;

	RealTime now = m_lastOnSimulationRealTime;

	// Weapon ready status (cooldown indicator)
	if (sessConfig->hud.renderWeaponStatus) {
		// Draw the "active" cooldown box
		if (sessConfig->hud.cooldownMode == "box") {
			float boxLeft = (float)rd->viewport().width() * 0.0f;
			if (sessConfig->hud.weaponStatusSide == "right") {
				// swap side
				boxLeft = (float)rd->viewport().width() * (1.0f - sessConfig->clickToPhoton.size.x);
			}
			Draw::rect2D(
				Rect2D::xywh(
					boxLeft,
					(float)rd->viewport().height() * (float)(weapon->cooldownRatio(now)),
					(float)rd->viewport().width() * sessConfig->clickToPhoton.size.x,
					(float)rd->viewport().height() * (float)(1.0 - weapon->cooldownRatio(now))
				), rd, Color3::white() * 0.8f
			);
		}
		else if (sessConfig->hud.cooldownMode == "ring") {
			// Draw cooldown "ring" instead of box
			const float iRad = sessConfig->hud.cooldownInnerRadius;
			const float oRad = iRad + sessConfig->hud.cooldownThickness;
			const int segments = sessConfig->hud.cooldownSubdivisions;
			int segsToLight = static_cast<int>(ceilf((1 - weapon->cooldownRatio(now))*segments));
			// Create the segments
			for (int i = 0; i < segsToLight; i++) {
				const float inc = static_cast<float>(2 * pi() / segments);
				const float theta = -i * inc;
				Vector2 center = Vector2(rd->viewport().width() / 2.0f, rd->viewport().height() / 2.0f);
				Array<Vector2> verts = {
					center + Vector2(oRad*sin(theta), -oRad * cos(theta)),
					center + Vector2(oRad*sin(theta + inc), -oRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta + inc), -iRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta), -iRad * cos(theta))
				};
				Draw::poly2D(verts, rd, sessConfig->hud.cooldownColor);
			}
		}
	}

	// Draw the player health bar
	if (sessConfig->hud.showPlayerHealthBar) {
		const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		const float health = scene()->typedEntity<PlayerEntity>("player")->health();
		const Point2 location = Point2(sessConfig->hud.playerHealthBarPos.x, sessConfig->hud.playerHealthBarPos.y + m_debugMenuHeight) + Point2(guardband, guardband);
		const Point2 size = sessConfig->hud.playerHealthBarSize;
		const Point2 border = sessConfig->hud.playerHealthBarBorderSize;
		const Color4 borderColor = sessConfig->hud.playerHealthBarBorderColor;
		const Color4 color = sessConfig->hud.playerHealthBarColors[1] * (1.0f - health) + sessConfig->hud.playerHealthBarColors[0] * health;

		Draw::rect2D(Rect2D::xywh(location - border, size + border + border), rd, borderColor);
		Draw::rect2D(Rect2D::xywh(location, size*Point2(health, 1.0f)), rd, color);
	}
	// Draw the ammo indicator
	if (sessConfig->hud.showAmmo) {
		const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		Point2 lowerRight = Point2(static_cast<float>(rd->viewport().width()), static_cast<float>(rd->viewport().height())) - Point2(guardband, guardband);
		hudFont->draw2D(rd,
			format("%d/%d", weapon->remainingAmmo(), sessConfig->weapon.maxAmmo),
			lowerRight - sessConfig->hud.ammoPosition,
			sessConfig->hud.ammoSize,
			sessConfig->hud.ammoColor,
			sessConfig->hud.ammoOutlineColor,
			GFont::XALIGN_RIGHT,
			GFont::YALIGN_BOTTOM
		);
	}

	if (sessConfig->hud.showBanner && !emergencyTurbo) {
		const shared_ptr<Texture> scoreBannerTexture = hudTextures["scoreBannerBackdrop"];
		const Point2 hudCenter(rd->viewport().width() / 2.0f, sessConfig->hud.bannerVertVisible*scoreBannerTexture->height() * scale.y + debugMenuHeight());
		Draw::rect2D((scoreBannerTexture->rect2DBounds() * scale - scoreBannerTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), scoreBannerTexture);

		// Create strings for time remaining, progress in sessions, and score
		float remainingTime = sess->getRemainingTrialTime();
		float printTime = remainingTime > 0 ? remainingTime : 0.0f;
		String time_string = format("%0.2f", printTime);
		float prog = sess->getProgress();
		String prog_string = "";
		if (!isnan(prog)) {
			prog_string = format("%d", (int)(100.0f*prog)) + "%";
		}
		String score_string = format("%d", (int)(10 * sess->getScore()));

		hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale.x, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale.x * sessConfig->hud.bannerLargeFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale.x * sessConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}

	// Draw any static HUD elements
	for (StaticHudElement element : sessConfig->hud.staticElements) {
		if (!hudTextures.containsKey(element.filename)) continue;						// Skip any items we haven't loaded
		const shared_ptr<Texture> texture = hudTextures[element.filename];				// Get the loaded texture for this element
		const Vector2 size = element.scale * scale * texture->vector2Bounds();			// Get the final size of the image
		const Vector2 pos = (element.position * rd->viewport().wh()) - size/2.0;		// Compute position (center image on provided position)
		Draw::rect2D(Rect2D::xywh(pos, size), rd, Color3::white(), texture);			// Draw the rect
	}
}


void FPSciApp::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
	// Track the instantaneous frame duration (no smoothing) in a circular queue
	if (m_frameDurationQueue.length() > MAX_HISTORY_TIMING_FRAMES) {
		m_frameDurationQueue.dequeue();
	}
	{
		const float f = rd->stats().frameRate;
		const float t = 1.0f / f;
		m_frameDurationQueue.enqueue(t);
	}

	float recentMin = finf();
	float recentMax = -finf();
	for (int i = 0; i < m_frameDurationQueue.length(); ++i) {
		const float t = m_frameDurationQueue[i];
		recentMin = min(recentMin, t);
		recentMax = max(recentMax, t);
	}

	// Set render buffer depending on whether we are rendering to a sperate 2D buffer
	sessConfig->render.shader2D.empty() ? rd->push2D() : rd->push2D(m_buffer2D); {
		if (sessConfig->render.shader2D.empty()) {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
		}
		else {
			rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		}
		const float scale = rd->viewport().width() / 1920.0f;

		// FPS display (faster than the full stats widget)
		if (renderFPS) {
			String msg;

			if (window()->settings().refreshRate > 0) {
				msg = format("%d measured / %d requested fps",
					iRound(renderDevice->stats().smoothFrameRate),
					window()->settings().refreshRate);
			}
			else {
				msg = format("%d fps", iRound(renderDevice->stats().smoothFrameRate));
			}

			msg += format(" | %.1f min/%.1f avg/%.1f max ms", recentMin * 1000.0f, 1000.0f / renderDevice->stats().smoothFrameRate, 1000.0f * recentMax);
			outputFont->draw2D(rd, msg, Point2(rd->viewport().width()*0.75f, rd->viewport().height()*0.05f).floor(), floor(20.0f*scale), Color3::yellow());
		}

		// Handle recording indicator
		if (startupConfig.waypointEditorMode && waypointManager->recordMotion) {
			Draw::point(Point2(rd->viewport().width()*0.9f - 15.0f, 20.0f+m_debugMenuHeight*scale), rd, Color3::red(), 10.0f);
			outputFont->draw2D(rd, "Recording Position", Point2(rd->viewport().width() - 200.0f , m_debugMenuHeight*scale), 20.0f, Color3::red());
		}

		// Click-to-photon mouse event indicator
		if (sessConfig->clickToPhoton.enabled && sessConfig->clickToPhoton.mode != "total") {
			drawClickIndicator(rd, sessConfig->clickToPhoton.mode);
		}

		// Player camera only indicators
		if (activeCamera() == playerCamera) {
			// Reticle
			const shared_ptr<UserConfig> user = currentUser();
			float tscale = weapon->cooldownRatio(m_lastOnSimulationRealTime, user->reticleChangeTimeS);
			float rScale = tscale * user->reticleScale[0] + (1.0f - tscale)*user->reticleScale[1];
			Color4 rColor = user->reticleColor[1] * (1.0f - tscale) + user->reticleColor[0] * tscale;
			Draw::rect2D(((reticleTexture->rect2DBounds() - reticleTexture->vector2Bounds() / 2.0f))*rScale / 2.0f + rd->viewport().wh() / 2.0f, rd, rColor, reticleTexture);

			// Handle the feedback message
			String message = sess->getFeedbackMessage();
			const float centerHeight = rd->viewport().height() * 0.4f;
			const float scaledFontSize = floor(sessConfig->feedback.fontSize * scale);
			if (!message.empty()) {
				String currLine;
				Array<String> lines = stringSplit(message, '\n');
				float vertPos = centerHeight - (scaledFontSize * 1.5f * lines.length()/ 2.0f);
				// Draw a "back plate"
				Draw::rect2D(Rect2D::xywh(0.0f, 
					vertPos - 1.5f * scaledFontSize,
					rd->viewport().width(), 
					scaledFontSize * (lines.length()+1) * 1.5f),
					rd, sessConfig->feedback.backgroundColor);
				for (String line : lines) {
					outputFont->draw2D(rd, line.c_str(),
						(Point2(rd->viewport().width() * 0.5f, vertPos)).floor(),
						scaledFontSize,
						sessConfig->feedback.color,
						sessConfig->feedback.outlineColor,
						GFont::XALIGN_CENTER, GFont::YALIGN_CENTER
					);
					vertPos += scaledFontSize * 1.5f;
				}
			}
		}

	} rd->pop2D();

	// Handle 2D-only shader here (requires split 2D framebuffer)
	if (!sessConfig->render.shader2D.empty() && m_shaderTable.containsKey(sessConfig->render.shader2D)) {
		BEGIN_PROFILER_EVENT_WITH_HINT("2D Shader Pass", "Time to run the post-2D shader pass");

		rd->push2D(m_shader2DOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_buffer2D->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_last2DTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shader2D], args);
			m_last2DTime = iTime;
		} rd->pop2D();

		// Direct shader output to the display or composite shader input (if specified)
		sessConfig->render.shaderComposite.empty() ? rd->push2D() : rd->push2D(m_bufferComposite); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			//rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shader2DOutput->texture(0), Sampler::video());
		} rd->pop2D();

		END_PROFILER_EVENT();
	}

	//  Handle post-2D composite shader here
	if (!sessConfig->render.shaderComposite.empty() && m_shaderTable.containsKey(sessConfig->render.shaderComposite)) {
		BEGIN_PROFILER_EVENT_WITH_HINT("Composite Shader Pass", "Time to run the composite shader pass");

		// If we haven't run a shader into the composite buffer then blit the framebuffer here now
		if (sessConfig->render.shader2D.empty() && sessConfig->render.shader3D.empty()) {
			rd->readFramebuffer()->blitTo(rd, m_bufferComposite, true, false, false, false);
		}

		rd->push2D(m_shaderCompositeOutput); {
			// Setup shadertoy-style args
			Args args;
			args.setUniform("iChannel0", m_bufferComposite->texture(0), Sampler::video());
			const float iTime = float(System::time() - m_startTime);
			args.setUniform("iTime", iTime);
			args.setUniform("iTimeDelta", iTime - m_lastCompositeTime);
			args.setUniform("iMouse", userInput->mouseXY());
			args.setUniform("iFrame", m_frameNumber);
			args.setRect(rd->viewport());
			LAUNCH_SHADER_PTR(m_shaderTable[sessConfig->render.shaderComposite], args);
			m_lastCompositeTime = iTime;
		} rd->pop2D();

		// Copy the shader output buffer into the framebuffer
		rd->push2D(); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_shaderCompositeOutput->texture(0), Sampler::buffer());
		} rd->pop2D();

		END_PROFILER_EVENT();
	}

	// Might not need this on the reaction trial
	// This is rendering the GUI. Can remove if desired.
	Surface2D::sortAndRender(rd, posed2D);
}
