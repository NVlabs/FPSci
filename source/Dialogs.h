#pragma once
#include <G3D/G3D.h>

// Internal class for ease of use
class DialogBase : public GuiWindow {
protected:
	String m_prompt;
	DialogBase(const shared_ptr<GuiTheme> theme, String title = "Dialog", Point2 pos = Point2(200.f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
		GuiWindow(title, theme, Rect2D::xywh(pos, size), GuiTheme::MENU_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE) {};
public:
	String result = "";
	bool complete = false;

	void reset() {
		result = "";
		complete = false;
		setVisible(true);
	}
};

// N-way selection dialog
class SelectionDialog : public DialogBase {
protected:
	GuiButton* m_submitBtn = nullptr;
	GuiButton* m_clearBtn = nullptr;
	Array<GuiButton*> m_optionBtns;
	Array<String> m_options;
	Array<std::function<void()>> m_callbacks;
		
	void callback(String option) {
		result = option;
		if (!m_submitBtn) {
			submitCallback();							// If we don't have a submit button finish the dialog here
		}
		else {
			m_submitBtn->setEnabled(true);				// If we have a submit button set it enabled here
			m_clearBtn->setEnabled(true);
			for (auto btn : m_optionBtns) {
				if (btn->caption().text() == option) btn->setEnabled(false);
				else btn->setVisible(false);
			}
		}
	}

	void submitCallback() {
		complete = true;
		setVisible(false);
	}

	void clearCallback() {
		m_submitBtn->setEnabled(false);
		m_clearBtn->setEnabled(false);
		for (auto btn : m_optionBtns) { 
			btn->setVisible(true);
			btn->setEnabled(true); 
		}
	}

	SelectionDialog(String prompt, Array<String> options, const shared_ptr<GuiTheme>& theme,
		String title = "Selection", bool submitButton = false, int itemsPerRow = 3,
		Point2 size = Point2(400.0f, 400.0f), bool resize = true, Point2 pos = Point2(200.0f, 200.0f),	GFont::XAlign promptAlign = GFont::XALIGN_CENTER) :
		DialogBase(theme, title, pos, size)
	{
		m_prompt = prompt;
		m_options = options;
		GuiPane *pane = GuiWindow::pane();
		pane->beginRow(); {
			auto  promptLabel = pane->addLabel(m_prompt, promptAlign);
			promptLabel->setWidth(size.x);									// Set the prompt label to full width to center text
		} pane->endRow();
		// Create option buttons
		int cnt = 0;
		int i = 0;
		const int rows = options.length() / itemsPerRow;
		for (String option : options) {
			cnt %= itemsPerRow;
			if (cnt == 0) {
				pane->beginRow();
			}
			m_callbacks.append(std::bind(&SelectionDialog::callback, this, option));
			GuiButton* btn = pane->addButton(option, m_callbacks[i]);
			if (!resize) {		// Larger buttons when not resizing
				btn->setWidth(0.99f*size.x / itemsPerRow);
				btn->setHeight(min(100.0f, 0.99f * size.y/rows));
			}
			m_optionBtns.append(btn);
			i++;
			if (++cnt == itemsPerRow) {
				pane->endRow();
			}
		}
		if (submitButton) {
			pane->beginRow(); {
				// Create submit and clear buttons
				m_clearBtn = pane->addButton("Clear", std::bind(&SelectionDialog::clearCallback, this));
				m_submitBtn = pane->addButton("Submit", std::bind(&SelectionDialog::submitCallback, this));
				// Start with submit/clear buttons disabled
				m_clearBtn->setEnabled(false);
				m_submitBtn->setEnabled(false);		
			} pane->endRow();
		}

		// Optionally resize the window for contents
		if (resize) { pack();}

		if (m_submitBtn) {
			// Move submit button to the far right bottom corner of the pane
			m_submitBtn->moveBy(bounds().width() - m_clearBtn->rect().width() - m_submitBtn->rect().width() - 5.f, 0.f);
		}
	};

public:
	static shared_ptr<SelectionDialog> create(String prompt, Array<String> options, const shared_ptr<GuiTheme> theme,
		String title = "Selection", bool submitBtn = false, int itemsPerRow = 3, Point2 size = Point2(400.0f, 200.0f), bool resize = true, Point2 position = Point2(200.0f, 200.0f))
	{
		return createShared<SelectionDialog>(prompt, options, theme, title, submitBtn, itemsPerRow,  size, resize, position);
	}
};

class YesNoDialog : public SelectionDialog {
protected:
	YesNoDialog(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog", bool submitBtn=false) :
		SelectionDialog(question, Array<String>{"Yes", "No"}, theme, title, submitBtn) {};
public:
	static shared_ptr<YesNoDialog> create(String question, const shared_ptr<GuiTheme> theme, String title="Dialog", bool submitBtn=false){ 
		return createShared<YesNoDialog>(question, theme, title, submitBtn);
	}
};

class YesNoCancelDialog : public SelectionDialog {
protected:
	YesNoCancelDialog(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog", bool submitBtn=false) :
		SelectionDialog(question, Array<String>{"Yes", "No", "Cancel"}, theme, title, submitBtn) {};
public:
	static shared_ptr<YesNoCancelDialog> create(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog", bool submitBtn=false) {
		return createShared<YesNoCancelDialog>(question, theme, title, submitBtn);
	}
};

class TextEntryDialog : public DialogBase {
protected:	
	bool m_allowEmpty;
	GuiLabel* m_warningLabel;
	GuiMultiLineTextBox* m_textbox;

	void submitCallback() {
		if (!m_allowEmpty && result.length() == 0) {
			m_warningLabel->setCaption("Must provide a response!");
			m_textbox->setFocused(true);
		}
		else {
			complete = true;
			setVisible(false);
		}
	}

	TextEntryDialog(String prompt, const shared_ptr<GuiTheme> theme, String title = "Dialog", bool allowEmpty = true,
		Point2 size = Point2(400.0f, 200.0f), bool resize=true, Point2 position = Point2(200.0f, 200.0f)) :
		DialogBase(theme, title, position, size) 
	{
		m_prompt = prompt;
		m_allowEmpty = allowEmpty;
		GuiPane *pane = GuiWindow::pane();
		pane->beginRow(); {
			auto l = pane->addLabel(m_prompt);
			l->setSize(0.99f*size[0], 50.0f);
		} pane->endRow();

		if (!m_allowEmpty) {
			pane->beginRow(); {
				m_warningLabel = pane->addLabel("", GFont::XALIGN_CENTER);
			}pane->endRow();
		}

		pane->beginRow(); {
			m_textbox = pane->addMultiLineTextBox("", &result);
			m_textbox->setSize(0.99f*size[0], size[1] - 150.0f);
		} pane->endRow();

		pane->beginRow(); {
			auto b = pane->addButton("Submit", std::bind(&TextEntryDialog::submitCallback, this));
			b->setSize(0.99f*size[0], 50.0f);
		}

		if (resize) { pack(); }
	}
public:
	static shared_ptr<TextEntryDialog> create(String prompt, const shared_ptr<GuiTheme> theme, 
		String title = "Dialog", bool allowEmpty=true, Point2 size = Point2(400.0f, 300.0f), bool resize=true, Point2 position = Point2(200.0f, 200.0f))
	{
		return createShared<TextEntryDialog>(prompt, theme, title, allowEmpty, size, resize, position);
	}
};

class RatingDialog : public SelectionDialog {
protected:
	RatingDialog(String prompt, Array<String> levels, const shared_ptr<GuiTheme> theme,
		String title = "Dialog", bool submitBtn=false, Point2 size = Point2(400.0f, 200.0f), bool resize=true, Point2 position = Point2(200.0f, 200.0f)) :
		SelectionDialog(prompt, levels, theme, title, submitBtn, levels.size(), size, resize, position, GFont::XALIGN_LEFT) {}
public:
	static shared_ptr<RatingDialog> create(String prompt, Array<String> levels, const shared_ptr<GuiTheme> theme,
		String title = "Dialog", bool submitBtn=false, Point2 size = Point2(400.0f, 200.0f), bool resize=true, Point2 position = Point2(200.0f, 200.0f))
	{
		return createShared<RatingDialog>(prompt, levels, theme, title, submitBtn, size, resize, position);
	}
};