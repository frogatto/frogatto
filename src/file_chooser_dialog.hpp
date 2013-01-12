#pragma once
#ifndef FILE_CHOOSER_DIALOG_HPP_INCLUDED
#define FILE_CHOOSER_DIALOG_HPP_INCLUDED

#include <vector>

#include "dialog.hpp"
#include "dropdown_widget.hpp"
#include "text_editor_widget.hpp"

namespace gui {

typedef std::pair<std::string, std::string> filter_pair;
typedef std::vector<filter_pair> filter_list;

typedef std::vector<std::string> file_list;
typedef std::vector<std::string> dir_list;
typedef std::pair<file_list, dir_list> file_directory_list;
typedef std::map<std::string, file_directory_list> file_directory_map;

class file_chooser_dialog : public virtual dialog
{
public:
	file_chooser_dialog(int x, int y, int w, int h, const filter_list& filters=filter_list(), bool dir_only=false, const std::string& default_path=".");
	std::string get_file_name() { return file_name_; }
	std::string get_path();
	void set_saveas_dialog() { file_open_dialog_ = false; }
	void set_open_dialog() { file_open_dialog_ = true; }
	void use_relative_paths(bool val=true, const std::string& rel_path="");
protected:
	void init();
	void ok_button();
	void cancel_button();
	void up_button();
	void home_button();
	void add_dir_button();
	void text_enter(const text_editor_widget_ptr editor);
	void execute_change_directory(const dir_list& d, int index);
	void execute_select_file(const file_list& f, int index);
	void execute_dir_name_enter(const text_editor_widget_ptr editor);
	void execute_dir_name_select(int row);
	void change_filter(int selection, const std::string& s);
	void set_default_path(const std::string& path);
private:
	std::string abs_default_path_;
	std::string current_path_;
	std::string relative_path_;
	std::string file_name_;
	filter_list filters_;
	int filter_selection_;
	bool file_open_dialog_;
	text_editor_widget_ptr editor_;
	widget_ptr context_menu_;
	dropdown_widget_ptr filter_widget_;
	bool dir_only_;
	bool use_relative_paths_;
};

}

#endif // FILE_CHOOSER_DIALOG_HPP_INCLUDED
