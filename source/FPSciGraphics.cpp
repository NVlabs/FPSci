/** \file FPSciAppGraphics.cpp */
#include "FPSciApp.h"
#include "WaypointManager.h"

void FPSciApp::updateShaderBuffers(const shared_ptr<FpsConfig> config) {
	// Parameters for update/resize of buffers
	int width = renderDevice->width();
	int height = renderDevice->height();

	// 2D buffers (input and output) used when 2D resolution or shader is specified
	if (!config->render.shader2D.empty() || config->render.resolution2D[0] > 0) {
		if (config->render.resolution2D[0] > 0) {
			width = config->render.resolution2D[0];
			height = config->render.resolution2D[1];
		}
		m_ldrBuffer2D = Framebuffer::create(Texture::createEmpty("FPSci::2DShaderPass::Input", width, height, 
			ImageFormat::RGBA8(), Texture::DIM_2D, true));
		m_ldrShader2DOutput = Framebuffer::create(Texture::createEmpty("FPSci::2DShaderPass::Output", width, height, 
			ImageFormat::RGBA8(), Texture::DIM_2D, true));
	}
	else {
		m_ldrBuffer2D.reset();
		m_ldrShader2DOutput.reset();
	}

	// 3D shader output (use popped framebuffer as input) used when 3D resolution or shader is specified
	if (!config->render.shader3D.empty() || config->render.resolution3D[0] > 0) {
		width = m_framebuffer->width(); height = m_framebuffer->height();
		if (config->render.resolution3D[0] > 0) {
			width = config->render.resolution3D[0];
			height = config->render.resolution3D[1];
		}
		m_hdrShader3DOutput = Framebuffer::create(Texture::createEmpty("FPSci::3DShaderPass::Output", width, height, 
			m_framebuffer->texture(0)->format(), Texture::DIM_2D, true));
	}
	else {
		m_hdrShader3DOutput.reset();
	}

	// Composite buffer (input and output) used when composite shader or resolution is specified
	if (!config->render.shaderComposite.empty() || config->render.resolutionComposite[0] > 0) {
		width = renderDevice->width(); height = renderDevice->height();
		m_ldrBufferPrecomposite = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Precomposite", width, height, 
			ImageFormat::RGB8(), Texture::DIM_2D, true));
		if (config->render.resolutionComposite[0] > 0) {
			width = config->render.resolutionComposite[0];
			height = config->render.resolutionComposite[1];
		}
		m_ldrBufferComposite = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Input", width, height, 
			ImageFormat::RGB8(), Texture::DIM_2D, true));
		m_ldrShaderCompositeOutput = Framebuffer::create(Texture::createEmpty("FPSci::CompositeShaderPass::Output", width, height, 
			ImageFormat::RGB8(), Texture::DIM_2D, true));
	}
	else {
		m_ldrBufferPrecomposite.reset();
		m_ldrBufferComposite.reset();
		m_ldrShaderCompositeOutput.reset();
	}
}

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


void FPSciApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) {

	pushRdStateWithDelay(rd, m_ldrDelayBufferQueue, m_currentDelayBufferIndex, displayLagFrames);

	// Tone mapping from HDR --> LDR happens at the end of this call (after onPostProcessHDR3DEffects() call)
	GApp::onGraphics3D(rd, surface);

	// Draw 2D game elements to be delayed
	if (notNull(m_ldrBuffer2D)) {
		// If rendering into a split (offscreen) 2D buffer, then clear it/render to it here
		m_ldrBuffer2D->texture(0)->clear();
	}

	const Vector2 resolution = isNull(m_ldrBuffer2D) ? rd->viewport().wh() : m_ldrBuffer2D->vector2Bounds();
	isNull(m_ldrBuffer2D) ? rd->push2D() : rd->push2D(m_ldrBuffer2D); {
		isNull(m_ldrBuffer2D) ?
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA) :		// Drawing into framebuffer
			rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);							// Drawing into buffer 2D (don't alpha blend)
		drawDelayed2DElements(rd, resolution);
	}rd->pop2D();

	popRdStateWithDelay(rd, m_ldrDelayBufferQueue, m_currentDelayBufferIndex, displayLagFrames);

	// Transfer LDR framebuffer to the composite buffer (if used)
	if (m_ldrBufferComposite) {
		// Copy the current draw framebuffer texture to precomposite buffer (blitTo causes debug error here)
		rd->copyTextureFromScreen(m_ldrBufferPrecomposite->texture(0), rd->viewport());
		// Resample the copied framebuffer onto the (controlled resolution) composite shader input buffer
		rd->push2D(m_ldrBufferComposite); {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrBufferPrecomposite->texture(0), trialConfig->render.samplerPrecomposite, true);
		} rd->pop2D();
	}
}
 
