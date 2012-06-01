#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <sys/types.h>

#include "asserts.hpp"
#include "button.hpp"
#include "color_utils.hpp"
#include "dialog.hpp"
#include "draw_scene.hpp"
#include "dropdown_widget.hpp"
#include "file_chooser_dialog.hpp"
#include "foreach.hpp"
#include "graphics.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "module.hpp"
#include "raster.hpp"
#include "text_editor_widget.hpp"
#include "unit_test.hpp"

#if defined(_WINDOWS)
#include <direct.h>
#define getcwd	_getcwd
#endif

namespace sys {

std::string get_absolute_path(const std::string& path, const std::string& curdir="")
{
	std::string abs_path;
	// A path is absolute if it starts with / (linux)
	// on windows a path is absolute if it starts with \\, x:\, \ 
	//boost::regex regexp(re_absolute_path);
	//bool path_is_absolute = boost::regex_match(path, boost::regex(re_absolute_path));
	//std::cerr << "set_default_path: path(" << path << ") is " << (path_is_absolute ? "absolute" : "relative") << std::endl;

	if(sys::is_path_absolute(path)) {
		abs_path = sys::make_conformal_path(path);
	} else {
		if(curdir.empty()) {
			std::vector<char> buf(1024);
			const char* const res = getcwd(&buf[0], buf.capacity());
			if(res != NULL) {
				abs_path  = sys::make_conformal_path(res);
			} else {
				ASSERT_LOG(false, "getcwd failed");
			}
		} else {
			ASSERT_LOG(sys::is_path_absolute(curdir) == true, "get_absolute_path: curdir was specified but isn't absolute! " << curdir);
			abs_path  = sys::make_conformal_path(curdir);
		}

		std::vector<std::string> cur_path;
		boost::split(cur_path, path, boost::is_any_of("/"));
		foreach(const std::string& s, cur_path) {
			if(s == ".") {
			} else if(s == "..") {
				size_t offs = abs_path.rfind('/');
				if(abs_path.length() > 1 && offs != std::string::npos) {
					abs_path.erase(offs);
				}
			} else {
				abs_path += "/" + s;
			}
		}
		abs_path = sys::make_conformal_path(abs_path);
	}
	return abs_path;
}

}

