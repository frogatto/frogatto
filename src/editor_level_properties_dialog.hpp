#ifndef EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED
#define EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR


#include <string>
#include <vector>

#include "dialog.hpp"

class editor;

namespace editor_dialogs
{

class editor_level_properties_dialog : public gui::dialog
{
public:
	explicit editor_level_properties_dialog(editor& e);
	void init();
private:
	void change_title();
	void change_background();
	void execute_change_background(const std::vector<std::string>& choices, int index);

	void change_next_level();
	void change_previous_level();

	editor& editor_;
	gui::widget_ptr context_menu_;
};

}

#endif // !NO_EDITOR
#endif