void FPSciApp::onPostProcessHDR3DEffects(RenderDevice* rd) {
	if (notNull(m_hdrShader3DOutput)) {
		if(trialConfig->render.shader3D.empty()) {
			// No shader specified, just a resize perform pass through into 3D output buffer from framebuffer
			rd->push2D(m_hdrShader3DOutput); {
				Draw::rect2D(rd->viewport(), rd, Color3::white(), m_framebuffer->texture(0), trialConfig->render.sampler3D);
			} rd->pop2D();
		}
		else {
			BEGIN_PROFILER_EVENT_WITH_HINT("3D Shader Pass", "Time to run the post-3D shader pass");

				rd->push2D(m_hdrShader3DOutput); {
				// Setup shadertoy-style args
				Args args;
				args.setUniform("iChannel0", m_framebuffer->texture(0), trialConfig->render.sampler3D);
				const float iTime = float(System::time() - m_startTime);
				args.setUniform("iTime", iTime);
				args.setUniform("iTimeDelta", iTime - m_lastTime);
				args.setUniform("iMouse", userInput->mouseXY());
				args.setUniform("iFrame", m_frameNumber);
				args.setRect(rd->viewport());
				LAUNCH_SHADER_PTR(m_shaderTable[trialConfig->render.shader3D], args);
				m_lastTime = iTime;
			} rd->pop2D();

			END_PROFILER_EVENT();
		}

		// Resample the shader output buffer into the framebuffer
		rd->push2D(); {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_hdrShader3DOutput->texture(0), trialConfig->render.sampler3DOutput);
		} rd->pop2D();
	}

	GApp::onPostProcessHDR3DEffects(rd);
}

void FPSciApp::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D) {
	shared_ptr<Framebuffer> target = isNull(m_ldrBuffer2D) ? m_ldrBufferComposite : m_ldrBuffer2D;
	// Set render buffer depending on whether we are rendering to a sperate 2D buffer
	const Vector2 resolution = isNull(target) ? rd->viewport().wh() : target->vector2Bounds();
	isNull(target) ? rd->push2D() : rd->push2D(target); {
		target != m_ldrBuffer2D || isNull(target) ?
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA) :		// Drawing into buffer w/ contents
			rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);							// Drawing into 2D buffer (don't alpha blend)
		draw2DElements(rd, resolution);
	} rd->pop2D();

	if(notNull(m_ldrBuffer2D)){
		if (trialConfig->render.shader2D.empty()) {
			m_ldrShader2DOutput = m_ldrBuffer2D;		// Redirect output pointer to input (skip shading)
		}
		else {
			BEGIN_PROFILER_EVENT_WITH_HINT("2D Shader Pass", "Time to run the post-2D shader pass");
			rd->push2D(m_ldrShader2DOutput); {
				// Setup shadertoy-style args
				Args args;
				args.setUniform("iChannel0", m_ldrBuffer2D->texture(0), trialConfig->render.sampler2D);
				const float iTime = float(System::time() - m_startTime);
				args.setUniform("iTime", iTime);
				args.setUniform("iTimeDelta", iTime - m_last2DTime);
				args.setUniform("iMouse", userInput->mouseXY());
				args.setUniform("iFrame", m_frameNumber);
				args.setRect(rd->viewport());
				LAUNCH_SHADER_PTR(m_shaderTable[trialConfig->render.shader2D], args);
				m_last2DTime = iTime;
			} rd->pop2D();
			END_PROFILER_EVENT();
		}

		// Direct shader output to the display or composite shader input (if specified)
		isNull(m_ldrBufferComposite) ? rd->push2D() : rd->push2D(m_ldrBufferComposite); {
			rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrShader2DOutput->texture(0), trialConfig->render.sampler2DOutput);
		} rd->pop2D();
	}

	//  Handle post-2D composite shader here
	if (m_ldrBufferComposite) {
		if (trialConfig->render.shaderComposite.empty()) {
			m_ldrShaderCompositeOutput = m_ldrBufferComposite;		// Redirect output pointer to input
		}
		else {
			// Run a composite shader
			BEGIN_PROFILER_EVENT_WITH_HINT("Composite Shader Pass", "Time to run the composite shader pass");

			rd->push2D(m_ldrShaderCompositeOutput); {
				// Setup shadertoy-style args
				Args args;
				args.setUniform("iChannel0", m_ldrBufferComposite->texture(0), trialConfig->render.samplerComposite);
				const float iTime = float(System::time() - m_startTime);
				args.setUniform("iTime", iTime);
				args.setUniform("iTimeDelta", iTime - m_lastCompositeTime);
				args.setUniform("iMouse", userInput->mouseXY());
				args.setUniform("iFrame", m_frameNumber);
				args.setRect(rd->viewport());
				LAUNCH_SHADER_PTR(m_shaderTable[trialConfig->render.shaderComposite], args);
				m_lastCompositeTime = iTime;
			} rd->pop2D();

			END_PROFILER_EVENT();
		}

		// Copy the shader output buffer into the framebuffer
		rd->push2D(); {
			Draw::rect2D(rd->viewport(), rd, Color3::white(), m_ldrShaderCompositeOutput->texture(0), trialConfig->render.samplerFinal);
		} rd->pop2D();
	}

	// Non-scaled content is rendered here
	// Draw the user feedback message (not part of experiment so drawn at native resolution)
	drawFeedbackMessage(rd);
	// Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
	Surface2D::sortAndRender(rd, posed2D);
}

