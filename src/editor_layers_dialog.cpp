#ifndef NO_EDITOR
#include <boost/bind.hpp>

#include "editor_layers_dialog.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "level.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

editor_layers_dialog::editor_layers_dialog(editor& e)
  : dialog(graphics::screen_width() - 200, LAYERS_DIALOG_WIDTH, 40, graphics::screen_height() - 40), editor_(e), locked_(false)
{
	set_clear_bg_amount(255);
	init();
}

void editor_layers_dialog::init()
{
	clear();
	rows_.clear();

	using namespace gui;
	grid_ptr g(new grid(2));

	std::set<int> all_layers, visible_layers;
	editor_.get_level().get_tile_layers(&all_layers, &visible_layers);

	foreach(int layer, all_layers) {
		const bool hidden = visible_layers.count(layer);
		gui_section_widget* section = new gui_section_widget(hidden ? "checkbox-empty" : "checkbox-filled");

		row_data row = { section, layer, hidden };
		rows_.push_back(row);
		g->add_col(widget_ptr(section));
		g->add_col(widget_ptr(new label(formatter() << layer, graphics::color_white())));
	}

	gui_section_widget* section = new gui_section_widget(locked_ ? "checkbox-filled" : "checkbox-empty");
	g->add_col(widget_ptr(section));
	g->add_col(widget_ptr(new label("lock", graphics::color_white())));

	g->allow_selection();
	g->register_selection_callback(boost::bind(&editor_layers_dialog::row_selected, this, _1));
	g->register_mouseover_callback(boost::bind(&editor_layers_dialog::row_mouseover, this, _1));
	
	add_widget(g, 0, 0);
}

void editor_layers_dialog::process()
{
	const int index = editor_.get_tileset();
	if(index < 0 || index >= editor_.all_tilesets().size()) {
		return;
	}

	if(locked_) {
		const editor::tileset& t = editor_.all_tilesets()[index];
		std::set<int> all_layers, visible_layers;
		editor_.get_level().get_tile_layers(&all_layers, &visible_layers);
		std::cerr << "LOCKED.. " << visible_layers.size() << "\n";

		if(visible_layers.size() != 1 || *visible_layers.begin() != t.zorder) {
			std::cerr << "CHANGING LOCK\n";
			foreach(level_ptr lvl, editor_.get_level_list()) {
				foreach(int layer, all_layers) {
					lvl->hide_tile_layer(layer, true);
				}

				lvl->hide_tile_layer(t.zorder, false);
			}

			init();
		}
	}
}

void editor_layers_dialog::row_selected(int nrow)
{
	if(nrow == rows_.size()) {
		locked_ = !locked_;
		init();
		return;
	}
	
	if(nrow < 0 || nrow >= rows_.size()) {
		return;
	}

	locked_ = false;

	foreach(level_ptr lvl, editor_.get_level_list()) {
		lvl->hide_tile_layer(rows_[nrow].layer, !rows_[nrow].hidden);
	}

	init();
}

void editor_layers_dialog::row_mouseover(int nrow)
{
	if(nrow < 0 || nrow >= rows_.size()) {
		editor_.get_level().highlight_tile_layer(INT_MIN);
		return;
	}

	editor_.get_level().highlight_tile_layer(rows_[nrow].layer);
}

}
#endif // !NO_EDITOR

