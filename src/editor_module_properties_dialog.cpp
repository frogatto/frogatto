#ifndef NO_EDITOR
#include "graphics.hpp"

#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>

#include "background.hpp"
#include "button.hpp"
#include "checkbox.hpp"
#include "editor.hpp"
#include "editor_dialogs.hpp"
#include "editor_module_properties_dialog.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "json_parser.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "module.hpp"
#include "raster.hpp"
#include "stats.hpp"
#include "text_editor_widget.hpp"
#include "text_entry_widget.hpp"

namespace editor_dialogs
{

editor_module_properties_dialog::editor_module_properties_dialog(editor& e)
  : dialog(0, 0, graphics::screen_width(), graphics::screen_height()), editor_(e), new_mod_(true)
{
	set_clear_bg_amount(255);
	init();
}

editor_module_properties_dialog::editor_module_properties_dialog(editor& e, const std::string& modname)
	: dialog(0, 0, graphics::screen_width(), graphics::screen_height()), editor_(e), new_mod_(false)
{
	if(!modname.empty()) {
		module::load_module_from_file(modname, &mod_);
		std::cerr << "MOD: " << modname << ":" << mod_.name_ << std::endl;
	}
	set_clear_bg_amount(255);
	init();
}

void editor_module_properties_dialog::init()
{
	dirs_.clear();
	module::get_module_list(dirs_);

	using namespace gui;
	clear();

	add_widget(widget_ptr(new label("Module Properties", graphics::color_white(), 48)), 10, 10);

	grid_ptr g(new grid(2));
	if(new_mod_) {
		g->add_col(widget_ptr(new label(mod_.name_, graphics::color_white(), 36)))
		  .add_col(widget_ptr(new button(widget_ptr(new label("Change Identifier", graphics::color_white())), boost::bind(&editor_module_properties_dialog::change_id, this))));
		add_widget(g);
	} else {
		g->add_col(widget_ptr(new label("Identifier:  ", graphics::color_white(), 36)))
			.add_col(widget_ptr(new label(mod_.name_, graphics::color_white(), 36)));
		add_widget(g);
	}

	text_editor_widget* change_name_entry(new text_editor_widget(80));
	change_name_entry->set_text(mod_.pretty_name_);
	change_name_entry->set_on_change_handler(boost::bind(&editor_module_properties_dialog::change_name, this, change_name_entry));
	change_name_entry->set_on_enter_handler(boost::bind(&dialog::close, this));

	g.reset(new grid(2));
	g->add_col(widget_ptr(new label("Name:", graphics::color_white(), 36)))
	  .add_col(widget_ptr(change_name_entry));
	add_widget(g);

	text_editor_widget* change_abbrev_entry(new text_editor_widget(80));
	change_abbrev_entry->set_text(mod_.abbreviation_);
	change_abbrev_entry->set_on_change_handler(boost::bind(&editor_module_properties_dialog::change_prefix, this, change_abbrev_entry));
	change_abbrev_entry->set_on_enter_handler(boost::bind(&dialog::close, this));

	g.reset(new grid(2));
	g->add_col(widget_ptr(new label("Prefix:", graphics::color_white(), 36)))
	  .add_col(widget_ptr(change_abbrev_entry));
	add_widget(g);

	g.reset(new grid(2));
	g->add_col(widget_ptr(new label("Modules  ", graphics::color_white(), 36)))
		.add_col(widget_ptr(new button(widget_ptr(new label("Add", graphics::color_white())), boost::bind(&editor_module_properties_dialog::change_module_includes, this))));
	add_widget(g);
	foreach(const std::string& s, mod_.included_modules_) {
		g.reset(new grid(2));
		g->add_col(widget_ptr(new label(s, graphics::color_white(), 36)))
			.add_col(widget_ptr(new button(widget_ptr(new label("Remove", graphics::color_white())), boost::bind(&editor_module_properties_dialog::remove_module_include, this, s))));
		add_widget(g);
	}
}

void editor_module_properties_dialog::change_id()
{
	using namespace gui;
	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(new label("Change Identifier", graphics::color_white(), 48)));
	text_entry_widget* entry = new text_entry_widget;
	if(!mod_.name_.empty()) {
		entry->set_text(mod_.name_);
	}
	d.add_widget(widget_ptr(new label("Identifier:", graphics::color_white())))
	 .add_widget(widget_ptr(entry));
	d.show_modal();