void FPSciApp::pushRdStateWithDelay(RenderDevice* rd, Array<shared_ptr<Framebuffer>> &delayBufferQueue, int &delayIndex, int lagFrames) {

	if (lagFrames > 0) {
		// Need one more frame in the queue than we have frames of delay, to hold the current frame
		if (delayBufferQueue.size() <= lagFrames) {
			// Allocate new textures
			for (int i = lagFrames - delayBufferQueue.size(); i >= 0; --i) {
				delayBufferQueue.push(Framebuffer::create(Texture::createEmpty(format("Delay buffer %d", delayBufferQueue.size()), rd->width(), rd->height(), ImageFormat::RGB8())));
			}
			debugAssert(delayBufferQueue.size() == lagFrames + 1);
		}

		// When the display lag changes, we must be sure to be within range
		delayIndex = min(lagFrames, delayIndex);

		rd->pushState(delayBufferQueue[delayIndex]);
	}
}

void FPSciApp::popRdStateWithDelay(RenderDevice* rd, const Array<shared_ptr<Framebuffer>> &delayBufferQueue, int& delayIndex, int lagFrames) {

	if (lagFrames > 0) {
		// Display the delayed frame
		rd->popState();
		// Render into the frame buffer or the composite shader input buffer (if provided)
		rd->push2D(); {
			// Advance the pointer to the next, which is also the oldest frame
			delayIndex = (delayIndex + 1) % (lagFrames + 1);
			Draw::rect2D(rd->viewport(), rd, Color3::white(), delayBufferQueue[delayIndex]->texture(0), Sampler::buffer());
		} rd->pop2D();
	}
}

void FPSciApp::draw2DElements(RenderDevice* rd, Vector2 resolution) {
	// Put elements that should not be delayed here
	const float scale = resolution.x / 1920.0f;		// Double check on how this scale is used (seems to assume 1920x1080 defaults)	

	updateFPSIndicator(rd, resolution);				// FPS display (faster than the full stats widget)

	// Handle recording indicator
	if (notNull(waypointManager) && waypointManager->recordMotion) {
		Draw::point(Point2(0.9f * resolution.x - 15.f * scale, (20.0f + m_debugMenuHeight) * scale), rd, Color3::red(), 10.0f * scale);
		outputFont->draw2D(rd, "Recording Position", Point2(0.9f * resolution.x, (7.5f + m_debugMenuHeight) * scale), 20.0f * scale, Color3::red());
	}

	// Click-to-photon mouse event indicator
	if (trialConfig->clickToPhoton.enabled && trialConfig->clickToPhoton.mode != "total") {
		drawClickIndicator(rd, trialConfig->clickToPhoton.mode, resolution);
	}

	// Player camera only indicators
	if (activeCamera() == playerCamera) {
		// Reticle
		float tscale = weapon->cooldownRatio(m_lastOnSimulationRealTime, reticleConfig.changeTimeS);
		float rScale = scale * (tscale * reticleConfig.scale[0] + (1.0f - tscale) * reticleConfig.scale[1]);
		Color4 rColor = reticleConfig.color[1] * (1.0f - tscale) + reticleConfig.color[0] * tscale;
		Draw::rect2D(((reticleTexture->rect2DBounds() - reticleTexture->vector2Bounds() / 2.0f)) * rScale / 2.0f + resolution / 2.0f, rd, rColor, reticleTexture);
	}
}

