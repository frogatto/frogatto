#ifndef SEGMENT_EDITOR_DIALOG_HPP_INCLUDED
#define SEGMENT_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include "dialog.hpp"

class editor;

namespace editor_dialogs
{

class segment_editor_dialog : public gui::dialog
{
public:
	explicit segment_editor_dialog(editor& e);
	void init();

	void set_segment(int num);

private:
	void set_segment_start_difficulty(int value);

	editor& editor_;
	int segment_;
};

}

#endif
#endif // !NO_EDITOR

