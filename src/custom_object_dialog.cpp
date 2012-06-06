#ifndef NO_EDITOR

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <climits>

#include <math.h>

#include "animation_creator.hpp"
#include "asserts.hpp"
#include "button.hpp"
#include "code_editor_widget.hpp"
#include "custom_object_dialog.hpp"
#include "draw_scene.hpp"
#include "dropdown_widget.hpp"
#include "file_chooser_dialog.hpp"
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
		slider_offset_(0), dragging_slider_(false), row_count_(0), is_new_attribute_(true)
	{
		if(items_.is_map() == false) {
			std::map<variant, variant> m;
			items_ = variant(&m);
		}
		selected_attribute_ = boost::shared_ptr<variant>(new variant());
		init();
	}
	virtual ~item_edit_dialog() 
	{}
	variant get_items() { return items_; }
	void allow_functions(bool val=true) { allow_functions_ = val; }
protected:
	void init();
	void on_add();
	void on_add_execute(const std::vector<std::string>& choices, int index);
	void on_delete();
	void on_save();
	void change_text_attribute(const text_editor_widget_ptr editor, const std::string& s);

	void on_slider_change(text_editor_widget_ptr editor, double d);
	void on_slider_end(slider_ptr numeric_slider, text_editor_widget_ptr editor, double d);
	void on_number_change(text_editor_widget_ptr editor, slider_ptr numeric_slider);

	void on_bool_change(int selection, const std::string& s);
	void on_attribute_type_change(int selection, const std::string& s);

	void on_itemgrid_select(int selection, std::pair<std::string, variant*> p);
	std::vector<std::string> get_type_list();
	variant get_new_variant(const std::string& s);
private:
	std::string display_name_;
	variant items_;
	bool allow_functions_;
	widget_ptr context_menu_;

	std::string selected_attribute_name_;
	boost::shared_ptr<variant> selected_attribute_;

	std::string error_text_;

	tree_view_widget_ptr item_grid_;
	int row_count_;

	int slider_offset_;
	bool dragging_slider_;
	
	bool is_new_attribute_;
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
	// cache prototypes.
	//std::transform(prototype_file_paths().begin(), prototype_file_paths().end(), std::back_inserter(prototypes_),
	//	boost::bind(&std::map<std::string, std::string>::value_type::first,_1));
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
	container->set_max_height(height() - 50);

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
	change_entry->set_on_enter_handler(do_nothing);
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

	/*{
		grid_ptr gg(new grid(2));
		text_editor_widget_ptr file_text(new text_editor_widget(400, 28));
		file_text->set_font_size(14);
		button_ptr bb(new button(new label("Choose File...", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_choose_file, this, file_text)));
		bb->set_dim(75, 20);
		gg->add_col(bb);
		gg->add_col(file_text);

		text_editor_widget_ptr output_dir_text(new text_editor_widget(400, 28));
		output_dir_text->set_font_size(14);
		output_dir_text->set_text(module::get_module_path("") + "images/");
		bb.reset(new button(new label("CopyDir", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_choose_dest_dir, this, output_dir_text)));
		bb->set_dim(75, 20);
		gg->add_col(bb);
		gg->add_col(output_dir_text);
		container->add_col(gg);
		//std::cerr << "module path: \"" << module::get_module_path("") << "images/\"" << std::endl;
	}*/
	
	button_ptr b(new button(new label("Create", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_create, this)));
	container->add_col(b);
	add_widget(container, border_offset, border_offset*2);
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
	//ASSERT_LOG(false, "Unhandled attribute " << attr);
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

/*void custom_object_dialog::change_template()
{
	using namespace gui;
	dialog d(int(preferences::virtual_screen_width()*0.3), 
		int(preferences::virtual_screen_height()*0.3), 
		int(preferences::virtual_screen_width()*0.4), 
		int(preferences::virtual_screen_height()*0.4));
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);
	d.set_padding(20);
	
	grid_ptr container(new gui::grid(1));
	container->add_col(widget_ptr(new label("Choose Template File", graphics::color_white(), 36)));

	grid* grid = new gui::grid(1);
	grid->set_hpad(40);
	grid->set_max_height(int(preferences::virtual_screen_height()*0.4 - 50));
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();
	std::vector<std::string> choices;
	std::transform(get_template_path().begin(), get_template_path().end(), std::back_inserter(choices),
		boost::bind(&module::module_file_map::value_type::first,_1));
	std::sort(choices.begin(), choices.end());
	choices.insert(choices.begin(), "Blank");
	foreach(const std::string& s, choices) {
		grid->add_col(widget_ptr(new label(s, graphics::color_white())));
	}
	grid->register_selection_callback(boost::bind(&custom_object_dialog::execute_change_template, this, &d, choices, _1));
	container->add_col(grid);
	d.add_widget(container, 35, 35);

	d.show_modal();
}

void custom_object_dialog::execute_change_template(dialog* d, const std::vector<std::string>& choices, size_t index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}
	if(index < 0 || index >= choices.size()) {
		return;
	}
	if(index > 0) {
		template_file_.first = get_id_from_filemap(std::pair<std::string, std::string>(choices[index],""));
		template_file_.second = get_dialog_file(choices[index]);
	} else {
		template_file_ = std::pair<std::string, std::string>();
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
	}

	d->close();
	init();
}*/

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
	// XXX write out the file
	//std::string path;
	//if(path.length() < 4 || path.substr(path.length()-4) != ".cfg") {
	//	path += ".cfg";
	//}
	//module::write_file(module::get_module_name(), path, object_template_.write_json());
	close();
}