	if(d.cancelled()) {
		return;
	}

	if(std::find(dirs_.begin(), dirs_.end(), entry->text()) == dirs_.end()) {
		mod_.name_ = entry->text();
		init();
	}
}

void editor_module_properties_dialog::change_name(const gui::text_editor_widget* editor)
{
	mod_.pretty_name_ = editor->text();
}

void editor_module_properties_dialog::change_prefix(const gui::text_editor_widget* editor)
{
	mod_.abbreviation_ = editor->text();
}

void editor_module_properties_dialog::change_module_includes()
{
	using namespace gui;
	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(new label("Change Included Modules", graphics::color_white(), 48)));
	if(dirs_.empty()) {
		return;
	}
	std::sort(dirs_.begin(), dirs_.end());

	gui::grid* grid = new gui::grid(1);
	grid->set_hpad(40);
	grid->set_show_background(true);
	grid->allow_selection();
	grid->swallow_clicks();
	std::vector<std::string> choices;
	foreach(const std::string& dir, dirs_) {
		// only include modules not included already.
		if(std::find(mod_.included_modules_.begin(), mod_.included_modules_.end(), dir) == mod_.included_modules_.end() 
			&& dir != mod_.name_) {
			grid->add_col(widget_ptr(new label(dir, graphics::color_white())));
			choices.push_back(dir);
		}
	}
	grid->register_selection_callback(boost::bind(&editor_module_properties_dialog::execute_change_module_includes, this, choices, _1));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void editor_module_properties_dialog::remove_module_include(const std::string& s)
{
	std::vector<std::string>::iterator it = std::find(mod_.included_modules_.begin(), mod_.included_modules_.end(), s);
	if(it != mod_.included_modules_.end()) {
		mod_.included_modules_.erase(it);
	}
	init();
}

void editor_module_properties_dialog::execute_change_module_includes(const std::vector<std::string>& choices, int index)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}

	if(index < 0 || index >= choices.size()) {
		return;
	}

	mod_.included_modules_.push_back(choices[index]);

	init();
}

const std::string editor_module_properties_dialog::on_exit() {
	save_module_properties();
	if(new_mod_) {
		create_new_module();
	} 
	// Switch to the new_module
	module::reload(mod_.name_);
	// Reload level paths
	loadlevel::reload_level_paths();
	customobjecttype::reload_file_paths();
	if(mod_.abbreviation_.empty() == false) {
		return mod_.abbreviation_ + ":titlescreen.cfg";
	}
	return mod_.name_ + ":titlescreen.cfg";
}

void editor_module_properties_dialog::create_new_module() {
	if(!mod_.name_.empty()) {
		std::string mod_path = "./modules/" + mod_.name_ + "/";
		// create some default directories.
		sys::get_dir(mod_path + "data");
		sys::get_dir(mod_path + "data/level");
		sys::get_dir(mod_path + "data/objects");
		sys::get_dir(mod_path + "data/object_prototypes");
		sys::get_dir(mod_path + "gui");
		sys::get_dir(mod_path + "images");
		sys::get_dir(mod_path + "sounds");
		sys::get_dir(mod_path + "music");
		// Create an empty titlescreen.cfg
		variant empty_lvl = json::parse_from_file("data/level/empty.cfg");
		empty_lvl.add_attr(variant("id"), variant("titlescreen.cfg"));
		sys::write_file(mod_path + preferences::level_path() + "titlescreen.cfg", empty_lvl.write_json());
	}
}

void editor_module_properties_dialog::save_module_properties() {
	if(!mod_.name_.empty()) {
		std::map<variant,variant> m;
		m[variant("id")] = variant(mod_.name_);
		if(mod_.pretty_name_.empty() == false) {
			m[variant("name")] = variant(mod_.pretty_name_);
		}
		if(mod_.abbreviation_.empty() == false) {
			m[variant("abbreviation")] = variant(mod_.abbreviation_);
		}
		if(mod_.included_modules_.empty() == false) {
			std::vector<variant> v;
			foreach(const std::string& s, mod_.included_modules_) {
				v.push_back(variant(s));
			}
			m[variant("include-modules")] = variant(&v);
		}
		variant new_module(&m);
		std::string mod_path = "./modules/" + mod_.name_ + "/";
		// create the module file.
		sys::write_file(mod_path + "module.cfg", new_module.write_json());
	}
}

}
#endif // !NO_EDITOR
