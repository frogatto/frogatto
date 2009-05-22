#ifndef CHARACTER_EDITOR_DIALOG_HPP_INCLUDED
#define CHARACTER_EDITOR_DIALOG_HPP_INCLUDED

#include <string>

#include "dialog.hpp"
#include "widget.hpp"

class editor;

namespace editor_dialogs
{

class character_editor_dialog : public gui::dialog
{
public:
	explicit character_editor_dialog(editor& e);
	void init();
	void set_character(int index);
	void select_category(const std::string& str);
private:
	void show_category_menu();

	void close_context_menu(int index);
	editor& editor_;
	std::string category_;
	gui::widget_ptr context_menu_;

	int first_index_;
};

}

#endif
