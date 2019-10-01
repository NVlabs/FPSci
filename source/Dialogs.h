#pragma once
#include <G3D/G3D.h>

// Internal class for ease of use
class G3Dialog : public GuiWindow {
protected:
	G3Dialog(const shared_ptr<GuiTheme> theme, String title = "Dialog", Point2 pos = Point2(200.f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
		GuiWindow(title, theme, Rect2D::xywh(pos, size), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE) {};
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
class SelectionDialog : public G3Dialog {
protected:
	String m_message;
	Array<String> m_options;
	Array<std::function<void()>> m_callbacks;
	
	void callback(String option) {
		result = option;
		complete = true;
		setVisible(false);
	}

	SelectionDialog(String message, Array<String> options, const shared_ptr<GuiTheme>& theme,
		String title = "Selection", Point2 size = Point2(400.0f, 400.0f), Point2 pos = Point2(200.0f, 200.0f),	int itemsPerRow = 3) :
		m_message(message), m_options(options), G3Dialog(theme, title, pos, size)
	{
		GuiPane *pane = GuiWindow::pane();
		pane->beginRow(); {
			pane->addLabel(m_message, G3D::GFont::XALIGN_CENTER);
		} pane->endRow();
		// Create option buttons
		int cnt = 0;
		int i = 0;
		for (String option : options) {
			cnt %= itemsPerRow;
			if (cnt == 0) {
				pane->beginRow();
			}
			m_callbacks.append(std::bind(&SelectionDialog::callback, this, option));
			pane->addButton(option, m_callbacks[i]);
			i++;
			if (++cnt == itemsPerRow) {
				pane->endRow();
			}
		}
		pack();
	};

public:
	static shared_ptr<SelectionDialog> create(String message,  Array<String> options, const shared_ptr<GuiTheme> theme, 
		String title = "Selection", Point2 size = Point2(400.0f, 200.0f), Point2 position = Point2(200.0f, 200.0f),	int itemsPerRow	= 3) 
	{
		return createShared<SelectionDialog>(message, options, theme, title, size, position, itemsPerRow);
	}
};

class YesNoDialog : public SelectionDialog {
protected:
	YesNoDialog(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog") :
		SelectionDialog(question, Array<String>{"Yes", "No"}, theme, title) {};
public:
	static shared_ptr<YesNoDialog> create(String question, const shared_ptr<GuiTheme> theme, String title="Dialog"){ 
		return createShared<YesNoDialog>(question, theme, title);
	}
};

class YesNoCancelDialog : public SelectionDialog {
protected:
	YesNoCancelDialog(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog") :
		SelectionDialog(question, Array<String>{"Yes", "No", "Cancel"}, theme, title) {};
public:
	static shared_ptr<YesNoCancelDialog> create(String question, const shared_ptr<GuiTheme> theme, String title = "Dialog") {
		return createShared<YesNoCancelDialog>(question, theme, title);
	}
};

class TextEntryDialog : public G3Dialog {
protected:
	String m_prompt;
	
	void submitCallback() {
		complete = true;
		setVisible(false);
	}

	TextEntryDialog(String prompt, const shared_ptr<GuiTheme> theme, 
		String title = "Dialog", Point2 position = Point2(200.0f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
		m_prompt(prompt), G3Dialog(theme, title, position, size) 
	{
		GuiPane *pane = GuiWindow::pane();
		pane->beginRow(); {
			auto l = pane->addLabel(m_prompt);
			l->setSize(size[0], 50.0f);
		} pane->endRow();

		pane->beginRow(); {
			auto textBox = pane->addMultiLineTextBox("", &result);
			textBox->setSize(size[0], size[1] - 100.0f);
		} pane->endRow();

		pane->beginRow(); {
			auto b = pane->addButton("Submit", std::bind(&TextEntryDialog::submitCallback, this));
			b->setSize(size[0], 50.0f);
		}
		pack();
	}
public:
	static shared_ptr<TextEntryDialog> create(String prompt, const shared_ptr<GuiTheme> theme, 
		String title = "Dialog", Point2 position = Point2(200.0f, 200.0f), Point2 size = Point2(400.0f, 300.0f)) 
	{
		return createShared<TextEntryDialog>(prompt, theme, title, position, size);
	}
};