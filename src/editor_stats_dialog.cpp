#include "SDL.h"

#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>

#include "background.hpp"
#include "button.hpp"
#include "editor.hpp"
#include "editor_stats_dialog.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "stats.hpp"
#include "text_entry_widget.hpp"

namespace editor_dialogs
{

editor_stats_dialog::editor_stats_dialog(editor& e)
  : dialog(0, 0, graphics::screen_width(), graphics::screen_height()), editor_(e)
{
	init();
}

void editor_stats_dialog::init()
{
	using namespace gui;
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

void editor_stats_dialog::add_stats(const std::vector<stats::record_ptr>& stats)
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

}