void FPSciApp::drawDelayed2DElements(RenderDevice* rd, Vector2 resolution) {
	// Put elements that should be delayed along w/ (or independent of) 3D here
	const float scale = resolution.x / 1920.0f;

	// Draw target health bars
	if (trialConfig->targetView.showHealthBars) {
		for (auto const& target : sess->targetArray()) {
			target->drawHealthBar(rd, *activeCamera(), *m_framebuffer,
				trialConfig->targetView.healthBarSize,
				trialConfig->targetView.healthBarOffset,
				trialConfig->targetView.healthBarBorderSize,
				trialConfig->targetView.healthBarColors,
				trialConfig->targetView.healthBarBorderColor);
		}
	}

	// Draw the combat text
	if (trialConfig->targetView.showCombatText) {
		Array<int> toRemove;
		for (int i = 0; i < m_combatTextList.size(); i++) {
			bool remove = !m_combatTextList[i]->draw(rd, *playerCamera, *m_framebuffer);
			if (remove) m_combatTextList[i] = nullptr;		// Null pointers to remove
		}
		// Remove the expired elements here
		m_combatTextList.removeNulls();
	}

	if (trialConfig->clickToPhoton.enabled && trialConfig->clickToPhoton.mode == "total") {
		drawClickIndicator(rd, "total", resolution);
	}

	// Draw the HUD here
	if (trialConfig->hud.enable) {
		drawHUD(rd, resolution);
	}
}

void FPSciApp::drawClickIndicator(RenderDevice* rd, String mode, Vector2 resolution) {
	// Click to photon latency measuring corner box
	if (trialConfig->clickToPhoton.enabled) {
		float boxLeft = 0.0f;
		// Paint both sides by the width of latency measuring box.
		Point2 latencyRect = trialConfig->clickToPhoton.size * resolution;
		float boxTop = resolution.y * trialConfig->clickToPhoton.vertPos - latencyRect.y / 2;
		if (trialConfig->clickToPhoton.mode == "both") {
			boxTop = (mode == "minimum") ? boxTop - latencyRect.y : boxTop + latencyRect.y;
		}
		if (trialConfig->clickToPhoton.side == "right") {
			boxLeft = resolution.x - latencyRect.x;
		}
		// Draw the "active" box
		Color3 boxColor;
		if (trialConfig->clickToPhoton.mode == "frameRate") {
			boxColor = (frameToggle) ? trialConfig->clickToPhoton.colors[0] : trialConfig->clickToPhoton.colors[1];
			frameToggle = !frameToggle;
		}
		else boxColor = (shootButtonUp) ? trialConfig->clickToPhoton.colors[0] : trialConfig->clickToPhoton.colors[1];
		Draw::rect2D(Rect2D::xywh(boxLeft, boxTop, latencyRect.x, latencyRect.y), rd, boxColor);
	}
}

void FPSciApp::drawFeedbackMessage(RenderDevice* rd) {
	if (activeCamera() != playerCamera) return;		// Only draw on the player camera

	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
		// Handle the feedback message
		const float scale = rd->width() / 1920.f;
		String message = sess->getFeedbackMessage();
		const float centerHeight = rd->height() * 0.4f;
		const float scaledFontSize = floor(trialConfig->feedback.fontSize * scale);
		if (!message.empty()) {
			String currLine;
			Array<String> lines = stringSplit(message, '\n');
			float vertPos = centerHeight - (scaledFontSize * 1.5f * lines.length() / 2.0f);
			// Draw a "back plate"
			Draw::rect2D(Rect2D::xywh(0.0f,
				vertPos - 1.5f * scaledFontSize,
				(float) rd->width(),
				scaledFontSize * (lines.length() + 1) * 1.5f),
				rd, trialConfig->feedback.backgroundColor);
			for (String line : lines) {
				outputFont->draw2D(rd, line.c_str(),
					(Point2(rd->width() * 0.5f, vertPos)).floor(),
					scaledFontSize,
					trialConfig->feedback.color,
					trialConfig->feedback.outlineColor,
					GFont::XALIGN_CENTER, GFont::YALIGN_CENTER
				);
				vertPos += scaledFontSize * 1.5f;
			}
		}
	} rd->pop2D();
}

