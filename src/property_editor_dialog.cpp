#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "editor_dialogs.hpp"
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

	set_padding(5);

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
	if(entity_->editor_info()) {
		foreach(const editor_variable_info& info, entity_->editor_info()->vars()) {
			if(info.type() == editor_variable_info::XPOSITION ||
			   info.type() == editor_variable_info::YPOSITION) {
				//don't show x/y position as they are directly editable on
				//the map.
				continue;
			}

			std::ostringstream s;
			s << info.variable_name() << ": " << vars->query_value(info.variable_name()).to_debug_string();
			label_ptr lb = label::create(s.str(), graphics::color_white());
			add_widget(widget_ptr(lb));

			if(info.type() == editor_variable_info::TYPE_TEXT) {
				std::string current_value;
				variant current_value_var = entity_->query_value(info.variable_name());
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				add_widget(widget_ptr(new button(
				      widget_ptr(new label(current_value.empty() ? "(set text)" : current_value, graphics::color_white())),
				      boost::bind(&property_editor_dialog::change_text_property, this, info.variable_name()))));
			} else if(info.type() == editor_variable_info::TYPE_LEVEL) {
				std::string current_value;
				variant current_value_var = entity_->query_value(info.variable_name());
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				add_widget(widget_ptr(new button(
				         widget_ptr(new label(current_value.empty() ? "(set level)" : current_value, graphics::color_white())),
				         boost::bind(&property_editor_dialog::change_level_property, this, info.variable_name()))));
			} else if(info.type() == editor_variable_info::TYPE_LABEL) {
				std::string current_value;
				variant current_value_var = entity_->query_value(info.variable_name());
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				add_widget(widget_ptr(new button(
				         widget_ptr(new label(current_value.empty() ? "(set label)" : current_value, graphics::color_white())),
				         boost::bind(&property_editor_dialog::change_label_property, this, info.variable_name()))));
				
			} else if(info.type() == editor_variable_info::TYPE_BOOLEAN) {
				variant current_value = entity_->query_value(info.variable_name());
				std::cerr << "CURRENT VALUE: " << current_value.as_bool() << "\n";
				add_widget(widget_ptr(new button(
				         widget_ptr(new label("toggle", graphics::color_white())),
				         boost::bind(&property_editor_dialog::change_property, this, info.variable_name(), current_value.as_bool() ? -current_value.as_int() : 1))));
			} else {
				grid_ptr buttons_grid(new grid(4));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, info.variable_name(), -10))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, info.variable_name(), -1))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, info.variable_name(), +1))));
				buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, info.variable_name(), +10))));
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
	std::cerr << "CHANGE PROPERTY: " << change << "\n";
	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		vars->mutate_value(id, vars->query_value(id) + variant(change));
		init();
	}
}

void property_editor_dialog::change_level_property(const std::string& id)
{
	std::string lvl = show_choose_level_dialog("Set " + id);
	if(lvl.empty() == false) {
		entity_->mutate_value(id, variant(lvl));
		init();
	}
}

void property_editor_dialog::set_label_dialog()
{
	using namespace gui;
	text_entry_widget* entry = new text_entry_widget;
	dialog d(200, 200, 400, 200);
	d.add_widget(widget_ptr(new label("Label:", graphics::color_white())))
	 .add_widget(widget_ptr(entry));
	d.show_modal();

	//we have to add and remove the character from the level to
	//modify their label properly.
	editor_.get_level().remove_character(entity_);
	entity_->set_label(entry->text());
	editor_.get_level().add_character(entity_);
	init();
}

void property_editor_dialog::change_text_property(const std::string& id)
{
	//show a text box to set the new text
	using namespace gui;
	text_entry_widget* entry = new text_entry_widget;
	dialog d(200, 200, 400, 200);
	d.add_widget(widget_ptr(new label("Label:", graphics::color_white())))
	 .add_widget(widget_ptr(entry));
	d.show_modal();

	//set the variable to the new value
	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		vars->mutate_value(id, variant(entry->text()));
		init();
	}
}

namespace {
bool hidden_label(const std::string& label) {
	return label.empty() || label[0] == '_';
}
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
			lvl.finish_loading();
			lvl.get_all_labels(labels);
			loaded_level = true;
		}
	}

	if(!loaded_level) {
		editor_.get_level().get_all_labels(labels);
	}

	labels.erase(std::remove_if(labels.begin(), labels.end(), hidden_label), labels.end());

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
