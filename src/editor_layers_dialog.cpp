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

	std::set<int> all_layers, hidden_layers;
	editor_.get_level().get_tile_layers(&all_layers, &hidden_layers);

	foreach(int layer, all_layers) {
		const bool hidden = hidden_layers.count(layer);
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

	const int ypos = g->y() + g->height();

	find_classifications();
	g.reset(new grid(2));
	foreach(const std::string& classification, all_classifications_) {
		const bool hidden = editor_.get_level().hidden_object_classifications().count(classification) != 0;
		gui_section_widget* section = new gui_section_widget(hidden ? "checkbox-empty" : "checkbox-filled");
		g->add_col(widget_ptr(section));
		g->add_col(widget_ptr(new label(classification, graphics::color_white())));
	}

	g->allow_selection();
	g->register_selection_callback(boost::bind(&editor_layers_dialog::classification_selected, this, _1));

	add_widget(g, 0, ypos + 80);
}

void editor_layers_dialog::process()
{
	const int index = editor_.get_tileset();
	if(index < 0 || index >= editor_.all_tilesets().size()) {
		return;
	}

	if(locked_) {
		const editor::tileset& t = editor_.all_tilesets()[index];
		std::set<int> all_layers, hidden_layers;
		editor_.get_level().get_tile_layers(&all_layers, &hidden_layers);
		std::cerr << "LOCKED.. " << hidden_layers.size() << "\n";

		if(hidden_layers.size() != 1 || *hidden_layers.begin() != t.zorder) {
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

	const std::set<std::string> classifications = all_classifications_;
	find_classifications();
	if(classifications != all_classifications_) {
		init();
	}
}

void editor_layers_dialog::row_selected(int nrow)
{
	if(nrow == rows_.size()) {
		locked_ = !locked_;
		if(locked_) {
			std::set<int> all_layers, hidden_layers;
			editor_.get_level().get_tile_layers(&all_layers, &hidden_layers);
			before_locked_state_ = hidden_layers;
		} else if(!locked_) {
			std::set<int> all_layers, hidden_layers;
			editor_.get_level().get_tile_layers(&all_layers, &hidden_layers);
			foreach(level_ptr lvl, editor_.get_level_list()) {
				foreach(int layer, all_layers) {
					lvl->hide_tile_layer(layer, before_locked_state_.count(layer));
				}
			}
		}
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

void editor_layers_dialog::find_classifications()
{
	all_classifications_.clear();
	foreach(level_ptr lvl, editor_.get_level_list()) {
		foreach(entity_ptr e, lvl->get_chars()) {
			if(e->editor_info() && !e->editor_info()->classification().empty()) {
				all_classifications_.insert(e->editor_info()->classification());
			}
		}
	}

}

void editor_layers_dialog::classification_selected(int index)
{
	if(index < 0 || index >= all_classifications_.size()) {
		return;
	}

	std::set<std::string>::const_iterator itor = all_classifications_.begin();
	std::advance(itor, index);
	foreach(level_ptr lvl, editor_.get_level_list()) {
		lvl->hide_object_classification(*itor, !lvl->hidden_object_classifications().count(*itor));
	}

	init();
}

}
#endif // !NO_EDITOR