namespace gui {

file_chooser_dialog::file_chooser_dialog(int x, int y, int w, int h, const filter_list& filters, bool dir_only, const std::string& default_path)
	: dialog(x,y,w,h), filters_(filters), file_open_dialog_(true), filter_selection_(0), dir_only_(dir_only), 
	use_relative_paths_(false)
{
	if(filters_.empty()) {
		filters_.push_back(filter_pair("All files", ".*"));
	}

	relative_path_ = sys::get_absolute_path("");
	set_default_path(default_path);
	
	editor_ = new text_editor_widget(400, 32);
	editor_->set_font_size(16);
	//file_text->set_on_change_handler(boost::bind(&file_chooser_dialog::change_text_attribute, this, change_entry, attr));
	editor_->set_on_enter_handler(boost::bind(&file_chooser_dialog::text_enter, this, editor_));
	editor_->set_on_tab_handler(boost::bind(&file_chooser_dialog::text_enter, this, editor_));

	init();
}

void file_chooser_dialog::set_default_path(const std::string& path)
{
	abs_default_path_ = sys::get_absolute_path(path);
	current_path_ = abs_default_path_;
}

void file_chooser_dialog::init()
{
	int current_height = 30;
	int hpad = 10;
	clear();

	file_list files;
	dir_list dirs;
	sys::get_files_in_dir(current_path_, &files, &dirs);

	std::string l = "Choose File ";
	if(dir_only_) {
		l = "Choose Directory";
	} else {
		l += file_open_dialog_ ? "To Open" : "To Save";
	}

	label_ptr lp = new label(l, graphics::color_white(), 20);
	add_widget(widget_ptr(lp), 30, current_height);
	current_height += lp->height() + hpad;

	lp = new label("Current Path: " + current_path_, graphics::color_green(), 16);
	add_widget(widget_ptr(lp), 30, current_height);
	current_height += lp->height() + hpad;

	/*  Basic list of things needed after extensive review.
		List of directory names from the directory we are currently in.
		Add directory buttons (+)?
		List of files names under the current directory.
		Ok/Cancel Buttons
		Up one level button
		Text entry box for typing the file/directory path. i.e. if file exists choose it, if it's a directory
		  then make it the current_path_;
		
	*/

	grid_ptr g(new grid(3));
	g->set_hpad(50);
	g->add_col(widget_ptr(new button(widget_ptr(new label("Up", graphics::color_white())), boost::bind(&file_chooser_dialog::up_button, this))));
	g->add_col(widget_ptr(new button(widget_ptr(new label("Home", graphics::color_white())), boost::bind(&file_chooser_dialog::home_button, this))));
	g->add_col(widget_ptr(new button(widget_ptr(new label("Add", graphics::color_white())), boost::bind(&file_chooser_dialog::add_dir_button, this))));
	add_widget(g, 30, current_height);	
	current_height += g->height() + hpad;

	grid_ptr container(new grid(dir_only_ ? 1 : 2));
	container->set_hpad(30);
	container->allow_selection(false);
	container->set_col_width(0, dir_only_ ? width()*2 : width()/3);
	if(dir_only_ == false) {
		container->set_col_width(1, width()/3);
	}
	container->set_show_background(false);

	g.reset(new grid(1));
	g->set_dim(dir_only_ ? width()/2 : width()/3, height()/3);
	g->set_max_height(height()/3);
	g->set_show_background(true);
	g->allow_selection();
	foreach(const std::string& dir, dirs) {
		g->add_col(widget_ptr(new label(dir, graphics::color_white())));
	}
	g->register_selection_callback(boost::bind(&file_chooser_dialog::execute_change_directory, this, dirs, _1));
	container->add_col(g);

	if(dir_only_ == false) {
		g.reset(new grid(1));
		g->set_dim(width()/3, height()/3);
		g->set_max_height(height()/3);
		g->set_show_background(true);
		g->allow_selection();
		std::vector<std::string> filtered_file_list;
		foreach(const std::string& file, files) {
			boost::regex re(filters_[filter_selection_].second, boost::regex_constants::icase);
			if(boost::regex_match(file, re)) {
				filtered_file_list.push_back(file);
				g->add_col(widget_ptr(new label(file, graphics::color_white())));
			}
		}
		g->register_selection_callback(boost::bind(&file_chooser_dialog::execute_select_file, this, filtered_file_list, _1));
		container->add_col(g);
	}
	add_widget(container, 30, current_height);
	current_height += container->height() + hpad;

	add_widget(editor_, 30, current_height);
	current_height += editor_->height() + hpad;

	if(dir_only_ == false) {
		dropdown_list dl_list;
		std::transform(filters_.begin(), filters_.end(), 
			std::back_inserter(dl_list), 
			boost::bind(&filter_list::value_type::first,_1));
		filter_widget_ = new dropdown_widget(dl_list, width()/2, 20);
		filter_widget_->set_on_select_handler(boost::bind(&file_chooser_dialog::change_filter, this, _1, _2));
		//std::cerr << "filter_selection: " << filter_selection_ << std::endl;
		filter_widget_->set_selection(filter_selection_);
		add_widget(filter_widget_, 30, current_height);
		current_height += filter_widget_->get_max_height() + hpad;
	}

	g.reset(new grid(2));
	g->set_hpad(20);
	g->add_col(widget_ptr(new button(widget_ptr(new label("OK", graphics::color_white())), boost::bind(&file_chooser_dialog::ok_button, this))));
	g->add_col(widget_ptr(new button(widget_ptr(new label("Cancel", graphics::color_white())), boost::bind(&file_chooser_dialog::cancel_button, this))));
	add_widget(g, 30, current_height);
	current_height += g->height() + hpad;
}

void file_chooser_dialog::change_filter(int selection, const std::string& s)
{
	//std::cerr << "New Filter: " << s << " : " << selection << std::endl;
	if(selection >= 0) {
		filter_selection_ = selection;
	}
	init();
}

void file_chooser_dialog::execute_change_directory(const dir_list& d, int index)
{
	if(index < 0 || size_t(index) >= d.size()) {
		return;
	}
	if(d[index] == "."){
		return;
	}
	if(d[index] == ".."){
		up_button();
	}
	current_path_ = current_path_ + "/" + d[index];
	if(dir_only_) {
		editor_->set_text(get_path());
	} else {
		editor_->set_text("");
	}
	init();
}

void file_chooser_dialog::ok_button() 
{
	close();
}

void file_chooser_dialog::cancel_button() 
{
	cancel();
	close();
}

void file_chooser_dialog::home_button()
{
	current_path_ = relative_path_;
	if(dir_only_) {
		editor_->set_text(get_path());
	} else {
		editor_->set_text("");
	}
	init();
}

void file_chooser_dialog::up_button()
{
	size_t offs = current_path_.rfind('/');
	if(current_path_.length() > 1 && offs != std::string::npos) {
		current_path_.erase(offs);
		if(dir_only_) {
			editor_->set_text(get_path());
		} else {
			editor_->set_text("");
		}
	}
	init();
}

void file_chooser_dialog::add_dir_button()
{
	gui::grid* grid = new gui::grid(1);
	grid->set_show_background(true);
	grid->allow_selection(true);
	grid->swallow_clicks(false);
	grid->allow_draw_highlight(false);
	text_editor_widget_ptr dir_name_editor = new text_editor_widget(200, 28);
	dir_name_editor->set_font_size(14);
	dir_name_editor->set_on_enter_handler(boost::bind(&file_chooser_dialog::execute_dir_name_enter, this, dir_name_editor));
	dir_name_editor->set_on_tab_handler(boost::bind(&file_chooser_dialog::execute_dir_name_enter, this, dir_name_editor));
	dir_name_editor->set_focus(true);
	grid->add_col(dir_name_editor);
	grid->register_selection_callback(boost::bind(&file_chooser_dialog::execute_dir_name_select, this, _1));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex, mousey);
}

