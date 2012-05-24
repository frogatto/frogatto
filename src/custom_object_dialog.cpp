#ifndef NO_EDITOR

#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>

#include "asserts.hpp"
#include "button.hpp"
#include "custom_object_dialog.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "graphics.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "module.hpp"

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

custom_object_dialog::custom_object_dialog(editor& e, int x, int y, int w, int h)
	: gui::dialog(x,y,w,h)
{
	load_template_file_paths(TEMPLATE_DIRECTORY);
	set_clear_bg_amount(255);
	init();
}


void custom_object_dialog::init()
{
	if(template_file_.empty() == false) {
		object_template_ = json::parse_from_file(template_file_);
		ASSERT_LOG(object_template_.is_map(), 
			"OBJECT TEMPLATE READ FROM FILE IS NOT MAP: " << template_file_)
		// ignorning these exceptions till we're finished
		assert_recover_scope recover_from_assert;
		try {
			object_ = custom_object_type_ptr(new custom_object_type(object_template_, NULL, NULL));
		} catch(validation_failure_exception& e) {
			std::cerr << "error parsing formula: " << e.msg << std::endl;
		} catch(type_error& e) {
			std::cerr << "error executing formula: " << e.message << std::endl;
		}

	}

	using namespace gui;
	clear();

	add_widget(widget_ptr(new label("Object Properties", graphics::color_white(), 20)), 10, 10);

	grid_ptr g(new grid(2));
	g->set_hpad(20);
	g->add_col(widget_ptr(new label("Modules  ", graphics::color_white(), 14)))
		.add_col(widget_ptr(new button(widget_ptr(new label("Template", graphics::color_white())), boost::bind(&custom_object_dialog::change_template, this))));
	add_widget(g);

}

void custom_object_dialog::change_template()
{
	using namespace gui;
	dialog d(int(graphics::screen_width()*0.3), 
		int(graphics::screen_height()*0.3), 
		int(graphics::screen_width()*0.4), 
		int(graphics::screen_height()*0.4));
	d.set_background_frame("empty_window"); // regular_window
	d.add_widget(widget_ptr(new label("Choose Template File", graphics::color_white(), 48)));
	
	gui::grid* grid = new gui::grid(1);
	grid->set_hpad(40);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();
	std::vector<std::string> choices;
	std::transform(get_template_path().begin(), get_template_path().end(), std::back_inserter(choices),
		boost::bind(&module::module_file_map::value_type::first,_1));
	std::sort(choices.begin(), choices.end());
	foreach(const std::string& s, choices) {
		grid->add_col(widget_ptr(new label(s, graphics::color_white())));
	}
	grid->register_selection_callback(boost::bind(&custom_object_dialog::execute_change_template, this, choices, _1));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void custom_object_dialog::execute_change_template(const std::vector<std::string>& choices, size_t index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}
	if(index < 0 || index >= choices.size()) {
		return;
	}
	template_file_ = get_dialog_file(choices[index]);
	init();
}

}

#endif // !NO_EDITOR

