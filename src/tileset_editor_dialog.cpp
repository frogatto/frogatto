#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <iostream>

#include "button.hpp"
#include "editor.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "preview_tileset_widget.hpp"
#include "tileset_editor_dialog.hpp"

namespace editor_dialogs
{

tileset_editor_dialog::tileset_editor_dialog(editor& e)
  : dialog(640, 0, 160, 600), editor_(e)
{
	init();
}

void tileset_editor_dialog::init()
{
	clear();
	using namespace gui;
	set_padding(20);

	assert(editor_.get_tileset() >= 0 && editor_.get_tileset() < editor_.all_tilesets().size());
	preview_tileset_widget* preview = new preview_tileset_widget(*editor_.all_tilesets()[editor_.get_tileset()].preview);

	add_widget(widget_ptr(preview), 10, 10);

	grid_ptr grid(new gui::grid(3));
	int index = 0;
	foreach(const editor::tileset& t, editor_.all_tilesets()) {
		preview_tileset_widget* preview = new preview_tileset_widget(*t.preview);
		preview->set_dim(48, 48);
		button_ptr tileset_button(new button(widget_ptr(preview), boost::bind(&tileset_editor_dialog::set_tileset, this, index)));
		tileset_button->set_dim(52, 52);
		grid->add_col(tileset_button);
		++index;
	}

	grid->finish_row();

	add_widget(grid, 0, 120);
}

void tileset_editor_dialog::set_tileset(int index)
{
	editor_.set_tileset(index);
	init();
}

bool tileset_editor_dialog::handle_event(const SDL_Event& event, bool claimed)
{
	if(!claimed) {

		switch(event.type) {
		case SDL_KEYDOWN:
			if(event.key.keysym.sym == SDLK_COMMA) {
				set_tileset(editor_.get_tileset()-1);
				claimed = true;
			} else if(event.key.keysym.sym == SDLK_PERIOD) {
				set_tileset(editor_.get_tileset()+1);
				claimed = true;
			}
			break;
		}
	}

	return dialog::handle_event(event, claimed);
}
}