void FPSciApp::updateFPSIndicator(RenderDevice* rd, Vector2 resolution) {
	// Track the instantaneous frame duration (no smoothing) in a circular queue (regardless of whether we are drawing the indicator)
	if (m_frameDurationQueue.length() > MAX_HISTORY_TIMING_FRAMES) {
		m_frameDurationQueue.dequeue();
	}

	const float f = rd->stats().frameRate;
	const float t = 1.0f / f;
	m_frameDurationQueue.enqueue(t);

	float recentMin = finf();
	float recentMax = -finf();
	for (int i = 0; i < m_frameDurationQueue.length(); ++i) {
		const float t = m_frameDurationQueue[i];
		recentMin = min(recentMin, t);
		recentMax = max(recentMax, t);
	}

	if (renderFPS) {
		// Draw the FPS indicator
		const float scale = resolution.x / 1920.0f;
		String msg;
		if (window()->settings().refreshRate > 0) {
			msg = format("%d measured | %d requested fps", iRound(rd->stats().smoothFrameRate), window()->settings().refreshRate);
		}
		else {
			msg = format("%d fps", iRound(rd->stats().smoothFrameRate));
		}
		msg += format(" | %.1f min | %.1f avg | %.1f max ms", recentMin * 1000.0f, 1000.0f / rd->stats().smoothFrameRate, 1000.0f * recentMax);
		outputFont->draw2D(rd, msg, Point2(0.75f * resolution.x, 0.05f * resolution.y).floor(), floor(20.0f * scale), Color3::yellow());
	}
}

