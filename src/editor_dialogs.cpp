#ifndef NO_EDITOR
#include <boost/bind.hpp>

#include "dialog.hpp"
#include "editor_dialogs.hpp"
#include "foreach.hpp"
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
	std::vector<std::string> levels = get_known_levels();
	gui::grid* grid = new gui::grid(1);
	grid->set_max_height(graphics::screen_height() - 80);
	grid->set_show_background(true);
	grid->allow_selection();

	grid->register_selection_callback(boost::bind(&do_select_level, &d, levels, _1, &result));
	foreach(const std::string& lvl, levels) {
		grid->add_col(widget_ptr(new label(lvl, graphics::color_white())));
	}

	d.add_widget(widget_ptr(grid));
	d.show_modal();
	return result;
}
#endif // NO_EDITOR

