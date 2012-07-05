#ifndef NO_EDITOR

#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <climits>

#include <math.h>

#include "animation_creator.hpp"
#include "animation_widget.hpp"
#include "asserts.hpp"
#include "button.hpp"
#include "code_editor_widget.hpp"
#include "custom_object_dialog.hpp"
#include "draw_scene.hpp"
#include "dropdown_widget.hpp"
#include "file_chooser_dialog.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "grid_widget.hpp"
#include "json_parser.hpp"
#include "graphics.hpp"
#include "label.hpp"
#include "module.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"
#include "tree_view_widget.hpp"

#define TEMPLATE_DIRECTORY	"data/object_templates/"

namespace gui {

class item_edit_dialog : public virtual dialog
{
public:
	item_edit_dialog(int x, int y, int w, int h, const std::string&name, variant items) 
		: dialog(x,y,w,h), items_(items), display_name_(name), allow_functions_(false),
		row_count_(0)
	{
		if(items_.is_map() == false) {
			std::map<variant, variant> m;
			items_ = variant(&m);
		}
		init();
	}
	virtual ~item_edit_dialog() 
	{}
	variant get_items() const { return item_grid_->get_tree(); }
	void allow_functions(bool val=true) { allow_functions_ = val; }
protected:
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	void init();
	void on_save();
	bool has_keyboard_focus();

	void editor_select(variant* v, boost::function<void(const variant&)> save_fn);
	void string_entry_save();
	void string_entry_discard();
private:
	std::string display_name_;
	variant items_;
	bool allow_functions_;
	widget_ptr context_menu_;

	tree_editor_widget_ptr item_grid_;
	code_editor_widget_ptr string_entry_;
	button_ptr save_text_button_;
	button_ptr discard_text_button_;
	grid_ptr text_button_grid;
	boost::function<void(const variant&)> save_fn_;
	int row_count_;
	std::string saved_text_;
};

}

namespace {

int slider_transform(double d)
{
	// normalize to [-20.0,20.0] range.
	d = (d - 0.5) * 2.0 * 20;
	double d_abs = abs(d);
	if(d_abs > 10) {
		// Above 10 units we go non-linear.
		return int((d < 0 ? -1.0 : 1.0) * pow(10, d_abs/10));
	}
	return int(d);
}

module::module_file_map& get_template_path()
{
	static module::module_file_map dialog_file_map;
	return dialog_file_map;
}

void load_template_file_paths(const std::string& path)
{
	if(get_template_path().empty()) {
		module::get_unique_filenames_under_dir(path, &get_template_path());
	}
}

void do_draw_scene() {
	draw_scene(level::current(), last_draw_position());
}

void load_default_attributes(std::vector<std::string>& d)
{
	//d.push_back("id");
	//d.push_back("animation");
	//d.push_back("editor_info");
	d.push_back("prototype");
	d.push_back("hitpoints");
	d.push_back("mass");
	d.push_back("vars");
	d.push_back("friction");
	d.push_back("traction");
	d.push_back("traction_in_air");
}

std::vector<std::string>& get_default_attribute_list()
{
	static std::vector<std::string> defaults;
	if(defaults.empty()) {
		load_default_attributes(defaults);
	}
	return defaults;
}

void reset_dialog_paths()
{
	get_template_path().clear();
}

std::string get_dialog_file(const std::string& fname)
{
	load_template_file_paths(TEMPLATE_DIRECTORY);
	module::module_file_map::const_iterator it = module::find(get_template_path(), fname);
	ASSERT_LOG(it != get_template_path().end(), "OBJECT TEMPLATE FILE NOT FOUND: " << fname);
	return it->second;
}


std::string get_id_from_filemap(std::pair<std::string, std::string> p)
{
	std::string s = module::get_id(p.first);
	if(s.length() > 4 && s.substr(s.length()-4) == ".cfg") {
		return s.substr(0, s.length()-4);
	}
	return s;
}

void do_nothing()
{
}

}

