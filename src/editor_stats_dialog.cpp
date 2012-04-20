#ifndef NO_EDITOR
#include "graphics.hpp"

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
	set_clear_bg_amount(255);
	init();
}

void editor_stats_dialog::init()
{
	using namespace gui;
	add_widget(widget_ptr(new label("Statistics (whole level)", graphics::color_white(), 36)));
/*
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
	*/
}

}
#endif // !NO_EDITOR

