#pragma once
#ifndef HEX_TILESET_EDITOR_DIALOG_HPP_INCLUDED
#define HEX_TILESET_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/shared_ptr.hpp>

#include "dialog.hpp"
#include "tile_map.hpp"
#include "widget.hpp"

class editor;

namespace editor_dialogs
{

class hex_tileset_editor_dialog : public gui::dialog
{
public:
	static void global_tile_update();
	explicit hex_tileset_editor_dialog(editor& e);
	~hex_tileset_editor_dialog();
	
	void init();
	void select_category(const std::string& category);
	void set_tileset(int index);
private:
	hex_tileset_editor_dialog(const hex_tileset_editor_dialog&);

	void close_context_menu(int index);
	void show_category_menu();

	bool handle_event(const SDL_Event& event, bool claimed);
	editor& editor_;

	gui::widget_ptr context_menu_;
	std::string category_;

	//index of the first item in the current category
	int first_index_;
};

typedef boost::intrusive_ptr<hex_tileset_editor_dialog> hex_tileset_editor_dialog_ptr;

}

#endif // !NO_EDITOR
#endif // HEX_TILESET_EDITOR_DIALOG_HPP_INCLUDED
