#ifndef NO_EDITOR
#include <boost/bind.hpp>
#include <vector>

#include "asserts.hpp"
#include "image_widget.hpp"
#include "dropdown_widget.hpp"
#include "foreach.hpp"
#include "geometry.hpp"
#include "graphics.hpp"
#include "raster.hpp"

namespace gui {

namespace {
	const std::string dropdown_button_image = "dropdown_button";
}

dropdown_widget::dropdown_widget(const dropdown_list& list, int width, int height, dropdown_type type)
	: list_(list), type_(type), current_selection_(0), dropdown_height_(100)
{
	set_environment();
	set_dim(width, height);
	editor_ = new text_editor_widget(width, height);
	editor_->set_on_change_handler(boost::bind(&dropdown_widget::text_change, this));
	editor_->set_on_enter_handler(boost::bind(&dropdown_widget::text_enter, this));
	editor_->set_on_tab_handler(boost::bind(&dropdown_widget::text_enter, this));
	dropdown_image_ = widget_ptr(new gui_section_widget(dropdown_button_image));
	//if(type_ == DROPDOWN_COMBOBOX) {
	//	editor_->set_focus(true);
	//}
	set_zorder(1);

	init();
}

dropdown_widget::dropdown_widget(const variant& v, game_logic::formula_callable* e)
	: widget(v,e), current_selection_(0), dropdown_height_(100)
{
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
	if(v.has_key("type")) {
		std::string s = v["type"].as_string();
		if(s == "combo" || s == "combobox") {
			type_ = DROPDOWN_COMBOBOX;
		} else if(s == "list" || s == "listbox") {
			type_ = DROPDOWN_LIST;
		} else {
			ASSERT_LOG(false, "Unreognised type: " << s);
		}
	}
	if(v.has_key("text_edit")) {
		editor_ = new text_editor_widget(v["text_edit"], e);
	} else {
		editor_ = new text_editor_widget(width(), height());
	}
	editor_->set_on_enter_handler(boost::bind(&dropdown_widget::text_enter, this));
	editor_->set_on_tab_handler(boost::bind(&dropdown_widget::text_enter, this));
	editor_->set_on_change_handler(boost::bind(&dropdown_widget::text_change, this));
	if(v.has_key("on_change")) {
		change_handler_ = get_environment()->create_formula(v["on_change"]);
		on_change_ = boost::bind(&dropdown_widget::change_delegate, this, _1);
	}
	if(v.has_key("on_select")) {
		select_handler_ = get_environment()->create_formula(v["on_select"]);
		on_select_ = boost::bind(&dropdown_widget::select_delegate, this, _1, _2);
	}
	if(v.has_key("item_list")) {
		list_ = v["item_list"].as_list_string();
	}
	if(v.has_key("default")) {
		current_selection_ = v["default"].as_int();
	}
	init();
}

void dropdown_widget::init()
{
	const int dropdown_image_size = std::max(height(), dropdown_image_->height());
	label_ = new label(list_.size() > 0 ? list_[current_selection_] : "No items");
	label_->set_loc(0, (height() - label_->height()) / 2);
	dropdown_image_->set_loc(width() - height() + (height() - dropdown_image_->width()) / 2, 
		(height() - dropdown_image_->height()) / 2);
	// go on ask me why there is a +20 in the line below.
	// because text_editor_widget uses a magic -20 when setting the width!
	// The magic +4's are because we want the rectangles drawn around the text_editor_widget 
	// to match the ones we draw around the dropdown image.
	editor_->set_dim(width() - dropdown_image_size + 20 + 4, dropdown_image_size + 4);
	editor_->set_loc(-2, -2);

	if(dropdown_menu_) {
		dropdown_menu_.reset(new grid(1));
	} else {
		dropdown_menu_ = new grid(1);
	}
	dropdown_menu_->set_loc(0, height()+2);
	dropdown_menu_->allow_selection(true);
	dropdown_menu_->set_show_background(true);
	dropdown_menu_->swallow_clicks(true);
	dropdown_menu_->set_col_width(0, width());
	dropdown_menu_->set_max_height(dropdown_height_);
	dropdown_menu_->set_dim(width(), dropdown_height_);
	foreach(const std::string& s, list_) {
		dropdown_menu_->add_col(widget_ptr(new label(s, graphics::color_white())));
	}
	dropdown_menu_->register_selection_callback(boost::bind(&dropdown_widget::execute_selection, this, _1));
	dropdown_menu_->set_visible(false);

}

void dropdown_widget::set_selection(int selection)
{
	if(selection >= 0 || size_t(selection) < list_.size()) {
		current_selection_ = selection;
		if(type_ == DROPDOWN_LIST) {
			label_->set_text(list_[current_selection_]);
		} else if(type_ == DROPDOWN_COMBOBOX) {
			editor_->set_text(list_[current_selection_]);
		}
	}
}

void dropdown_widget::change_delegate(const std::string& s)
{
	if(get_environment()) {
		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
		callable->add("selection", variant(s));
		variant v(callable);
		variant value = change_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "dropdown_widget::change_delegate() called without environment!" << std::endl;
	}
}

void dropdown_widget::select_delegate(int selection, const std::string& s)
{
	if(get_environment()) {
		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
		if(selection == -1) {
			callable->add("selection", variant(selection));
		} else {
			callable->add("selection", variant(s));
		}
		variant v(callable);
		variant value = select_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "dropdown_widget::select_delegate() called without environment!" << std::endl;
	}
}

void dropdown_widget::text_enter()
{
	dropdown_list::iterator it = std::find(list_.begin(), list_.end(), editor_->text());
	if(it == list_.end()) {
		current_selection_ = -1;
	} else {
		current_selection_ = it - list_.begin();
	}
	if(on_select_) {
		on_select_(current_selection_, editor_->text());
	}
}

void dropdown_widget::text_change()
{
	if(on_change_) {
		on_change_(editor_->text());
	}
}

void dropdown_widget::handle_draw() const
{
	if(type_ == DROPDOWN_LIST) {
		graphics::draw_hollow_rect(
			rect(x()-1, y()-1, width()+2, height()+2).sdl_rect(), 
			graphics::color_grey());
	}
	graphics::draw_hollow_rect(
		rect(x()+width()-height(), y()-1, height()+1, height()+2).sdl_rect(), 
		graphics::color_grey());

	glPushMatrix();
	glTranslatef(GLfloat(x() & ~1), GLfloat(y() & ~1), 0.0);
	if(type_ == DROPDOWN_LIST) {
		label_->handle_draw();
	} else if(type_ == DROPDOWN_COMBOBOX) {
		editor_->handle_draw();
	}
	if(dropdown_image_) {
		dropdown_image_->handle_draw();
	}
	if(dropdown_menu_ && dropdown_menu_->visible()) {
		dropdown_menu_->handle_draw();
	}
	glPopMatrix();
}

bool dropdown_widget::handle_event(const SDL_Event& event, bool claimed)
{
	SDL_Event ev = event;
	normalize_event(&ev);

	if(claimed) {
		return claimed;
	}

	if(type_ == DROPDOWN_COMBOBOX && editor_) {
		if(editor_->handle_event(ev, claimed)) {
			return true;
		}
	}

	if(dropdown_menu_ && dropdown_menu_->visible()) {
		if(dropdown_menu_->handle_event(ev, claimed)) {
			return true;
		}
	}

	if(event.type == SDL_MOUSEMOTION) {
		return handle_mousemotion(event.motion, claimed);
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		return handle_mousedown(event.button, claimed);
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		return handle_mouseup(event.button, claimed);
	}
	return claimed;
}

bool dropdown_widget::handle_mousedown(const SDL_MouseButtonEvent& event, bool claimed)
{
	point p(event.x, event.y);
	//int button_state = SDL_GetMouseState(&p.x, &p.y);
	if(point_in_rect(p, rect(x(), y(), width()+height(), height()))) {
		claimed = true;
		if(dropdown_menu_) {
			dropdown_menu_->set_visible(!dropdown_menu_->visible());
		}
	}
	return claimed;
}

void dropdown_widget::set_dropdown_height(int h)
{
	dropdown_height_ = h;
	if(dropdown_menu_) {
		dropdown_menu_->set_max_height(dropdown_height_);
	}
}

bool dropdown_widget::handle_mouseup(const SDL_MouseButtonEvent& event, bool claimed)
{
	point p(event.x, event.y);
	//int button_state = SDL_GetMouseState(&p.x, &p.y);
	if(point_in_rect(p, rect(x(), y(), width()+height(), height()))) {
		claimed = true;
	}
	return claimed;
}

bool dropdown_widget::handle_mousemotion(const SDL_MouseMotionEvent& event, bool claimed)
{
	point p;
	int button_state = SDL_GetMouseState(&p.x, &p.y);
	return claimed;
}

void dropdown_widget::execute_selection(int selection)
{
	if(dropdown_menu_) {
		dropdown_menu_->set_visible(false);
	}
	if(selection < 0 || size_t(selection) >= list_.size()) {
		return;
	}
	//std::cerr << "execute_selection: " << selection << std::endl;
	current_selection_ = selection;
	if(type_ == DROPDOWN_LIST) {
		label_->set_text(list_[current_selection_]);
	} else if(type_ == DROPDOWN_COMBOBOX) {
		editor_->set_text(list_[current_selection_]);
	}
	if(on_select_) {
		if(type_ == DROPDOWN_LIST) {
			on_select_(current_selection_, list_[current_selection_]);
		} else if(type_ == DROPDOWN_COMBOBOX) {
			on_select_(current_selection_, editor_->text());
		}
	}
}

int dropdown_widget::get_max_height() const
{
	// Maximum height required, including dropdown and borders.
	return height() + (dropdown_menu_ ? dropdown_menu_->height() : dropdown_height_) + 2;
}

void dropdown_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "on_change") {
		on_change_ = boost::bind(&dropdown_widget::change_delegate, this, _1);
		change_handler_ = get_environment()->create_formula(v);
	} else if(key == "on_select") {
		on_select_ = boost::bind(&dropdown_widget::select_delegate, this, _1, _2);
		select_handler_ = get_environment()->create_formula(v);
	} else if(key == "item_list") {
		list_ = v.as_list_string();
		current_selection_ = 0;
	} else if(key == "selection") {
		current_selection_ = v.as_int();
	} else if(key == "type") {
		std::string s = v.as_string();
		if(s == "combo" || s == "combobox") {
			type_ = DROPDOWN_COMBOBOX;
		} else if(s == "list" || s == "listbox") {
			type_ = DROPDOWN_LIST;
		} else {
			ASSERT_LOG(false, "Unreognised type: " << s);
		}
	}
	widget::set_value(key, v);
}

variant dropdown_widget::get_value(const std::string& key) const
{
	if(key == "selection") {
		return variant(current_selection_);
	} else if(key == "selected_item") {
		if(current_selection_ < 0 || size_t(current_selection_) > list_.size()) {
			return variant();
		}
		return variant(list_[current_selection_]);
	}
	return widget::get_value(key);
}

}
#endif