void custom_object_dialog::on_choose_file(const gui::text_editor_widget_ptr editor)
{
	gui::filter_list f;
	f.push_back(gui::filter_pair("Image Files", ".*?\\.(png|jpg|gif|bmp)"));
	f.push_back(gui::filter_pair("All Files", ".*"));
	gui::file_chooser_dialog open_dlg(
		int(preferences::virtual_screen_width()*0.1), 
		int(preferences::virtual_screen_height()*0.1), 
		int(preferences::virtual_screen_width()*0.8), 
		int(preferences::virtual_screen_height()*0.8),
		f);
	open_dlg.set_background_frame("empty_window");
	open_dlg.set_draw_background_fn(do_draw_scene);
	open_dlg.show_modal();

	if(open_dlg.cancelled() == false) {
		editor->set_text(open_dlg.get_file_name());
	}
}

void custom_object_dialog::on_choose_dest_dir(const gui::text_editor_widget_ptr editor)
{
	gui::file_chooser_dialog dir_dlg(
		int(preferences::virtual_screen_width()*0.2), 
		int(preferences::virtual_screen_height()*0.2), 
		int(preferences::virtual_screen_width()*0.6), 
		int(preferences::virtual_screen_height()*0.6),
		gui::filter_list(), 
		true, module::get_module_path("") + "images/");
	dir_dlg.set_background_frame("empty_window");
	dir_dlg.set_draw_background_fn(do_draw_scene);
	dir_dlg.use_relative_paths(true);
	dir_dlg.show_modal();

	if(dir_dlg.cancelled() == false) {
		editor->set_text(dir_dlg.get_path());
	}
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

	grid_ptr g(new grid(3));
	g->set_hpad(30);
	button_ptr add_button(new button(new label("Add", graphics::color_white(), 16), boost::bind(&item_edit_dialog::on_add, this)));
	button_ptr mod_button(new button(new label("Save", graphics::color_white(), 16), boost::bind(&item_edit_dialog::on_save, this)));
	button_ptr del_button(new button(new label("Delete", graphics::color_white(), 16), boost::bind(&item_edit_dialog::on_delete, this)));
	g->add_col(add_button).add_col(mod_button).add_col(del_button);
	add_widget(g, border_offset, current_height);
	current_height += g->height() + hpad;

	g.reset(new grid(5));
	g->set_hpad(30);
	g->set_zorder(1);
	g->add_col(label_ptr(new label("Attribute Name", graphics::color_white(), 16)));
	text_editor_widget_ptr name_entry(new text_editor_widget(200, 28));
	name_entry->set_font_size(14);
	name_entry->set_text(selected_attribute_name_);
	name_entry->set_on_change_handler(boost::bind(&item_edit_dialog::change_text_attribute, this, name_entry, "attribute_name"));
	g->add_col(name_entry);
	dropdown_widget_ptr dropdown_type(new dropdown_widget(get_type_list(), 120, 24));
	dropdown_type->set_on_select_handler(boost::bind(&item_edit_dialog::on_attribute_type_change, this, _1, _2));
	if(is_new_attribute_) {
		if(selected_attribute_->is_null()) {
			dropdown_type->set_selection(0);
		} else if(selected_attribute_->is_bool()) {
			dropdown_type->set_selection(1);
		} else if(selected_attribute_->is_int()) {
			dropdown_type->set_selection(2);
		} else if(selected_attribute_->is_decimal()) {
			dropdown_type->set_selection(3);
		} else if(selected_attribute_->is_list()) {
			dropdown_type->set_selection(4);
		} else if(selected_attribute_->is_map()) {
			dropdown_type->set_selection(5);
		} else if(selected_attribute_->is_string()) {
			dropdown_type->set_selection(6);
		}
		dropdown_type->set_dropdown_height(200);
		g->add_col(dropdown_type);
	} else {
		std::string s = "<Unknown>";
		if(selected_attribute_->is_null()) {
			s = get_type_list()[0];
		} else if(selected_attribute_->is_bool()) {
			s = get_type_list()[1];
		} else if(selected_attribute_->is_int()) {
			s = get_type_list()[2];
		} else if(selected_attribute_->is_decimal()) {
			s = get_type_list()[3];
		} else if(selected_attribute_->is_list()) {
			s = get_type_list()[4];
		} else if(selected_attribute_->is_map()) {
			s = get_type_list()[5];
		} else if(selected_attribute_->is_string()) {
			s = get_type_list()[6];
		}
		g->add_col(label_ptr(new label(s, graphics::color_yellow(), 14)));
	}
	if(selected_attribute_->is_numeric()) {
		text_editor_widget_ptr numeric_entry(new text_editor_widget(200, 28));
		slider_ptr numeric_slider(new slider(200, boost::bind(&item_edit_dialog::on_slider_change, this, numeric_entry, _1), 0.5));
		numeric_slider->set_drag_end(boost::bind(&item_edit_dialog::on_slider_end, this, numeric_slider, numeric_entry, _1));
		numeric_entry->set_font_size(14);
		std::stringstream ss;
		if(selected_attribute_->is_decimal()) {
			ss << selected_attribute_->as_decimal();
		} else {
			ss << selected_attribute_->as_int();
		}
		numeric_entry->set_text(ss.str());
		numeric_entry->set_on_change_handler(boost::bind(&item_edit_dialog::on_number_change, this, numeric_entry, numeric_slider));
		g->add_col(numeric_entry).add_col(numeric_slider);
	} else if(selected_attribute_->is_bool()) {
		std::vector<std::string> bool_list;
		bool_list.push_back("false");
		bool_list.push_back("true");
		dropdown_widget_ptr bool_dd(new dropdown_widget(bool_list, 100, 30));
		bool_dd->set_selection(selected_attribute_->as_bool());
		bool_dd->set_on_select_handler(boost::bind(&item_edit_dialog::on_bool_change, this, _1, _2));
		g->add_col(bool_dd);
	}
	g->finish_row();
	add_widget(g, border_offset, current_height);
	current_height += g->height() + hpad;

	// Always have a place for the error text.
	label_ptr error_label(new label(error_text_, graphics::color_red(), 16));
	add_widget(error_label, border_offset, current_height);
	current_height += g->height() + hpad;
	error_text_.clear();
	
	if(selected_attribute_->is_string()) {
		code_editor_widget_ptr string_entry(new code_editor_widget(width() - 2*border_offset, height()/2 - border_offset));
		string_entry->set_font_size(12);
		string_entry->set_text(selected_attribute_->as_string());
		//string_entry->set_on_change_handler(boost::bind(&item_edit_dialog::on_change_string, this, string_entry));
		add_widget(string_entry, border_offset, current_height);
		current_height += string_entry->height() + hpad;
	}

	if(item_grid_) {
		item_grid_.reset(new tree_view_widget(width() - 2*border_offset, height()/3 - border_offset, items_));
	} else {
		item_grid_ = new tree_view_widget(width() - 2*border_offset, height()/3 - border_offset, items_);
	}
	//item_grid_->set_show_background(true);
	item_grid_->allow_selection();
	item_grid_->register_selection_callback(boost::bind(&item_edit_dialog::on_itemgrid_select, this, _1, _2));
	/*item_grid_.reset(new grid(2));
	item_grid_->set_dim(width() - 2*border_offset, height()/3 - border_offset);
	item_grid_->set_max_height(height()/3);
	item_grid_->set_show_background(true);
	item_grid_->allow_selection();
	item_grid_->register_selection_callback(boost::bind(&item_edit_dialog::on_itemgrid_select, this, _1));
	//item_grid_->highlight_selection(true, graphics::color_blue);
	row_count_ = 0;
	foreach(const variant& key, items_.get_keys().as_list()) {
		std::vector<std::vector<widget_ptr> > widget_list = get_widget(key);
		foreach(const std::vector<widget_ptr>& w, widget_list) {
			row_count_++;
			item_grid_->add_row(w);
		}
	}*/
	add_widget(item_grid_, border_offset, 2*height()/3 - border_offset);
	current_height += item_grid_->height() + hpad;
}

