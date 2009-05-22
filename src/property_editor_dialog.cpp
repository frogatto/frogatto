#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "property_editor_dialog.hpp"
#include "raster.hpp"
#include "text_entry_widget.hpp"

namespace editor_dialogs
{

property_editor_dialog::property_editor_dialog(editor& e)
  : gui::dialog(graphics::screen_width() - 160, 160, 160, 440), editor_(e)
{
	init();
}

void property_editor_dialog::init()
{
	clear();
	if(!entity_) {
		return;
	}

	using namespace gui;

	set_padding(20);

	const frame& frame = entity_->current_frame();
	image_widget* preview = new image_widget(frame.img());
	preview->set_dim(frame.width(), frame.height());
	preview->set_area(frame.area());
	add_widget(widget_ptr(preview), 10, 10);

	if(entity_->label().empty() == false) {
		add_widget(widget_ptr(new label(entity_->label(), graphics::color_white())));
	}

	add_widget(widget_ptr(new button(widget_ptr(new label("Set Label", graphics::color_white())), boost::bind(&property_editor_dialog::set_label_dialog, this))));

	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		std::vector<game_logic::formula_input> inputs;
		vars->get_inputs(&inputs);
		foreach(const game_logic::formula_input& in, inputs) {
			std::ostringstream s;
			s << in.name << ": " << vars->query_value(in.name).to_debug_string();
			label_ptr lb = label::create(s.str(), graphics::color_white());
			add_widget(widget_ptr(lb));

			const editor_variable_info* var_info = entity_->editor_info() ? entity_->editor_info()->get_var_info(in.name) : NULL;

			if(var_info && var_info->type() == editor_variable_info::TYPE_LEVEL) {
				std::string current_value;
				variant current_value_var = entity_->query_value(in.name);
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				add_widget(widget_ptr(new button(
				         widget_ptr(new label(current_value.empty() ? "(set level)" : current_value, graphics::color_white())),
				         boost::bind(&property_editor_dialog::change_level_property, this, in.name))));
			} else if(var_info && var_info->type() == editor_variable_info::TYPE_LABEL) {
				std::string current_value;
				variant current_value_var = entity_->query_value(in.name);
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				add_widget(widget_ptr(new button(
				         widget_ptr(new label(current_value.empty() ? "(set label)" : current_value, graphics::color_white())),
				         boost::bind(&property_editor_dialog::change_label_property, this, in.name))));
				
			} else {
				grid_ptr buttons_grid(new grid(4));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, -10))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, -1))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, +1))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, +10))));
				add_widget(widget_ptr(buttons_grid));
			}
		}
	}
}

void property_editor_dialog::set_entity(entity_ptr e)
{
	entity_ = e;
	init();
}

void property_editor_dialog::change_property(const std::string& id, int change)
{
	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		vars->mutate_value(id, vars->query_value(id) + variant(change));
		init();
	}
}

void property_editor_dialog::change_level_property(const std::string& id)
{
	using namespace gui;
	gui::grid* grid = new gui::grid(1);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->register_selection_callback(boost::bind(&property_editor_dialog::set_level_property, this, id, _1));
	
	std::vector<std::string> levels = get_known_levels();
	foreach(const std::string& lvl, levels) {
		grid->add_col(widget_ptr(new label(lvl, graphics::color_white())));
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	mousex -= x();
	mousey -= y();

	if(grid->height() > graphics::screen_height() - mousey) {
		mousey = graphics::screen_height() - grid->height();
	}

	if(grid->width() > graphics::screen_width() - mousex) {
		mousex = graphics::screen_width() - grid->width();
	}

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void property_editor_dialog::set_level_property(const std::string& id, int index)
{
	remove_widget(context_menu_);
	context_menu_.reset();

	std::vector<std::string> levels = get_known_levels();
	if(index < 0 || index >= levels.size()) {
		init();
		return;
	}

	const std::string& lvl = levels[index];
	entity_->mutate_value(id, variant(lvl));

	init();
}

void property_editor_dialog::set_label_dialog()
{
	using namespace gui;
	text_entry_widget* entry = new text_entry_widget;
	dialog d(200, 200, 400, 200);
	d.add_widget(widget_ptr(new label("Label:", graphics::color_white())))
	 .add_widget(widget_ptr(entry));
	d.show_modal();
	entity_->set_label(entry->text());
	init();
}

void property_editor_dialog::change_label_property(const std::string& id)
{
	const editor_variable_info* var_info = entity_->editor_info() ? entity_->editor_info()->get_var_info(id) : NULL;
	if(!var_info) {
		return;
	}
	
	bool loaded_level = false;
	std::vector<std::string> labels;
	if(var_info->info().empty() == false && var_info->info() != editor_.get_level().id()) {
		variant level_id = entity_->query_value(var_info->info());
		if(level_id.is_string() && level_id.as_string().empty() == false && level_id.as_string() != editor_.get_level().id()) {
			level lvl(level_id.as_string());
			lvl.get_all_labels(labels);
			loaded_level = true;
		}
	}

	if(!loaded_level) {
		editor_.get_level().get_all_labels(labels);
	}

	if(labels.empty() == false) {
		gui::grid* grid = new gui::grid(1);
		grid->set_show_background(true);
		grid->allow_selection();
		grid->register_selection_callback(boost::bind(&property_editor_dialog::set_label_property, this, id, labels, _1));
		foreach(const std::string& lb, labels) {
			grid->add_col(gui::widget_ptr(new gui::label(lb, graphics::color_white())));
		}

		int mousex, mousey;
		SDL_GetMouseState(&mousex, &mousey);
		mousex -= x();
		mousey -= y();

		if(grid->height() > graphics::screen_height() - mousey) {
			mousey = graphics::screen_height() - grid->height();
		}

		if(grid->width() > graphics::screen_width() - mousex) {
			mousex = graphics::screen_width() - grid->width();
		}

		remove_widget(context_menu_);
		context_menu_.reset(grid);
		add_widget(context_menu_, mousex, mousey);
	}
}

void property_editor_dialog::set_label_property(const std::string& id, const std::vector<std::string>& labels, int index)
{
	remove_widget(context_menu_);
	context_menu_.reset();
	if(index < 0 || index >= labels.size()) {
		init();
		return;
	}

	entity_->mutate_value(id, variant(labels[index]));

	init();
}

}
