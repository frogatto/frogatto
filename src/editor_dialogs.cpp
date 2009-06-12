#include <boost/bind.hpp>

#include "dialog.hpp"
#include "editor_dialogs.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "raster.hpp"
#include "widget.hpp"

namespace {
void do_select_level(gui::dialog* d, const std::vector<std::string>& levels, int index, std::string* result) {
	if(index >= 0 && index < levels.size()) {
		d->close();
		*result = levels[index];
	}
}
}

std::string show_choose_level_dialog(const std::string& prompt)
{
	using namespace gui;
	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(new label(prompt, graphics::color_white(), 48)));

	std::string result;
	const int levels_per_col = 20;
	std::vector<std::string> levels = get_known_levels();
	gui::grid* parent_grid = new gui::grid(levels.size()/levels_per_col + (levels.size()%levels_per_col ? 1 : 0));
	int index = 0;
	while(index < levels.size()) {
		const int end = std::min(index + levels_per_col, static_cast<int>(levels.size()));
		gui::grid* grid = new gui::grid(1);
		grid->set_show_background(true);
		grid->allow_selection();

		std::vector<std::string> levels_portion(levels.begin() + index, levels.end());
		grid->register_selection_callback(boost::bind(&do_select_level, &d, levels_portion, _1, &result));
		while(index != end) {
			grid->add_col(widget_ptr(new label(levels[index], graphics::color_white())));
			++index;
		}
		parent_grid->add_col(widget_ptr(grid));
	}

	d.add_widget(widget_ptr(parent_grid));
	d.show_modal();
	return result;
}
