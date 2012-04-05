#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "editor_dialogs.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
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
  : gui::dialog(graphics::screen_width() - EDITOR_SIDEBAR_WIDTH, 160, EDITOR_SIDEBAR_WIDTH, 440), editor_(e)
{
	set_clear_bg_amount(255);
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

	grid_ptr preview_grid(new grid(2));
	preview_grid->add_col(widget_ptr(preview));

	//draw the object's difficulty settings.
	grid_ptr difficulty_grid(new grid(3));
	const custom_object* obj = dynamic_cast<const custom_object*>(entity_.get());
	ASSERT_LOG(obj, "ENTITY IS NOT AN OBJECT");
	std::string min_difficulty = "-", max_difficulty = "-";
	if(obj->min_difficulty() != -1) {
		min_difficulty = formatter() << obj->min_difficulty();
	}

	if(obj->max_difficulty() != -1) {
		max_difficulty = formatter() << obj->max_difficulty();
	}

	if(min_difficulty.size() == 1) {
		min_difficulty = " " + min_difficulty;
	}

	if(max_difficulty.size() == 1) {
		max_difficulty = " " + max_difficulty;
	}

	difficulty_grid->add_col(widget_ptr(new label(min_difficulty, graphics::color_white())));

	button_ptr difficulty_button;
	difficulty_button.reset(new button(widget_ptr(new label("-", graphics::color_white())), boost::bind(&property_editor_dialog::change_min_difficulty, this, -1)));
	difficulty_button->set_tooltip("Decrease minimum difficulty");
	difficulty_button->set_dim(difficulty_button->width()-10, difficulty_button->height()-4);
	difficulty_grid->add_col(difficulty_button);
	difficulty_button.reset(new button(widget_ptr(new label("+", graphics::color_white())), boost::bind(&property_editor_dialog::change_min_difficulty, this, 1)));
	difficulty_button->set_tooltip("Increase minimum difficulty");
	difficulty_button->set_dim(difficulty_button->width()-10, difficulty_button->height()-4);
	difficulty_grid->add_col(difficulty_button);

	difficulty_grid->add_col(widget_ptr(new label(max_difficulty, graphics::color_white())));
	difficulty_button.reset(new button(widget_ptr(new label("-", graphics::color_white())), boost::bind(&property_editor_dialog::change_max_difficulty, this, -1)));
	difficulty_button->set_tooltip("Decrease maximum difficulty");
	difficulty_button->set_dim(difficulty_button->width()-10, difficulty_button->height()-4);
	difficulty_grid->add_col(difficulty_button);
	difficulty_button.reset(new button(widget_ptr(new label("+", graphics::color_white())), boost::bind(&property_editor_dialog::change_max_difficulty, this, 1)));
	difficulty_button->set_tooltip("Increase maximum difficulty");
	difficulty_button->set_dim(difficulty_button->width()-10, difficulty_button->height()-4);
	difficulty_grid->add_col(difficulty_button);


	preview_grid->add_col(difficulty_grid);
	
	add_widget(preview_grid, 10, 10);

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

			variant val = vars->query_value(info.variable_name());
			std::string current_val_str;
			if(info.type() == editor_variable_info::TYPE_POINTS) {
				if(!val.is_list()) {
					current_val_str = "null";
				} else {
					current_val_str = formatter() << val.num_elements() << " points";
				}
			} else {
				current_val_str = val.to_debug_string();
			}

			std::ostringstream s;
			s << info.variable_name() << ": " << current_val_str;
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
			} else if(info.type() == editor_variable_info::TYPE_ENUM) {
				std::string current_value;
				variant current_value_var = entity_->query_value(info.variable_name());
				if(current_value_var.is_string()) {
					current_value = current_value_var.as_string();
				}

				if(std::count(info.enum_values().begin(), info.enum_values().end(), current_value) == 0) {
					current_value = info.enum_values().front();
				}

				add_widget(widget_ptr(new button(
				     widget_ptr(new label(current_value, graphics::color_white())),
					 boost::bind(&property_editor_dialog::change_enum_property, this, info.variable_name()))));
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
				         boost::bind(&property_editor_dialog::toggle_property, this, info.variable_name()))));
			} else if(info.type() == editor_variable_info::TYPE_POINTS) {
				variant current_value = entity_->query_value(info.variable_name());
				const int npoints = current_value.is_list() ? current_value.num_elements() : 0;

				const bool already_adding = editor_.adding_points() == info.variable_name();
				add_widget(widget_ptr(new button(already_adding ? "Done Adding" : "Add Points",
						 boost::bind(&property_editor_dialog::change_points_property, this, info.variable_name()))));

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

void property_editor_dialog::change_min_difficulty(int amount)
{
	custom_object* obj = dynamic_cast<custom_object*>(entity_.get());
	ASSERT_LOG(obj, "ENTITY IS NOT AN OBJECT");

	const int new_difficulty = std::max<int>(-1, obj->min_difficulty() + amount);
	obj->set_difficulty(new_difficulty, obj->max_difficulty());
	init();
}

void property_editor_dialog::change_max_difficulty(int amount)
{
	custom_object* obj = dynamic_cast<custom_object*>(entity_.get());
	ASSERT_LOG(obj, "ENTITY IS NOT AN OBJECT");

	const int new_difficulty = std::max<int>(-1, obj->max_difficulty() + amount);
	obj->set_difficulty(obj->min_difficulty(), new_difficulty);
	init();
}

void property_editor_dialog::toggle_property(const std::string& id)
{
	mutate_value(id, variant(!entity_->query_value(id).as_bool()));
	init();
}

void property_editor_dialog::change_property(const std::string& id, int change)
{
	mutate_value(id, entity_->query_value(id) + variant(change));
	init();
}

void property_editor_dialog::change_level_property(const std::string& id)
{
	std::string lvl = show_choose_level_dialog("Set " + id);
	if(lvl.empty() == false) {
		mutate_value(id, variant(lvl));
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

	mutate_value(id, variant(entry->text()));
	init();
}

void property_editor_dialog::change_enum_property(const std::string& id)
{
	const editor_variable_info* var_info = entity_->editor_info() ? entity_->editor_info()->get_var_info(id) : NULL;
	if(!var_info) {
		return;
	}

	using namespace gui;

	gui::grid* grid = new gui::grid(1);
	grid->set_show_background(true);
	grid->allow_selection();

	grid->register_selection_callback(boost::bind(&property_editor_dialog::set_enum_property, this, id, var_info->enum_values(), _1));
	foreach(const std::string& s, var_info->enum_values()) {
		grid->add_col(gui::widget_ptr(new gui::label(s, graphics::color_white())));
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
		grid->register_selection_callback(boost::bind(&property_editor_dialog::set_enum_property, this, id, labels, _1));
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

void property_editor_dialog::set_enum_property(const std::string& id, const std::vector<std::string>& labels, int index)
{
	remove_widget(context_menu_);
	context_menu_.reset();
	if(index < 0 || index >= labels.size()) {
		init();
		return;
	}

	mutate_value(id, variant(labels[index]));
	init();
}

void property_editor_dialog::change_points_property(const std::string& id)
{
	//Toggle whether we're adding points or not.
	if(editor_.adding_points() == id) {
		editor_.start_adding_points("");
	} else {
		editor_.start_adding_points(id);
	}
}

void property_editor_dialog::mutate_value(const std::string& key, variant value)
{
	foreach(level_ptr lvl, editor_.get_level_list()) {
		entity_ptr e = lvl->get_entity_by_label(entity_->label());
		if(e) {
			editor_.mutate_object_value(e, key, value);
		}
	}
}

}