namespace editor_dialogs {

custom_object_dialog::custom_object_dialog(editor& e, int x, int y, int w, int h)
	: gui::dialog(x,y,w,h), dragging_slider_(false), selected_template_(0)
{
	load_template_file_paths(TEMPLATE_DIRECTORY);
	set_clear_bg_amount(255);
	std::map<variant, variant> m;
	object_template_ = variant(&m);
	current_object_save_path_ = module::get_module_path() + "data/objects/";
	std::transform(prototype_file_paths().begin(), prototype_file_paths().end(), std::back_inserter(prototypes_), get_id_from_filemap);
	std::sort(prototypes_.begin(), prototypes_.end());
	init();
}


void custom_object_dialog::init()
{
	const int border_offset = 30;
	using namespace gui;
	clear();

	add_widget(widget_ptr(new label("Object Properties", graphics::color_white(), 20)), border_offset, border_offset);

	grid_ptr container(new grid(1));
	container->set_col_width(0, width() - border_offset);

	// Get choices for dropdown list.
	std::vector<std::string> template_choices;
	std::transform(get_template_path().begin(), get_template_path().end(), std::back_inserter(template_choices),
		boost::bind(&module::module_file_map::value_type::first,_1));
	std::sort(template_choices.begin(), template_choices.end());
	template_choices.insert(template_choices.begin(), "Blank");

	dropdown_widget_ptr template_dropdown(new dropdown_widget(template_choices, 200, 30, dropdown_widget::DROPDOWN_LIST));
	template_dropdown->set_dropdown_height(100);
	template_dropdown->set_on_select_handler(boost::bind(&custom_object_dialog::change_template, this, _1, _2));
	template_dropdown->set_selection(selected_template_);

	grid_ptr g(new grid(4));
	g->set_hpad(20);
	g->set_zorder(1);
	g->add_col(widget_ptr(new label("Template  ", graphics::color_white(), 14)))
		.add_col(template_dropdown);
	text_editor_widget_ptr change_entry(new text_editor_widget(200, 28));
	change_entry->set_font_size(14);
	if(object_template_.has_key("id")) {
		change_entry->set_text(object_template_["id"].as_string());
	}
	change_entry->set_on_change_handler(boost::bind(&custom_object_dialog::change_text_attribute, this, change_entry, "id"));
	change_entry->set_on_enter_handler(boost::bind(&custom_object_dialog::init, this));
	change_entry->set_on_tab_handler(boost::bind(&custom_object_dialog::init, this));
	change_entry->set_on_esc_handler(boost::bind(&custom_object_dialog::init, this));
	change_entry->set_on_change_focus_handler(boost::bind(&custom_object_dialog::id_change_focus, this, _1));
	g->add_col(widget_ptr(new label("id: ", graphics::color_white(), 14)))
		.add_col(widget_ptr(change_entry));
	container->add_col(g);

	g.reset(new grid(4));
	g->add_col(widget_ptr(new button(new label("Animations", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_animations, this))));
	g->add_col(widget_ptr(new button(new label("Variables", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_items, this, "Variables Editor", "vars", false))));
	g->add_col(widget_ptr(new button(new label("Properties", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_items, this, "Properties Editor", "properties", true))));
	g->add_col(widget_ptr(new button(new label("Editor Info", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_items, this, "Editor Info", "editor_info", false))));
	container->add_col(g);

	if(template_file_.first.empty()) {
		foreach(const std::string& attr, get_default_attribute_list()) {
			std::vector<widget_ptr> widget_list = get_widget_for_attribute(attr);
			foreach(const widget_ptr& w, widget_list) {
				if(w) {
					container->add_col(w);
				}
			}
		}
	} else {
		std::vector<variant> keys = object_template_.get_keys().as_list();
		foreach(const variant& v, keys) {
			std::vector<widget_ptr> widget_list = get_widget_for_attribute(v.as_string());
			foreach(const widget_ptr& w, widget_list) {
				if(w) {
					container->add_col(w);
				}
			}
		}
	}

	error_text_.clear();
	assert_recover_scope recover_from_assert;
	try {
		object_ = custom_object_type_ptr(new custom_object_type(object_template_, NULL, NULL));

		animation_widget_ptr preview(new animation_widget(128, 128, object_template_));
		add_widget(preview, width() - border_offset - 128, border_offset + 200);
	} catch(validation_failure_exception& e) {
		error_text_ = e.msg;
		std::cerr << "error parsing formula: " << e.msg << std::endl;
	} catch(type_error& e) {
		error_text_ = e.message;
		std::cerr << "error executing formula: " << e.message << std::endl;
	}

	std::string err_text = error_text_;
	boost::replace_all(err_text, "\n", "\\n");
	int max_chars = (width() - border_offset*2)/font::char_width(14);
	if(err_text.length() > max_chars && max_chars > 3) {
		err_text = err_text.substr(0, max_chars-3) + "...";
	}
	label_ptr error_text(new label(err_text, graphics::color_red(), 14));
	add_widget(error_text, border_offset, height() - g->height() - border_offset - error_text->height() - 5);

	g.reset(new grid(3));
	g->set_hpad(20);
	g->add_col(button_ptr(new button(new label("Create", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_create, this))));
	g->add_col(button_ptr(new button(new label("Set Path...", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_set_path, this))));
	std::string path = current_object_save_path_;
	if(object_template_.has_key("id")) {
		path += object_template_["id"].as_string() + ".cfg";
	} else {
		path += "<no id>.cfg";
	}
	g->add_col(label_ptr(new label(path, graphics::color_green())));
	add_widget(g, border_offset, height() - g->height() - border_offset);

	container->set_max_height(height() - g->height() - border_offset - error_text->height() - 10);
	add_widget(container, border_offset, border_offset*2);
}

void custom_object_dialog::on_set_path()
{
	gui::file_chooser_dialog dir_dlg(
		int(preferences::virtual_screen_width()*0.2), 
		int(preferences::virtual_screen_height()*0.2), 
		int(preferences::virtual_screen_width()*0.6), 
		int(preferences::virtual_screen_height()*0.6),
		gui::filter_list(), 
		true, current_object_save_path_);
	dir_dlg.set_background_frame("empty_window");
	dir_dlg.set_draw_background_fn(do_draw_scene);
	dir_dlg.use_relative_paths(true);
	dir_dlg.show_modal();

	if(dir_dlg.cancelled() == false) {
		current_object_save_path_ = dir_dlg.get_path() + "/";
	}
	init();
}

void custom_object_dialog::id_change_focus(bool focus)
{
	if(focus == false) {
		init();
	}
}

std::vector<gui::widget_ptr> custom_object_dialog::get_widget_for_attribute(const std::string& attr)
{
	using namespace gui;
	if(attr == "id") {
		//grid_ptr g(new grid(2));
		//text_editor_widget_ptr change_entry(new text_editor_widget(200, 28));
		//change_entry->set_font_size(14);
		//if(object_template_.has_key(attr)) {
		//	change_entry->set_text(object_template_[attr].as_string());
		//}
		//change_entry->set_on_change_handler(boost::bind(&custom_object_dialog::change_text_attribute, this, change_entry, attr));
		//change_entry->set_on_enter_handler(do_nothing);
		//g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14))).add_col(widget_ptr(change_entry));
		//return g;
	} else if(attr == "hitpoints" || attr == "mass" || attr == "friction" 
		|| attr == "traction" || attr == "traction_in_air") {
		grid_ptr g(new grid(3));
		int value = 0;
		text_editor_widget_ptr change_entry(new text_editor_widget(100, 28));
		change_entry->set_font_size(14);
		if(object_template_.has_key(attr)) {
			std::stringstream ss;
			value = object_template_[attr].as_int();
			ss << object_template_[attr].as_int();
			change_entry->set_text(ss.str());
		} else {
			change_entry->set_text("0");
		}
		slider_offset_[attr] = object_template_.has_key(attr) ? object_template_[attr].as_int() : 0;

		slider_ptr slide(new slider(200, 
			boost::bind((&custom_object_dialog::change_int_attribute_slider), this, change_entry, attr, _1), 
			value));
		slide->set_position(0.5);
		slide->set_drag_end(boost::bind(&custom_object_dialog::slider_drag_end, this, change_entry, attr, slide, _1));
		change_entry->set_on_change_handler(boost::bind(&custom_object_dialog::change_int_attribute_text, this, change_entry, attr, slide));
		change_entry->set_on_enter_handler(do_nothing);
		label_ptr attr_label(new label(attr + ": ", graphics::color_white(), 14));
		attr_label->set_dim(200, attr_label->height());
		change_entry->set_dim(100, change_entry->height());
		slide->set_dim(200, slide->height());
		g->add_col(attr_label).add_col(widget_ptr(change_entry)).add_col(slide);

		g->set_col_width(0, 200);
		g->set_col_width(1, 100);
		g->set_col_width(2, 200);

		return std::vector<gui::widget_ptr>(1, g);
	} else if(attr == "animation") {
		//button_ptr bb(new button(new label("Edit Animations", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_animations, this)));
		//return bb;
	} else if(attr == "vars") {
		//grid_ptr g(new grid(1));
		//g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14)));
		//return std::vector<gui::widget_ptr>(1, g);
	} else if(attr == "editor_info") {
		//grid_ptr g(new grid(1));
		//g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14)));
		//return std::vector<gui::widget_ptr>(1, g);
	} else if(attr == "prototype") {
		//int count = 0;
		// To make this nicer. Create the buttons before adding them to the grid.
		// Estimate the maximum number of columns needed (take the minimum size button 
		// divided into screen width being used.  Then we start adding buttons to the 
		// grid, if we are about to add a button that would go over the maximum
		// width then we do a .finish_row() (if needed) and start continue
		// adding the column to the next row (with .add_col()).
		std::vector<button_ptr> buttons;
		int min_size_button = INT_MAX;
		if(object_template_.has_key("prototype")) {
			foreach(const std::string& s, object_template_["prototype"].as_list_string()) {
				buttons.push_back(new button(widget_ptr(new label(s, graphics::color_white())), 
					boost::bind(&custom_object_dialog::remove_prototype, this, s)));
				if(min_size_button > buttons.back()->width()) {
					min_size_button = buttons.back()->width();
				}
			}
		}
		std::vector<gui::widget_ptr> rows;
		// conservative
		int column_estimate = (width() - 100) / min_size_button + 2;
		grid_ptr g(new grid(column_estimate));
		label_ptr attr_label  = new label(attr + ": ", graphics::color_white(), 14);
		button_ptr add_button = new button(widget_ptr(new label("Add...", graphics::color_white())), 
			boost::bind(&custom_object_dialog::change_prototype, this));
		g->add_col(attr_label).add_col(add_button);

		int current_row_size = attr_label->width() + add_button->width();
		int buttons_on_current_row = 2;
		foreach(const button_ptr& b, buttons) {
			if(b->width() + current_row_size >= width()-100 ) {
				if(buttons_on_current_row < column_estimate) {
					g->finish_row();
				}
				rows.push_back(g);
				g.reset(new grid(column_estimate));
				
				current_row_size = 0;
				buttons_on_current_row = 0;
			}
			g->add_col(b);
			current_row_size += b->width();
			buttons_on_current_row++;
		}
		if(buttons_on_current_row != 0) {
			if(buttons_on_current_row < column_estimate) {
				g->finish_row();
			}
			rows.push_back(g);
		}
		return rows;
	}
	std::cerr << "Unhandled attribute " << attr << std::endl;
	return std::vector<gui::widget_ptr>();
}

void custom_object_dialog::slider_drag_end(const gui::text_editor_widget_ptr editor, const std::string& s, gui::slider_ptr slide, double d)
{
	int i = slider_transform(d) + slider_offset_[s];
	slider_offset_[s] = i;
	slide->set_position(0.5);
	dragging_slider_ = false;
}

void custom_object_dialog::change_int_attribute_slider(const gui::text_editor_widget_ptr editor, const std::string& s, double d)
{	
	dragging_slider_ = true;
	std::ostringstream ss;
	int i = slider_transform(d) + slider_offset_[s];
	ss << i;
	editor->set_text(ss.str(), false);
	object_template_.add_attr(variant(s), variant(i));
}

void custom_object_dialog::change_text_attribute(const gui::text_editor_widget_ptr editor, const std::string& s)
{
	object_template_.add_attr(variant(s), variant(editor->text()));
}

void custom_object_dialog::change_int_attribute_text(const gui::text_editor_widget_ptr editor, const std::string& s, gui::slider_ptr slide)
{
	if(!dragging_slider_) {
		int i;
		std::istringstream(editor->text()) >> i;
		slider_offset_[s] = i;
		slide->set_position(0.5);
		object_template_.add_attr(variant(s), variant(i));
	}
}

void custom_object_dialog::change_template(int selection, const std::string& s)
{
	selected_template_ = selection;
	if(selection == 0) {
		template_file_ = std::pair<std::string, std::string>();
	} else {
		template_file_.first = get_id_from_filemap(std::pair<std::string, std::string>(s,""));
		template_file_.second = get_dialog_file(s);
	}
	if(template_file_.first.empty() == false) {
		object_template_ = json::parse_from_file(template_file_.second);
		ASSERT_LOG(object_template_.is_map(), 
			"OBJECT TEMPLATE READ FROM FILE IS NOT MAP: " << template_file_.second)
		// ignorning these exceptions till we're finished
		//assert_recover_scope recover_from_assert;
		//try {
		//	object_ = custom_object_type_ptr(new custom_object_type(object_template_, NULL, NULL));
		//} catch(validation_failure_exception& e) {
		//	std::cerr << "error parsing formula: " << e.msg << std::endl;
		//} catch(type_error& e) {
		//	std::cerr << "error executing formula: " << e.message << std::endl;
		//}
	} else {
		std::map<variant, variant> m;
		object_template_ = variant(&m);
	}
	init();
}

void custom_object_dialog::change_prototype()
{
	using namespace gui;
	std::vector<std::string> choices;
	if(object_template_.has_key("prototype")) {
		std::vector<std::string> v = object_template_["prototype"].as_list_string();
		std::sort(v.begin(), v.end());
		std::set_difference(prototypes_.begin(), prototypes_.end(), v.begin(), v.end(), 
			std::inserter(choices, choices.end()));
	} else {
		choices = prototypes_;
	}

	int mousex, mousey, my;
	SDL_GetMouseState(&mousex, &my);
	mousex -= this->x();
	mousey = my - this->y();

	gui::grid* grid = new gui::grid(1);
	grid->set_max_height(height() - my);
	grid->set_hpad(10);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();
	foreach(const std::string& s, choices) {
		grid->add_col(widget_ptr(new label(s, graphics::color_white())));
	}
	grid->register_selection_callback(boost::bind(&custom_object_dialog::execute_change_prototype, this, choices, _1));

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void custom_object_dialog::remove_prototype(const std::string& s)
{
	if(object_template_.has_key("prototype")) {
		std::vector<variant> v = object_template_["prototype"].as_list();
		v.erase(std::remove(v.begin(), v.end(), variant(s)), v.end());
		object_template_.add_attr(variant("prototype"), variant(&v));
	}
	init();
}

void custom_object_dialog::execute_change_prototype(const std::vector<std::string>& choices, size_t index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}
	if(index < 0 || index >= choices.size()) {
		return;
	}

	std::vector<variant> v;
	if(object_template_.has_key("prototype")) {
		v = object_template_["prototype"].as_list();
	}
	v.push_back(variant(choices[index]));
	object_template_.add_attr(variant("prototype"), variant(&v));

	init();
}


void custom_object_dialog::on_create()
{
	// write out the file
	sys::write_file(current_object_save_path_ + "/" + object_template_[variant("id")].as_string() + ".cfg", object_template_.write_json());
	close();
}

void custom_object_dialog::on_edit_animations()
{
	gui::animation_creator_dialog d(0, 0, preferences::virtual_screen_width(), 
		preferences::virtual_screen_height(),
		object_template_.has_key("animation") ? object_template_["animation"] : variant());
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);