std::vector<std::string> item_edit_dialog::get_type_list()
{
	std::vector<std::string> choices;
	choices.push_back("Null");
	choices.push_back("Boolean");
	choices.push_back("Integer");
	choices.push_back("Decimal");
	choices.push_back("List");
	choices.push_back("Map");
	choices.push_back("String");
	if(allow_functions_) {
		choices.push_back("Function");
	}
	return choices;
}

void item_edit_dialog::on_add()
{
	gui::grid* grid = new gui::grid(1);
	grid->set_hpad(40);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();

	std::vector<std::string> choices = get_type_list();
	foreach(const std::string& choice, choices) {
		grid->add_col(widget_ptr(new label(choice, graphics::color_white())));
	}
	grid->register_selection_callback(boost::bind(&item_edit_dialog::on_add_execute, this, choices, _1));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	context_menu_->set_zorder(2);
	add_widget(context_menu_, mousex, mousey);
}

void item_edit_dialog::on_add_execute(const std::vector<std::string>& choices, int index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}

	if(index < 0 || size_t(index) >= choices.size()) {
		return;
	}

	selected_attribute_name_.clear();

	selected_attribute_.reset(new variant(get_new_variant(choices[index])));

	is_new_attribute_ = true;

	init();
}

void item_edit_dialog::on_delete()
{
}

