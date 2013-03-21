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
#include "widget.hpp"
#include "asserts.hpp"
#include "formula_callable.hpp"
#include "widget_factory.hpp"
#include "variant.hpp"

#include "animation_preview_widget.hpp"
#include "animation_widget.hpp"
#include "bar_widget.hpp"
#include "border_widget.hpp"
#include "button.hpp"
#include "code_editor_widget.hpp"
#include "checkbox.hpp"
#include "custom_object_widget.hpp"
#include "dialog.hpp"
#include "drag_widget.hpp"
#include "file_chooser_dialog.hpp"
#include "graphical_font_label.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "gui_section.hpp"
#include "key_button.hpp"
#include "label.hpp"
#include "poly_line_widget.hpp"
#include "preview_tileset_widget.hpp"
#include "progress_bar.hpp"
#include "rich_text_label.hpp"
#include "scrollable_widget.hpp"
#include "scrollbar_widget.hpp"
#include "selector_widget.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"

namespace widget_factory {

using gui::widget_ptr;

widget_ptr create(const variant& v, game_logic::formula_callable* e)
{
	if(v.is_callable()) {
		widget_ptr w = v.try_convert<gui::widget>();
		ASSERT_LOG(w != NULL, "Error converting widget from callable.");
		return w;
	}
	ASSERT_LOG(v.is_map(), "TYPE ERROR: widget must be specified by a map, found: " << v.to_debug_string());
	std::string wtype = v["type"].as_string();
	if(wtype == "animation_widget") {
		return widget_ptr(new gui::animation_widget(v,e));
#ifndef NO_EDITOR
	} else if(wtype == "animation_preview") {
		return widget_ptr(new gui::animation_preview_widget(v,e));
#endif
	} else if(wtype == "border_widget") {
		return widget_ptr(new gui::border_widget(v,e));
	} else if(wtype == "button") {
		return widget_ptr(new gui::button(v,e));
	} else if(wtype == "checkbox") {
		return widget_ptr(new gui::checkbox(v,e));
	} else if(wtype == "dialog") {
		return widget_ptr(new gui::dialog(v,e));
#ifndef NO_EDITOR
	} else if(wtype == "drag_widget") {
		return widget_ptr(new gui::drag_widget(v,e));
#endif
	} else if(wtype == "graphical_font_label") {
		return widget_ptr(new gui::graphical_font_label(v,e));
	} else if(wtype == "grid") {
		return widget_ptr(new gui::grid(v,e));
	} else if(wtype == "image") {
		return widget_ptr(new gui::image_widget(v,e));
	} else if(wtype == "section") {
		return widget_ptr(new gui::gui_section_widget(v,e));
	} else if(wtype == "key_button") {
		return widget_ptr(new gui::key_button(v,e));
	} else if(wtype == "label") {
		return widget_ptr(new gui::label(v,e));
	} else if(wtype == "poly_line_widget") {
		return widget_ptr(new gui::poly_line_widget(v,e));
	} else if(wtype == "rich_text_label") {
		return widget_ptr(new gui::rich_text_label(v,e));
	} else if(wtype == "tileset_preview") {
		return widget_ptr(new gui::preview_tileset_widget(v,e));
	} else if(wtype == "scrollbar") {
		return widget_ptr(new gui::scrollbar_widget(v,e));
	} else if(wtype == "slider") {
		return widget_ptr(new gui::slider(v,e));
	} else if(wtype == "text_editor") {
		return widget_ptr(new gui::text_editor_widget(v,e));
	} else if(wtype == "progress") {
		return widget_ptr(new gui::progress_bar(v, e));
	} else if(wtype == "selector") {
		return widget_ptr(new gui::selector_widget(v, e));
	} else if(wtype == "object") {
		return widget_ptr(new gui::custom_object_widget(v, e));
	} else if(wtype == "bar") {
		return widget_ptr(new gui::bar_widget(v, e));
	} else if(wtype == "file_chooser") {
		return widget_ptr(new gui::file_chooser_dialog(v, e));
	//} else if(wtype == "scrollable") {
	//} else if(wtype == "widget") {
	} else {
		ASSERT_LOG(true, "Unable to create a widget of type " << wtype);
		return widget_ptr();
	}
}

}
