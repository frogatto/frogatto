#ifndef NO_EDITOR
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <iostream>

#include "border_widget.hpp"
#include "button.hpp"
#include "editor.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "hex_object.hpp"
#include "hex_tile.hpp"
#include "hex_tileset_editor_dialog.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "preview_tileset_widget.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

namespace {
std::set<hex_tileset_editor_dialog*>& all_tileset_editor_dialogs() {
	static std::set<hex_tileset_editor_dialog*> all;
	return all;
}
}

void hex_tileset_editor_dialog::global_tile_update()
{
	foreach(hex_tileset_editor_dialog* d, all_tileset_editor_dialogs()) {
		d->init();
	}
}

hex_tileset_editor_dialog::hex_tileset_editor_dialog(editor& e)
  : dialog(graphics::screen_width() - EDITOR_SIDEBAR_WIDTH, 160, EDITOR_SIDEBAR_WIDTH, 440), editor_(e), first_index_(-1)
{
	all_tileset_editor_dialogs().insert(this);

	set_clear_bg_amount(255);
	if(hex::hex_object::get_editor_tiles().empty() == false) {
		category_ = hex::hex_object::get_editor_tiles().front()->get_editor_info().group;
	}

	init();
}

hex_tileset_editor_dialog::~hex_tileset_editor_dialog()
{
	all_tileset_editor_dialogs().erase(this);
}

void hex_tileset_editor_dialog::init()
{
	clear();
	using namespace gui;
	set_padding(20);

	ASSERT_LOG(editor_.get_hex_tileset() >= 0 
		&& size_t(editor_.get_hex_tileset()) < hex::hex_object::get_editor_tiles().size(),
		"Index of hex tileset out of bounds must be between 0 and " 
		<< hex::hex_object::get_editor_tiles().size() << ", found " << editor_.get_hex_tileset());

	button* category_button = new button(widget_ptr(new label(category_, graphics::color_white())), boost::bind(&hex_tileset_editor_dialog::show_category_menu, this));
	add_widget(widget_ptr(category_button), 10, 10);

	grid_ptr grid(new gui::grid(3));
	int index = 0, first_index = -1;
	first_index_ = -1;
	
	foreach(const hex::hex_tile_ptr& t, hex::hex_object::get_editor_tiles()) {
		if(t->get_editor_info().group == category_) {
			if(first_index_ == -1) {
				first_index_ = index;
			}
			image_widget* preview = new image_widget(t->get_editor_info().texture, 54, 54);
			preview->set_area(t->get_editor_info().image_rect);
			button_ptr tileset_button(new button(widget_ptr(preview), boost::bind(&hex_tileset_editor_dialog::set_tileset, this, index)));
			tileset_button->set_tooltip(t->name() + "/" + t->get_editor_info().name, 14);
			tileset_button->set_dim(58, 58);
			grid->add_col(gui::widget_ptr(new gui::border_widget(tileset_button, index == editor_.get_hex_tileset() ? graphics::color(255,255,255,255) : graphics::color(0,0,0,0))));
		}
		++index;
	}

	grid->finish_row();
	add_widget(grid);
}

void hex_tileset_editor_dialog::select_category(const std::string& category)
{
	category_ = category;
	init();

	if(first_index_ != -1) {
		set_tileset(first_index_);
	}
}

void hex_tileset_editor_dialog::close_context_menu(int index)
{
	remove_widget(context_menu_);
	context_menu_.reset();
}

void hex_tileset_editor_dialog::show_category_menu()
{
	using namespace gui;
	gui::grid* grid = new gui::grid(2);
	grid->swallow_clicks();
	grid->set_show_background(true);
	grid->set_hpad(10);
	grid->allow_selection();
	grid->register_selection_callback(boost::bind(&hex_tileset_editor_dialog::close_context_menu, this, _1));

	std::set<std::string> categories;
	foreach(const hex::hex_tile_ptr& t, hex::hex_object::get_hex_tiles()) {
		if(categories.count(t->get_editor_info().group)) {
			continue;
		}

		categories.insert(t->get_editor_info().group);

		image_widget* preview = new image_widget(t->get_editor_info().texture, 54, 54);
		preview->set_area(t->get_editor_info().image_rect);
		grid->add_col(widget_ptr(preview))
		     .add_col(widget_ptr(new label(t->get_editor_info().group, graphics::color_white())));
		grid->register_row_selection_callback(boost::bind(&hex_tileset_editor_dialog::select_category, this, t->get_editor_info().group));
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	if(mousex + grid->width() > graphics::screen_width()) {
		mousex = graphics::screen_width() - grid->width();
	}

	if(mousey + grid->height() > graphics::screen_height()) {
		mousey = graphics::screen_height() - grid->height();
	}

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void hex_tileset_editor_dialog::set_tileset(int index)
{
	if(editor_.get_hex_tileset() != index) {
		editor_.set_hex_tileset(index);
		init();
	}
}

bool hex_tileset_editor_dialog::handle_event(const SDL_Event& event, bool claimed)
{
	if(!claimed) {
		if(context_menu_) {
			gui::widget_ptr ptr = context_menu_;
			SDL_Event ev = event;
			normalize_event(&ev);
			return ptr->process_event(ev, claimed);
		}

		switch(event.type) {
		case SDL_KEYDOWN:
			if(event.key.keysym.sym == SDLK_COMMA) {
				editor_.set_hex_tileset(editor_.get_hex_tileset()-1);
				while(hex::hex_object::get_hex_tiles()[editor_.get_hex_tileset()]->get_editor_info().group != category_) {
					editor_.set_hex_tileset(editor_.get_hex_tileset()-1);
				}
				set_tileset(editor_.get_hex_tileset());
				claimed = true;
			} else if(event.key.keysym.sym == SDLK_PERIOD) {
				editor_.set_hex_tileset(editor_.get_hex_tileset()+1);
				while(hex::hex_object::get_hex_tiles()[editor_.get_hex_tileset()]->get_editor_info().group != category_) {
					editor_.set_hex_tileset(editor_.get_hex_tileset()+1);
				}
				set_tileset(editor_.get_hex_tileset());
				claimed = true;
			}
			break;
		}
	}

	return dialog::handle_event(event, claimed);
}

}

#endif // !NO_EDITOR
