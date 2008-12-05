#ifndef PROP_EDITOR_DIALOG_HPP_INCLUDED
#define PROP_EDITOR_DIALOG_HPP_INCLUDED

#include "dialog.hpp"
#include "prop.hpp"
#include "widget.hpp"

class editor;

namespace editor_dialogs
{

class prop_editor_dialog : public gui::dialog
{
public:
	explicit prop_editor_dialog(editor& e);
	void init();
private:
	void show_category_menu();
	void set_prop(int index);

	void close_context_menu(int index);
	void select_category(const std::string& str);
	editor& editor_;
	std::string category_;
	gui::widget_ptr context_menu_;
};

}

#endif
