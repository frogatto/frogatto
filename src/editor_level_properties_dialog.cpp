#include "graphics.hpp"

#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>

#include "background.hpp"
#include "button.hpp"
#include "checkbox.hpp"
#include "editor.hpp"
#include "editor_dialogs.hpp"
#include "editor_level_properties_dialog.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "stats.hpp"
#include "text_entry_widget.hpp"

namespace editor_dialogs
{

namespace {

void set_segmented_level_width(editor_level_properties_dialog* d, editor* e, bool value)
{
	foreach(level_ptr lvl, e->get_level_list()) {
		if(value) {
			//make sure the segment width is divisible by the tile size.
			int width = lvl->boundaries().w();
			while(width%32) {
				++width;
			}
			lvl->set_segment_width(width);
			lvl->set_boundaries(rect(lvl->boundaries().x(), lvl->boundaries().y(),
			                         width, lvl->boundaries().h()));
		} else {
			lvl->set_segment_width(0);
		}
	}

	d->init();
}

void set_segmented_level_height(editor_level_properties_dialog* d, editor* e, bool value)
{
	foreach(level_ptr lvl, e->get_level_list()) {
		if(value) {
			//make sure the segment height is divisible by the tile size.
			int height = lvl->boundaries().h();
			while(height%32) {
				++height;
			}
			lvl->set_segment_height(height);
			lvl->set_boundaries(rect(lvl->boundaries().x(), lvl->boundaries().y(),
			                         lvl->boundaries().w(), height));
		} else {
			lvl->set_segment_height(0);
		}
	}
	
	d->init();
}

}

editor_level_properties_dialog::editor_level_properties_dialog(editor& e)
  : dialog(0, 0, graphics::screen_width(), graphics::screen_height()), editor_(e)
{
	set_clear_bg_amount(255);
	init();
}

void editor_level_properties_dialog::init()
{
	using namespace gui;
	clear();

	add_widget(widget_ptr(new label("Level Properties", graphics::color_white(), 48)), 10, 10);

	grid_ptr g(new grid(2));
	g->add_col(widget_ptr(new label(editor_.get_level().title(), graphics::color_white(), 36)))
	  .add_col(widget_ptr(new button(widget_ptr(new label("Change Title", graphics::color_white())), boost::bind(&editor_level_properties_dialog::change_title, this))));

	add_widget(g);

	std::string background_id = editor_.get_level().get_background_id();
	if(background_id.empty()) {
		background_id = "(no background)";
	}
	g.reset(new grid(2));
	g->add_col(widget_ptr(new label("Background", graphics::color_white())))
	  .add_col(widget_ptr(new button(widget_ptr(new label(background_id, graphics::color_white())), boost::bind(&editor_level_properties_dialog::change_background, this))));
	add_widget(g);

	g.reset(new grid(3));
	g->set_hpad(10);
	g->add_col(widget_ptr(new label("Next Level", graphics::color_white())));
	g->add_col(widget_ptr(new label(editor_.get_level().next_level(), graphics::color_white())));
	g->add_col(widget_ptr(new button(widget_ptr(new label("Set", graphics::color_white())), boost::bind(&editor_level_properties_dialog::change_next_level, this))));

	g->add_col(widget_ptr(new label("Previous Level", graphics::color_white())));
	g->add_col(widget_ptr(new label(editor_.get_level().previous_level(), graphics::color_white())));
	g->add_col(widget_ptr(new button(widget_ptr(new label("Set", graphics::color_white())), boost::bind(&editor_level_properties_dialog::change_previous_level, this))));
	add_widget(g);

	checkbox* hz_segmented_checkbox = new checkbox("Horizontally Segmented Level", editor_.get_level().segment_width() != 0, boost::bind(set_segmented_level_width, this, &editor_, _1));
	widget_ptr hz_checkbox(hz_segmented_checkbox);
	add_widget(hz_checkbox);

	checkbox* vt_segmented_checkbox = new checkbox("Vertically Segmented Level", editor_.get_level().segment_height() != 0, boost::bind(set_segmented_level_height, this, &editor_, _1));
	widget_ptr vt_checkbox(vt_segmented_checkbox);
	add_widget(vt_checkbox);

	if(editor_.get_level().segment_height() != 0) {
		remove_widget(hz_checkbox);
	}

	if(editor_.get_level().segment_width() != 0) {
		remove_widget(vt_checkbox);
	}
}

void editor_level_properties_dialog::change_title()
{
	using namespace gui;
	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(new label("Change Title", graphics::color_white(), 48)));
	text_entry_widget* entry = new text_entry_widget;
	d.add_widget(widget_ptr(new label("Name:", graphics::color_white())))
	 .add_widget(widget_ptr(entry));
	d.show_modal();

	if(d.cancelled()) {
		return;
	}

	std::string title = entry->text();

	foreach(level_ptr lvl, editor_.get_level_list()) {
		lvl->set_title(title);
	}

	init();
}

void editor_level_properties_dialog::change_background()
{
	using namespace gui;
	std::vector<std::string> backgrounds = background::get_available_backgrounds();
	if(backgrounds.empty()) {
		return;
	}

	std::sort(backgrounds.begin(), backgrounds.end());

	gui::grid* grid = new gui::grid(1);
	grid->set_hpad(40);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();
	grid->register_selection_callback(boost::bind(&editor_level_properties_dialog::execute_change_background, this, backgrounds, _1));
	foreach(const std::string& bg, backgrounds) {
		grid->add_col(widget_ptr(new label(bg, graphics::color_white())));
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void editor_level_properties_dialog::execute_change_background(const std::vector<std::string>& choices, int index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}

	if(index < 0 || index >= choices.size()) {
		return;
	}

	foreach(level_ptr lvl, editor_.get_level_list()) {
		lvl->set_background_by_id(choices[index]);
	}

	init();
}

void editor_level_properties_dialog::change_next_level()
{
	std::string result = show_choose_level_dialog("Next Level");
	if(result.empty() == false) {
		foreach(level_ptr lvl, editor_.get_level_list()) {
			lvl->set_next_level(result);
		}
	}

	init();
}

void editor_level_properties_dialog::change_previous_level()
{
	std::string result = show_choose_level_dialog("Previous Level");
	if(result.empty() == false) {
		foreach(level_ptr lvl, editor_.get_level_list()) {
			lvl->set_previous_level(result);
		}
	}

	init();
}

}
