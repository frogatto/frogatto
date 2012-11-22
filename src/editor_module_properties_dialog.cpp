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

namespace editor_dialogs
{

namespace 
{
	const char cube_img[266] = {137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 
		73, 72, 68, 82, 0, 0, 0, 16, 0, 0, 0, 16, 8, 2, 0, 0, 0, 144, 145, 
		104, 54, 0, 0, 0, 7, 116, 73, 77, 69, 7, 220, 4, 23, 9, 56, 22, 125, 
		252, 141, 55, 0, 0, 0, 23, 116, 69, 88, 116, 83, 111, 102, 116, 119, 
		97, 114, 101, 0, 71, 76, 68, 80, 78, 71, 32, 118, 101, 114, 32, 51, 
		46, 52, 113, 133, 164, 225, 0, 0, 0, 8, 116, 112, 78, 71, 71, 76, 
		68, 51, 0, 0, 0, 0, 74, 128, 41, 31, 0, 0, 0, 4, 103, 65, 77, 65, 0,
		0, 177, 143, 11, 252, 97, 5, 0, 0, 0, 6, 98, 75, 71, 68, 0, 255, 0, 
		255, 0, 255, 160, 189, 167, 147, 0, 0, 0, 101, 73, 68, 65, 84, 120, 
		156, 221, 210, 209, 17, 128, 32, 12, 3, 208, 174, 232, 32, 30, 35, 
		116, 177, 78, 226, 50, 202, 89, 225, 66, 83, 208, 111, 115, 252, 53, 
		143, 175, 72, 217, 55, 126, 210, 146, 156, 210, 234, 209, 194, 76, 
		102, 85, 12, 50, 89, 87, 153, 61, 64, 85, 207, 59, 105, 213, 79, 102,
		54, 0, 79, 96, 189, 234, 73, 0, 50, 172, 190, 128, 154, 250, 189, 81, 
		254, 5, 216, 48, 136, 243, 10, 12, 65, 156, 6, 143, 175, 131, 213, 
		248, 62, 206, 251, 2, 161, 49, 129, 1, 89, 58, 130, 187, 0, 0, 0,
		0, 73, 69, 78, 68, 174, 66, 96, 130};
}

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
		text_editor_widget_ptr change_id_entry(new text_editor_widget(200, 30));
		change_id_entry->set_on_change_handler(boost::bind(&editor_module_properties_dialog::change_id, this, change_id_entry));
		change_id_entry->set_on_enter_handler(boost::bind(&dialog::close, this));

		g->add_col(widget_ptr(new label("Identifier:  ", graphics::color_white(), 36)))
			.add_col(widget_ptr(change_id_entry));
		add_widget(g);
	} else {
		g->add_col(widget_ptr(new label("Identifier: ", graphics::color_white(), 36)))
			.add_col(widget_ptr(new label(mod_.name_, graphics::color_white(), 36)));
		add_widget(g);
	}

	text_editor_widget_ptr change_name_entry(new text_editor_widget(200, 30));
	change_name_entry->set_text(mod_.pretty_name_);
	change_name_entry->set_on_change_handler(boost::bind(&editor_module_properties_dialog::change_name, this, change_name_entry));
	change_name_entry->set_on_enter_handler(boost::bind(&dialog::close, this));

	g.reset(new grid(2));
	g->add_col(widget_ptr(new label("Name:", graphics::color_white(), 36)))
	  .add_col(widget_ptr(change_name_entry));
	add_widget(g);

	text_editor_widget_ptr change_abbrev_entry(new text_editor_widget(200, 30));
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

void editor_module_properties_dialog::change_id(const gui::text_editor_widget_ptr editor)
{
	if(std::find(dirs_.begin(), dirs_.end(), editor->text()) == dirs_.end()) {
		mod_.name_ = editor->text();
	}
}

void editor_module_properties_dialog::change_name(const gui::text_editor_widget_ptr editor)
{
	mod_.pretty_name_ = editor->text();
}