void item_edit_dialog::on_save()
{
	if(selected_attribute_name_.empty()) {
		error_text_ = "You must specify a unqiue attribute name";
//	} else if(items_.has_key(selected_attribute_name_) && items_[selected_attribute_name_].type() != selected_attribute_.type()) {
//		error_text_ = "Can only modify attributes of the same type. Please delete the old attribute and re-create it with a new type.";
	} else {
		//items_.add_attr(variant(selected_attribute_name_), *selected_attribute_);
		selected_attribute_name_.clear();
		selected_attribute_.reset(new variant());
		slider_offset_ = 0;
		is_new_attribute_ = true;
	}
	init();
}

void item_edit_dialog::change_text_attribute(const gui::text_editor_widget_ptr editor, const std::string& s)
{
	if(s == "attribute_name") {
		selected_attribute_name_ = editor->text();
	}
}

void item_edit_dialog::on_slider_change(text_editor_widget_ptr editor, double d)
{
	dragging_slider_ = true;
	std::ostringstream ss;
	int i = slider_transform(d) + slider_offset_;
	ss << i;
	editor->set_text(ss.str(), false);
	selected_attribute_.reset(new variant(i));
}

void item_edit_dialog::on_slider_end(slider_ptr numeric_slider, text_editor_widget_ptr editor, double d)
{
	int i = slider_transform(d) + slider_offset_;
	slider_offset_ = i;
	numeric_slider->set_position(0.5);
	dragging_slider_ = false;
	init();
}

void item_edit_dialog::on_number_change(text_editor_widget_ptr editor, slider_ptr numeric_slider)
{
	if(!dragging_slider_) {
		int i;
		std::istringstream(editor->text()) >> i;
		slider_offset_ = i;
		numeric_slider->set_position(0.5);
		selected_attribute_.reset(new variant(i));
	}
}

void item_edit_dialog::on_bool_change(int selection, const std::string& s)
{
	if(selection < 0 || size_t(selection) > 1) {
		return;
	}
	selected_attribute_.reset(new variant(variant::from_bool(bool(selection))));
	init();
}

void item_edit_dialog::on_itemgrid_select(int selection, std::pair<std::string, variant*> p)
{
	if(selection < 0 || size_t(selection) >= item_grid_->nrows()) {
		return;
	}
	// XXX Need to come up with an internal representation of items in the grid which handles maps and lists etc.
	// for the purposes of selecting them.
	//selected_attribute_ = p.second;
	//selected_attribute_name_ = p.first;
	is_new_attribute_ = false;
	init();
}

void item_edit_dialog::on_attribute_type_change(int selection, const std::string& s)
{
	if(selection < 0 || size_t(selection) > get_type_list().size()) {
		return;
	}
	selected_attribute_.reset(new variant(get_new_variant(s)));
	init();
}

variant item_edit_dialog::get_new_variant(const std::string& s)
{
	if(s == "Boolean") {
		return variant::from_bool(false);
	} else if(s == "Decimal") {
		return variant(0.0);
	} else if(s == "Integer") {
		return variant(0);
	} else if(s == "List") {
		std::vector<variant> v;
		return variant(&v);
	} else if(s == "Map") {
		std::map<variant, variant> m;
		return variant(&m);
	} else if(s == "String") {
		return variant("");
	} else if(s == "Function") {
		return variant("");
	}
	return variant();
}

}

#endif // !NO_EDITOR