void FPSciApp::drawHUD(RenderDevice *rd, Vector2 resolution) {
	// Scale is used to position/resize the "score banner" when the window changes size in "windowed" mode (always 1 in fullscreen mode).
	const Vector2 scale = resolution / (Vector2)OSWindow::primaryDisplayWindowSize();

	RealTime now = m_lastOnSimulationRealTime;

	// Weapon ready status (cooldown indicator)
	if (trialConfig->hud.renderWeaponStatus) {
		// Draw the "active" cooldown box
		if (trialConfig->hud.cooldownMode == "box") {
			float boxLeft = 0.0f;
			if (trialConfig->hud.weaponStatusSide == "right") {
				// swap side
				boxLeft = resolution.x * (1.0f - trialConfig->clickToPhoton.size.x);
			}
			Draw::rect2D(
				Rect2D::xywh(
					boxLeft,
					resolution.y * (weapon->cooldownRatio(now)),
					resolution.x * trialConfig->clickToPhoton.size.x,
					resolution.y * (1.0f - weapon->cooldownRatio(now))
				), rd, Color3::white() * 0.8f
			);
		}
		else if (trialConfig->hud.cooldownMode == "ring") {
			// Draw cooldown "ring" instead of box
			const float iRad = trialConfig->hud.cooldownInnerRadius;
			const float oRad = iRad + trialConfig->hud.cooldownThickness;
			const int segments = trialConfig->hud.cooldownSubdivisions;
			int segsToLight = static_cast<int>(ceilf((1 - weapon->cooldownRatio(now))*segments));
			// Create the segments
			for (int i = 0; i < segsToLight; i++) {
				const float inc = static_cast<float>(2 * pi() / segments);
				const float theta = -i * inc;
				Vector2 center = resolution / 2.0f;
				Array<Vector2> verts = {
					center + Vector2(oRad*sin(theta), -oRad * cos(theta)),
					center + Vector2(oRad*sin(theta + inc), -oRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta + inc), -iRad * cos(theta + inc)),
					center + Vector2(iRad*sin(theta), -iRad * cos(theta))
				};
				Draw::poly2D(verts, rd, trialConfig->hud.cooldownColor);
			}
		}
	}

	// Draw the player health bar
	if (trialConfig->hud.showPlayerHealthBar) {
		//const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		const float health = scene()->typedEntity<PlayerEntity>("player")->health();
		Point2 location = trialConfig->hud.playerHealthBarPos * resolution;
		location.y += (m_debugMenuHeight * scale.y);
		const Point2 size = trialConfig->hud.playerHealthBarSize * resolution;
		const Vector2 border = trialConfig->hud.playerHealthBarBorderSize * resolution;
		const Color4 borderColor = trialConfig->hud.playerHealthBarBorderColor;
		const Color4 color = trialConfig->hud.playerHealthBarColors[1] * (1.0f - health) + trialConfig->hud.playerHealthBarColors[0] * health;

		Draw::rect2D(Rect2D::xywh(location - border, size + border + border), rd, borderColor);
		Draw::rect2D(Rect2D::xywh(location, size*Point2(health, 1.0f)), rd, color);
	}
	// Draw the ammo indicator
	if (trialConfig->hud.showAmmo) {
		//const float guardband = (rd->framebuffer()->width() - window()->framebuffer()->width()) / 2.0f;
		Point2 lowerRight = resolution; //Point2(static_cast<float>(rd->viewport().width()), static_cast<float>(rd->viewport().height())) - Point2(guardband, guardband);
		hudFont->draw2D(rd,
			format("%d/%d", weapon->remainingAmmo(), trialConfig->weapon.maxAmmo),
			lowerRight - trialConfig->hud.ammoPosition,
			trialConfig->hud.ammoSize,
			trialConfig->hud.ammoColor,
			trialConfig->hud.ammoOutlineColor,
			GFont::XALIGN_RIGHT,
			GFont::YALIGN_BOTTOM
		);
	}

	if (trialConfig->hud.showBanner) {
		const shared_ptr<Texture> scoreBannerTexture = hudTextures["scoreBannerBackdrop"];
		const Point2 hudCenter(resolution.x / 2.0f, trialConfig->hud.bannerVertVisible * scoreBannerTexture->height() * scale.y + debugMenuHeight());
		Draw::rect2D((scoreBannerTexture->rect2DBounds() * scale - scoreBannerTexture->vector2Bounds() * scale / 2.0f) * 0.8f + hudCenter, rd, Color3::white(), scoreBannerTexture);

		// Create strings for time remaining, progress in sessions, and score
		float time;
		if (trialConfig->hud.bannerTimerMode == "remaining") {
			time = sess->getRemainingTrialTime();
			if (time < 0.f) time = 0.f;
		}
		else if (trialConfig->hud.bannerTimerMode == "elapsed") {
			time = sess->getElapsedTrialTime();
		}
		String time_string = time < 10000.f ? format("%0.1f", time) : "---";		// Only allow up to 3 digit time strings

		float prog = sess->getProgress();
		String prog_string = "";
		if (!isnan(prog)) {
			prog_string = format("%d", (int)G3D::round(100.0f*prog)) + "%";
		}

		const double score = sess->getScore();
		String score_string;
		if (score <= 1e4) {
			score_string = format("%d", (int)G3D::round(score));
		}
		else if (score > 1e4 && score <= 1e7) {
			score_string = format("%dk", (int)G3D::round(score / 1e3));
		}
		else if (score > 1e7 && score <= 1e10) {
			score_string = format("%dM", (int)G3D::round(score / 1e6));
		}
		else if (score > 1e10) {
			score_string = format("%dB", (int)G3D::round(score / 1e9));
		}

		if (trialConfig->hud.bannerTimerMode != "none" && sess->inTask()) {
			hudFont->draw2D(rd, time_string, hudCenter - Vector2(80, 0) * scale.x, scale.x * trialConfig->hud.bannerSmallFontSize,
				Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
		}
		if(trialConfig->hud.bannerShowProgress) hudFont->draw2D(rd, prog_string, hudCenter + Vector2(0, -1), scale.x * trialConfig->hud.bannerLargeFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
		if(trialConfig->hud.bannerShowScore) hudFont->draw2D(rd, score_string, hudCenter + Vector2(125, 0) * scale, scale.x * trialConfig->hud.bannerSmallFontSize, Color3::white(), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_CENTER);
	}

	// Draw any static HUD elements
	for (StaticHudElement element : trialConfig->hud.staticElements) {
		if (!hudTextures.containsKey(element.filename)) continue;						// Skip any items we haven't loaded
		const shared_ptr<Texture> texture = hudTextures[element.filename];				// Get the loaded texture for this element
		const Vector2 size = element.scale * scale * texture->vector2Bounds();			// Get the final size of the image
		const Vector2 pos = (element.position * resolution) - size/2.0;					// Compute position (center image on provided position)
		Draw::rect2D(Rect2D::xywh(pos, size), rd, Color3::white(), texture);			// Draw the rect
	}
}

