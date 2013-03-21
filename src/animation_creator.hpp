/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef ANIMATION_CREATOR_HPP_INCLUDED
#define ANIMATION_CREATOR_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <vector>
#include <map>

#include "animation_preview_widget.hpp"
#include "checkbox.hpp"
#include "button.hpp"
#include "dialog.hpp"
#include "dropdown_widget.hpp"
#include "graphics.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"
#include "variant.hpp"

namespace gui {

class animation_creator_dialog : public virtual dialog
{
public:
	animation_creator_dialog(int x, int y, int w, int h, const variant& anims=variant());
	virtual ~animation_creator_dialog() 
	{}
	variant get_animations() { return variant(&anims_); }
	void process();
protected:
	void init();
	
	void set_destination();
	void select_animation(int index);
	void set_option();
	void anim_del();
	void anim_new();
	void anim_save(dialog* d);
	void finish();
	bool show_attribute(variant v);

	void check_anim_changed();
	void reset_current_object();

	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);
private:
	std::vector<variant> anims_;
	variant current_;				// Holds the currently selected variant.
	int selected_frame_;

	std::string copy_path_;
	std::string image_file_name_;	// file name.
	std::string image_file_;		// full path.
	std::string rel_path_;			// Path relative to modules images path.

	bool changed_;					// current animation modified?
	bool simple_options_;			// simplified list of options.

	std::vector<std::string> common_animation_list();
	void on_id_change(dropdown_widget_ptr editor, const std::string& s);
	void on_id_set(dropdown_widget_ptr editor, int selection, const std::string& s);
	void set_image_file();
	void change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slider);
	void execute_change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slider);
	void change_slide(const std::string& s, text_editor_widget_ptr editor, double d);
	void end_slide(const std::string& s, slider_ptr slide, text_editor_widget_ptr editor, double d);

	void set_animation_rect(rect r);
	void move_solid_rect(int dx, int dy);
	void set_integer_attr(const char* attr, int value);

	typedef std::pair<std::string, int> slider_offset_pair;
	std::map<std::string, int> slider_offset_;
	bool dragging_slider_;

	animation_preview_widget_ptr animation_preview_;
};

}

#endif // NO_EDITOR
#endif // ANIMATION_CREATOR_HPP_INCLUDED
