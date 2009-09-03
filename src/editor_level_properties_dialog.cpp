#include "SDL.h"

#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>

#include "background.hpp"
#include "button.hpp"
#include "editor.hpp"
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

editor_level_properties_dialog::editor_level_properties_dialog(editor& e)
  : dialog(0, 0, graphics::screen_width(), graphics::screen_height()), editor_(e)
{
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

	add_widget(widget_ptr(new label("Statistics (whole level)", graphics::color_white(), 36)));

	std::vector<stats::record_ptr> stats = editor_.stats();
	add_stats(stats);

	if(!editor_.selection().empty()) {
		foreach(stats::record_ptr& s, stats) {
			const point loc = s->location();
			bool in_selection = false;
			const int TileSize = 32;
			foreach(const point& tile, editor_.selection().tiles) {
				if(loc.x >= tile.x*TileSize && loc.y >= tile.y*TileSize && loc.x < (tile.x+1)*TileSize && loc.y < (tile.y+1)*TileSize) {
					in_selection = true;
					break;
				}
			}

			if(!in_selection) {
				s = stats::record_ptr();
			}
		}

		add_widget(widget_ptr(new label("Statistics (selection)", graphics::color_white(), 36)));
		add_stats(stats);
	}
}

void editor_level_properties_dialog::add_stats(const std::vector<stats::record_ptr>& stats)
{
	using namespace gui;

	std::map<std::string, int> stats_counts;
	foreach(stats::record_ptr r, stats) {
		if(r) {
			stats_counts[r->id()]++;
		}
	}

	const int seconds_play_time = stats_counts["move"]/5;
	stats_counts["Time Played"] = seconds_play_time;
	if(stats_counts["die"] > 0) {
		stats_counts["Seconds per Death"] = seconds_play_time/stats_counts["die"];
	}

	grid_ptr g(new grid(2));
	g->set_hpad(10);
	for(std::map<std::string, int>::const_iterator i = stats_counts.begin();
	    i != stats_counts.end(); ++i) {
		g->add_col(widget_ptr(new label(i->first, graphics::color_white())));
		g->add_col(widget_ptr(new label(formatter() << i->second, graphics::color_white())));
	}

	add_widget(g);
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

	editor_.get_level().set_title(title);

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

	editor_.get_level().set_background_by_id(choices[index]);
	init();
}

}