void file_chooser_dialog::execute_dir_name_select(int row)
{
	if(row == -1 && context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}
}

void file_chooser_dialog::execute_dir_name_enter(const text_editor_widget_ptr editor)
{
	if(context_menu_) {
		remove_widget(context_menu_);
		context_menu_.reset();
	}

	if(editor->text().empty() == false) {
		std::string new_path = sys::get_dir(sys::get_absolute_path(editor->text(), current_path_));
		if(new_path.empty() == false) {
			current_path_ = new_path;
			if(dir_only_) {
				editor_->set_text(get_path());
			} else {
				editor_->set_text("");
			}
		} else {
			std::cerr << "Failed to create directory " << editor->text() << " in " << current_path_ << std::endl;
		}
	}
	init();
}

void file_chooser_dialog::text_enter(const text_editor_widget_ptr editor)
{
	if(dir_only_) {
		std::string path = sys::get_absolute_path(editor->text(), current_path_);
		if(sys::is_directory(path)) {
			current_path_ = path;
			editor->set_text(get_path());
		} else {
			path = sys::get_absolute_path(editor->text(), relative_path_);
			if(sys::is_directory(path)) {
				current_path_ = path;
				editor->set_text(get_path());
			} else {
				std::cerr << "Invalid Path: " << path << std::endl;
			}
		}
	} else if(file_open_dialog_) {
		if(sys::file_exists(editor->text())) {
			file_name_ = editor->text();
		} else if(sys::is_directory(editor->text())) {
			current_path_ = editor->text();
			editor->set_text("");
		} else {
			// Not a valid file or directory name.
			// XXX
		}
	} else {
		// save as...
		if(sys::file_exists(editor->text())) {
			// XXX File exists prompt with an over-write confirmation box.
			file_name_ = editor->text();
		} else if(sys::is_directory(editor->text())) {
			current_path_ = editor->text();
			editor->set_text("");
		} else {
			file_name_ = editor->text();
		}
	}
	init();
}

void file_chooser_dialog::execute_select_file(const file_list& f, int index)
{
	if(index < 0 || size_t(index) >= f.size()) {
		return;
	}
	file_name_ = current_path_ + "/" + f[index];
	editor_->set_text(f[index]);
	init();
}

std::string file_chooser_dialog::get_path()
{
	if(use_relative_paths_) {
		//std::cerr << "get_path: " << std::endl << "\t" << relative_path_ << std::endl << "\t" << current_path_ << std::endl;
		return sys::compute_relative_path(relative_path_, current_path_);
	} 
	return current_path_;
}

void file_chooser_dialog::use_relative_paths(bool val, const std::string& rel_path) 
{ 
	use_relative_paths_ = val; 
	relative_path_ = sys::get_absolute_path(rel_path);
	if(editor_) {
		editor_->set_text(get_path());
	}
}

}

UNIT_TEST(compute_relative_paths_test) {
	CHECK_EQ(sys::compute_relative_path("/home/tester/frogatto/images", "/home/tester/frogatto/data"), "../data");
	CHECK_EQ(sys::compute_relative_path("/", "/"), "");
	CHECK_EQ(sys::compute_relative_path("/home/tester", "/"), "../..");
	CHECK_EQ(sys::compute_relative_path("/", "/home"), "home");
	CHECK_EQ(sys::compute_relative_path("C:/Projects/frogatto", "C:/Projects"), "..");
	CHECK_EQ(sys::compute_relative_path("C:/Projects/frogatto/images/experimental", "C:/Projects/xyzzy/test1/test2"), "../../../xyzzy/test1/test2");
	CHECK_EQ(sys::compute_relative_path("C:/Projects/frogatto/", "C:/Projects/frogatto/modules/vgi/images"), "modules/vgi/images");
	CHECK_EQ(sys::compute_relative_path("C:/Projects/frogatto-build/Frogatto/Win32/Release", "C:/Projects/frogatto-build/Frogatto/Win32/Release/modules/vgi/images"), "modules/vgi/images");
	CHECK_EQ(sys::compute_relative_path("C:/Projects/frogatto-build/Frogatto/Win32/Release", "c:/windows"), "../../../../../windows");
}