	d.show_modal();
	if(d.cancelled() == false) {
		object_template_.add_attr(variant("animation"), d.get_animations());
	}
}

void custom_object_dialog::on_edit_items(const std::string& name, const std::string& attr, bool allow_functions)
{
	gui::item_edit_dialog d(0, 0, preferences::virtual_screen_width(), 
		preferences::virtual_screen_height(),
		name,
		object_template_.has_key(attr) ? object_template_[attr] : variant());
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);
	d.allow_functions(allow_functions);

	d.show_modal();
	if(d.cancelled() == false) {
		object_template_.add_attr(variant(attr), d.get_items());
	}
}

}

namespace gui {

void item_edit_dialog::init()
{
	clear();

	const int border_offset = 35;
	const int hpad = 20;
	int current_height = border_offset;
	label_ptr title(new label(display_name_.empty() ? "Edit" : display_name_, graphics::color_white(), 20));
	add_widget(title, border_offset, current_height);
	current_height += title->height() + hpad;

	grid_ptr g(new grid(2));
	g->set_hpad(100);
	button_ptr mod_button(new button(new label("Save&Close", graphics::color_white(), 16), boost::bind(&item_edit_dialog::on_save, this)));
	button_ptr del_button(new button(new label("Cancel", graphics::color_white(), 16), boost::bind(&item_edit_dialog::cancel, this)));
	g->add_col(mod_button).add_col(del_button);
	add_widget(g, (width() - g->width())/2, current_height);
	current_height += g->height() + hpad;


	text_button_grid.reset(new grid(2));
	text_button_grid->set_hpad(30);
	save_text_button_.reset(new button(new label("Save Text", graphics::color_white(), 14), boost::bind(&item_edit_dialog::string_entry_save, this)));
	discard_text_button_.reset(new button(new label("Discard Text", graphics::color_white(), 14), boost::bind(&item_edit_dialog::string_entry_discard, this)));
	text_button_grid->add_col(save_text_button_).add_col(discard_text_button_);
	text_button_grid->set_visible(false);

	const int string_entry_height = height() - current_height - border_offset - text_button_grid->height() - 5;
	const int string_entry_width = 2*width()/3 - 2*border_offset;

	add_widget(text_button_grid, width()/3 + border_offset + (string_entry_width - text_button_grid->width())/2, string_entry_height + current_height + 5);

	string_entry_.reset(new code_editor_widget(string_entry_width, string_entry_height));
	string_entry_->set_font_size(12);
	string_entry_->set_on_esc_handler(boost::bind(&item_edit_dialog::string_entry_discard, this));
	string_entry_->set_loc(width()/3 + border_offset, current_height);
	if(allow_functions_) {
		string_entry_->set_formula();
	}

	item_grid_.reset(new tree_editor_widget(width()/3 - border_offset, height() - current_height - border_offset, items_));
	item_grid_->allow_selection();
	item_grid_->allow_persistent_highlight();
	item_grid_->set_editor_handler(variant::TYPE_STRING, string_entry_, boost::bind(&item_edit_dialog::editor_select, this, _1, _2));
	add_widget(item_grid_, border_offset, current_height);

	current_height += item_grid_->height() + hpad;
}

void item_edit_dialog::editor_select(variant* v, boost::function<void(const variant&)> save_fn)
{
	text_button_grid->set_visible(true);
	saved_text_ = v->as_string();
	string_entry_->set_text(saved_text_);
	string_entry_->set_focus(true);
	save_fn_ = save_fn;
}

void item_edit_dialog::on_save()
{
	close();
}

bool item_edit_dialog::has_keyboard_focus()
{
	return string_entry_->has_focus();
}

bool item_edit_dialog::handle_event(const SDL_Event& event, bool claimed)
{
	if(dialog::handle_event(event, claimed)) {
		return true;
	}

	if(has_keyboard_focus()) {
		if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.sym == SDLK_s && (event.key.keysym.mod&KMOD_CTRL)) {
				string_entry_save();
				return true;
			}
		}
	}
	return claimed;
}

void item_edit_dialog::string_entry_save()
{
	if(save_fn_) {
		text_button_grid->set_visible(false);
		save_fn_(variant(string_entry_->text()));
	}
}

void item_edit_dialog::string_entry_discard()
{
	if(save_fn_) {
		text_button_grid->set_visible(false);
		save_fn_(variant(saved_text_));
	}
}

}

#endif // !NO_EDITOR
