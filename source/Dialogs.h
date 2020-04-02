#pragma once
#include <G3D/G3D.h>

// Internal class for ease of use
class DialogBase : public GuiWindow {
protected:
	String m_prompt;
	DialogBase(const shared_ptr<GuiTheme> theme, String title = "Dialog", Point2 pos = Point2(200.f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
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
class SelectionDialog : public DialogBase {
protected:
	Array<String> m_options;
	Array<std::function<void()>> m_callbacks;
	
	void callback(String option) {
		result = option;
		complete = true;
		setVisible(false);
	}

	SelectionDialog(String prompt, Array<String> options, const shared_ptr<GuiTheme>& theme,
		String title = "Selection", Point2 size = Point2(400.0f, 400.0f), Point2 pos = Point2(200.0f, 200.0f),	int itemsPerRow = 3, GFont::XAlign promptAlign = GFont::XALIGN_CENTER) :
		DialogBase(theme, title, pos, size)
	{
		m_prompt = prompt;
		m_options = options;
		GuiPane *pane = GuiWindow::pane();
		pane->beginRow(); {
			pane->addLabel(m_prompt, promptAlign);
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
	static shared_ptr<SelectionDialog> create(String prompt,  Array<String> options, const shared_ptr<GuiTheme> theme, 
		String title = "Selection", Point2 size = Point2(400.0f, 200.0f), Point2 position = Point2(200.0f, 200.0f),	int itemsPerRow	= 3) 
	{
		return createShared<SelectionDialog>(prompt, options, theme, title, size, position, itemsPerRow);
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

class TextEntryDialog : public DialogBase {
protected:	
	void submitCallback() {
		complete = true;
		setVisible(false);
	}

	TextEntryDialog(String prompt, const shared_ptr<GuiTheme> theme, 
		String title = "Dialog", Point2 position = Point2(200.0f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
		DialogBase(theme, title, position, size) 
	{
		m_prompt = prompt;
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

class RatingDialog : public SelectionDialog {
protected:
	RatingDialog(String prompt, Array<String> levels, const shared_ptr<GuiTheme> theme,
		String title = "Dialog", Point2 position = Point2(200.0f, 200.0f), Point2 size = Point2(400.0f, 200.0f)) :
		SelectionDialog(prompt, levels, theme, title, size, position, levels.size(), GFont::XALIGN_LEFT) {}
public:
	static shared_ptr<RatingDialog> create(String prompt, Array<String> levels, const shared_ptr<GuiTheme> theme,
		String title = "Dialog", Point2 position = Point2(200.0f, 200.0f), Point2 size = Point2(400.0f, 200.0f))
	{
		return createShared<RatingDialog>(prompt, levels, theme, title, position, size);
	}
};