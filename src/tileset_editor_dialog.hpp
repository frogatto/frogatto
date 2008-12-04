#ifndef TILESET_EDITOR_DIALOG_HPP_INCLUDED
#define TILESET_EDITOR_DIALOG_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "dialog.hpp"
#include "tile_map.hpp"
#include "widget.hpp"

class editor;

namespace editor_dialogs
{

class tileset_editor_dialog : public gui::dialog
{
public:
	explicit tileset_editor_dialog(editor& e);
	void init();
private:
	void select_category(const std::string& category);
	void close_context_menu(int index);
	void show_category_menu();
	void set_tileset(int index);

	bool handle_event(const SDL_Event& event, bool claimed);
	editor& editor_;

	gui::widget_ptr context_menu_;
	std::string category_;
};

}

#endif
