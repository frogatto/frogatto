#include "widget.hpp"
#include "asserts.hpp"
#include "formula_callable.hpp"
#include "widget_factory.hpp"
#include "variant.hpp"

#include "animation_preview_widget.hpp"
#include "animation_widget.hpp"
#include "border_widget.hpp"
#include "button.hpp"
#include "code_editor_widget.hpp"
#include "checkbox.hpp"
#include "dialog.hpp"
#include "drag_widget.hpp"
#include "graphical_font_label.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "gui_section.hpp"
#include "key_button.hpp"
#include "label.hpp"
#include "poly_line_widget.hpp"
#include "preview_tileset_widget.hpp"
#include "scrollable_widget.hpp"
#include "scrollbar_widget.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"

namespace widget_factory {

using gui::widget_ptr;

widget_ptr create(const variant& v, game_logic::formula_callable* e)
{
	ASSERT_LOG(v.is_map(), "TYPE ERROR: widget must be specified by a map");	
	std::string wtype = v["type"].as_string();
	std::cerr << "Widget Factory creating widget of type: " << wtype << std::endl;
	if(wtype == "animation_preview") {
		return widget_ptr(new gui::animation_preview_widget(v,e));
	} else if(wtype == "animation_widget") {
		return widget_ptr(new gui::animation_widget(v,e));
	} else if(wtype == "border_widget") {
		return widget_ptr(new gui::border_widget(v,e));
	} else if(wtype == "button") {
		return widget_ptr(new gui::button(v,e));
	} else if(wtype == "checkbox") {
		return widget_ptr(new gui::checkbox(v,e));
	} else if(wtype == "dialog") {
		return widget_ptr(new gui::dialog(v,e));
	} else if(wtype == "drag_widget") {
		return widget_ptr(new gui::drag_widget(v,e));
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
	} else if(wtype == "tileset_preview") {
		return widget_ptr(new gui::preview_tileset_widget(v,e));
	} else if(wtype == "scrollbar") {
		return widget_ptr(new gui::scrollbar_widget(v,e));
	} else if(wtype == "slider") {
		return widget_ptr(new gui::slider(v,e));
	} else if(wtype == "text_editor") {
		return widget_ptr(new gui::text_editor_widget(v,e));
	//} else if(wtype == "scrollable") {
	//} else if(wtype == "widget") {
	} else {
		ASSERT_LOG(true, "Unable to create a widget of type " << wtype);
		return widget_ptr();
	}
}

}