void editor_module_properties_dialog::change_prefix(const gui::text_editor_widget_ptr editor)
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

	if(index < 0 || size_t(index) >= choices.size()) {
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
		sys::get_dir(mod_path + "data/gui");
		sys::get_dir(mod_path + "images");
		sys::get_dir(mod_path + "sounds");
		sys::get_dir(mod_path + "music");
		// Create an empty titlescreen.cfg
		variant empty_lvl = json::parse_from_file("data/level/empty.cfg");
		empty_lvl.add_attr(variant("id"), variant("titlescreen.cfg"));

		std::map<variant, variant> playable_m;
		playable_m[variant("_addr")] = variant("1010101");
		playable_m[variant("current_frame")] = variant("normal");
		playable_m[variant("custom")] = variant("yes");
		playable_m[variant("face_right")] = variant(1);
		playable_m[variant("is_human")] = variant(1);
		playable_m[variant("label")] = variant("_1111");
		playable_m[variant("time_in_frame")] = variant(0);
		playable_m[variant("type")] = variant("simple_playable");
		playable_m[variant("x")] = variant(0);
		playable_m[variant("y")] = variant(0);
		empty_lvl.add_attr(variant("character"), variant(&playable_m));
		sys::write_file(mod_path + preferences::level_path() + "titlescreen.cfg", empty_lvl.write_json());

		// Module specifed as standalone, write out a few extra useful files.
		if(mod_.included_modules_.empty()) {
			// data/fonts.cfg			-- {font:["@flatten","@include data/dialog_font.cfg","@include data/label_font.cfg"]}
			// data/functions.cfg		-- {}
			// data/gui.cfg				-- {section:["@flatten","@include data/editor-tools.cfg","@include data/gui-elements.cfg"],framed_gui_element: ["@flatten","@include data/framed-gui-elements.cfg"]}
			// data/music.cfg			-- {}
			// data/preload.cfg			-- { preload: [], }
			// data/tiles.cfg			-- {}
			// data/gui/null.cfg		-- {}
			sys::write_file(mod_path + "data/fonts.cfg", "{font:[\"@flatten\",\"@include data/dialog_font.cfg\",\"@include data/label_font.cfg\"]}");
			sys::write_file(mod_path + "data/functions.cfg", "[\n]");
			sys::write_file(mod_path + "data/music.cfg", "{\n}");
			sys::write_file(mod_path + "data/tiles.cfg", "{\n}");
			sys::write_file(mod_path + "data/gui/null.cfg", "{\n}");
			sys::write_file(mod_path + "data/preload.cfg", "{\npreload: [\n],\n}");
			sys::write_file(mod_path + "data/gui/default.cfg", "{\n}");
			sys::write_file(mod_path + "data/gui.cfg", 
				"{\nsection:["
				"\n\t\"@flatten\","
				"\n\t\"@include data/editor-tools.cfg\","
				"\n\t\"@include data/gui-elements.cfg\""
				"\n],"
				"framed_gui_element: ["
				"\n\t\"@flatten\","
				"\n\t\"@include data/framed-gui-elements.cfg\""
				"\n]\n}");
			sys::write_file(mod_path + "data/objects/simple_playable.cfg", 
				"{\n"
				"\tid: \"simple_playable\",\n"
				"\tis_human: true,\n"
				"\thitpoints: 4,\n"
				"\teditor_info: { category: \"player\" },\n"
				"\tanimation: [\n"
				"\t\t{\n"
				"\t\tid: \"stand\",\n"
				"\t\timage: \"cube.png\",\n"
				"\t\trect: [0,0,15,15]\n"
				"\t\t}\n"
				"\t],\n"
				"}");
			sys::write_file(mod_path + "images/cube.png", std::string(cube_img, 266));
		}
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
			m[variant("dependencies")] = variant(&v);
		}
		m[variant("min_engine_version")] = preferences::version_decimal();
		variant new_module(&m);
		std::string mod_path = "./modules/" + mod_.name_ + "/";
		// create the module file.
		sys::write_file(mod_path + "module.cfg", new_module.write_json());
	}
}

}
#endif // !NO_EDITOR
