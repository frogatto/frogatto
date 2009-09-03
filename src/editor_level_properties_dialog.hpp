#ifndef EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED
#define EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED

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
	void add_stats(const std::vector<stats::record_ptr>& stats);
	void change_title();
	void change_background();
	void execute_change_background(const std::vector<std::string>& choices, int index);

	editor& editor_;
	gui::widget_ptr context_menu_;
};

}

#endif
