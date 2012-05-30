#ifndef NO_EDITOR

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>

#include <math.h>

#include "animation_creator.hpp"
#include "asserts.hpp"
#include "button.hpp"
#include "custom_object_dialog.hpp"
#include "draw_scene.hpp"
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

#define TEMPLATE_DIRECTORY	"data/object_templates/"

namespace editor_dialogs {

namespace {

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
	d.push_back("id");
	d.push_back("animation");
	d.push_back("hitpoints");
	d.push_back("mass");
	d.push_back("vars");
	d.push_back("friction");
	d.push_back("traction");
	d.push_back("traction_in_air");
	d.push_back("editor_info");
	d.push_back("prototype");
}

std::vector<std::string>& get_default_attribute_list()
{
	static std::vector<std::string> defaults;
	if(defaults.empty()) {
		load_default_attributes(defaults);
	}
	return defaults;
}

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

custom_object_dialog::custom_object_dialog(editor& e, int x, int y, int w, int h)
	: gui::dialog(x,y,w,h), dragging_slider_(false)
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
	using namespace gui;
	clear();

	add_widget(widget_ptr(new label("Object Properties", graphics::color_white(), 20)), 10, 10);

	grid_ptr container(new grid(1));
	container->set_col_width(0, width() - 30);

	grid_ptr g(new grid(2));
	g->set_hpad(20);
	g->add_col(widget_ptr(new label("Template  ", graphics::color_white(), 14)))
		.add_col(widget_ptr(new button(widget_ptr(new label(template_file_.first.empty() ? "Blank" : template_file_.first, graphics::color_white())), 
		boost::bind(&custom_object_dialog::change_template, this))));
	container->add_col(g);
	container->set_max_height(height() - 50);

	if(template_file_.first.empty()) {
		foreach(const std::string& attr, get_default_attribute_list()) {
			widget_ptr w = get_widget_for_attribute(attr);
			if(w) {
				container->add_col(w);
			}
		}
	} else {
		std::vector<variant> keys = object_template_.get_keys().as_list();
		foreach(const variant& v, keys) {
			widget_ptr w = get_widget_for_attribute(v.as_string());
			if(w) {
				container->add_col(w);
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
	add_widget(container, 30, 30);
}

gui::widget_ptr custom_object_dialog::get_widget_for_attribute(const std::string& attr)
{
	using namespace gui;
	if(attr == "id") {
		grid_ptr g(new grid(2));
		text_editor_widget_ptr change_entry(new text_editor_widget(200, 28));
		change_entry->set_font_size(14);
		if(object_template_.has_key(attr)) {
			change_entry->set_text(object_template_[attr].as_string());
		}
		change_entry->set_on_change_handler(boost::bind(&custom_object_dialog::change_text_attribute, this, change_entry, attr));
		change_entry->set_on_enter_handler(do_nothing);
		g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14))).add_col(widget_ptr(change_entry));
		return g;
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
		slider_ptr slide(new slider(200, 
			boost::bind((&custom_object_dialog::change_int_attribute_slider), this, change_entry, attr, _1), 
			value));
		slide->set_position(0.5);
		slide->set_drag_end(boost::bind(&custom_object_dialog::slider_drag_end, this, change_entry, attr, slide, _1));
		change_entry->set_on_change_handler(boost::bind(&custom_object_dialog::change_int_attribute_text, this, change_entry, attr, slide));
		change_entry->set_on_enter_handler(do_nothing);
		g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14))).add_col(widget_ptr(change_entry));
		g->add_col(widget_ptr(slide));
		slider_offset_[attr] = object_template_.has_key(attr) ? object_template_[attr].as_int() : 0;
		return g;
	} else if(attr == "animation") {
		button_ptr bb(new button(new label("Edit Animations", graphics::color_white(), 20), boost::bind(&custom_object_dialog::on_edit_animations, this)));
		return bb;
	} else if(attr == "vars") {
		grid_ptr g(new grid(1));
		g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14)));
		return g;
	} else if(attr == "editor_info") {
		grid_ptr g(new grid(1));
		g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14)));
		return g;
	} else if(attr == "prototype") {
		int count = 0;
		std::vector<std::string> protos;
		if(object_template_.has_key("prototype")) {
			protos = object_template_["prototype"].as_list_string();
			count = protos.size();
		}
		grid_ptr g(new grid(2 + count));
		g->add_col(widget_ptr(new label(attr + ": ", graphics::color_white(), 14)));
		g->add_col(widget_ptr(new button(widget_ptr(new label("Add...", graphics::color_white())), 
			boost::bind(&custom_object_dialog::change_prototype, this))));
		foreach(const std::string& s, protos) {
			g->add_col(widget_ptr(new button(widget_ptr(new label(s, graphics::color_white())), 
			boost::bind(&custom_object_dialog::remove_prototype, this, s))));
		}
		return g;
	}
	//ASSERT_LOG(false, "Unhandled attribute " << attr);
	std::cerr << "Unhandled attribute " << attr << std::endl;
	return widget_ptr();
}

int custom_object_dialog::slider_transform(double d)
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
	editor->set_text(ss.str());
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

void custom_object_dialog::change_template()
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
		/*assert_recover_scope recover_from_assert;
		try {
			object_ = custom_object_type_ptr(new custom_object_type(object_template_, NULL, NULL));
		} catch(validation_failure_exception& e) {
			std::cerr << "error parsing formula: " << e.msg << std::endl;
		} catch(type_error& e) {
			std::cerr << "error executing formula: " << e.message << std::endl;
		}*/
	}

	d->close();
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
	gui::animation_creator_dialog d(int(preferences::virtual_screen_width()*0.1), 
		int(preferences::virtual_screen_height()*0.1), 
		int(preferences::virtual_screen_width()*0.8), 
		int(preferences::virtual_screen_height()*0.8),
		object_template_.has_key("animation") ? object_template_["animation"] : variant());
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);

	d.show_modal();
	if(d.cancelled() == false) {

	}
}

}

#endif // !NO_EDITOR

