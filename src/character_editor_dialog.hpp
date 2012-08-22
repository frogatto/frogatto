#ifndef CHARACTER_EDITOR_DIALOG_HPP_INCLUDED
#define CHARACTER_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <map>
#include <string>

#include "dialog.hpp"
#include "text_editor_widget.hpp"
#include "widget.hpp"

class editor;

namespace gui {
class border_widget;
}

namespace editor_dialogs
{

//editor dialog which displays the details of an object and allows editing it.
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

	gui::widget_ptr generate_grid(const std::string& category);
	std::map<std::string, gui::widget_ptr> grids_;

	//the borders around each object, we set the currently selected one
	//to white, and all the others to transparent.
	std::map<std::string, std::vector<gui::border_widget_ptr> > grid_borders_;

	//the first object in each category
	std::map<std::string, int> first_obj_;

	boost::intrusive_ptr<gui::text_editor_widget> find_edit_;
};

typedef boost::intrusive_ptr<character_editor_dialog> character_editor_dialog_ptr;

}

#endif // NO_EDITOR
#endif
