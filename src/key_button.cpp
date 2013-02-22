#include "iphone_controls.hpp"
#include "graphical_font_label.hpp"
#include "i18n.hpp"
#include "key_button.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"
#include "widget_factory.hpp"

namespace gui {


	
namespace {

int vpadding = 4;
int hpadding = 10;

void stoupper(std::string& s)
{
	std::string::iterator i = s.begin();
	std::string::iterator end = s.end();

	while (i != end) {
		*i = ::toupper((unsigned char)*i);
		++i;
	}
}

}

std::string get_key_name(SDL_Scancode key) {
	/*switch(key) {
	// these characters are not contained in the font
	case SDL_SCANCODE_PERIOD:
		return std::string("PERIOD");
	case SDL_SCANCODE_COMMA:
		return "COMMA";
	// abreviations for some long names
	case SDL_SCANCODE_MODE:
		return std::string("ALTGR");
	case SDL_SCANCODE_BACKSPACE:
		return std::string("BKSP");
	case SDL_SCANCODE_CAPSLOCK:
		return std::string("CAPS");
	case SDL_SCANCODE_DELETE:
		return std::string("DEL");
	case SDL_SCANCODE_INSERT:
		return std::string("INS");
	case SDL_SCANCODE_NUMLOCKCLEAR:
		return std::string("NUM");
	case SDL_SCANCODE_PAGEDOWN:
		return std::string("PGDN");
	case SDL_SCANCODE_PAGEUP:
		return std::string("PGUP");
	case SDL_SCANCODE_LEFT:
		return std::string(("←"));
	case SDL_SCANCODE_RIGHT:
		return std::string(("→"));
	case SDL_SCANCODE_UP:
		return std::string(("↑"));
	case SDL_SCANCODE_DOWN:
		return std::string(("↓"));
	// other names can be shortened by taking only the
	// first letter of the first word, i.e. "LEFT SHIFT" --> "LSHIFT"
	default:
		std::string s = SDL_GetScancodeName(key);
		stoupper(s);
		size_t pos = s.find_first_of(' ');
		if (pos == std::string::npos)
			return s;
		else
			return s.erase(1, pos);
	}*/
	return SDL_GetScancodeName(key);
}

SDL_Scancode get_key_sym(const std::string& s)
{
	return SDL_GetScancodeFromName(s.c_str());
}

key_button::key_button(SDL_Scancode key, BUTTON_RESOLUTION button_resolution)
  : label_(widget_ptr(new graphical_font_label(get_key_name(key), "door_label", 2))),
	key_(key), button_resolution_(button_resolution),
	normal_button_image_set_(framed_gui_element::get("regular_button")),
	depressed_button_image_set_(framed_gui_element::get("regular_button_pressed")),
	focus_button_image_set_(framed_gui_element::get("regular_button_focus")),
	current_button_image_set_(normal_button_image_set_), grab_keys_(false)
	
{
	set_environment();
	set_dim(label_->width()+hpadding*2,label_->height()+vpadding*2);
}

key_button::key_button(const variant& v, game_logic::formula_callable* e) 
	: widget(v,e), 	normal_button_image_set_(framed_gui_element::get("regular_button")),
	depressed_button_image_set_(framed_gui_element::get("regular_button_pressed")),
	focus_button_image_set_(framed_gui_element::get("regular_button_focus")),
	current_button_image_set_(normal_button_image_set_), grab_keys_(false)
{
	std::string key = v["key"].as_string();
	key_ = get_key_sym(key);
	label_ = v.has_key("label") ? widget_factory::create(v["label"], e) : widget_ptr(new graphical_font_label(key, "door_label", 2));
	button_resolution_ = v["resolution"].as_string_default("normal") == "normal" ? BUTTON_SIZE_NORMAL_RESOLUTION : BUTTON_SIZE_DOUBLE_RESOLUTION;

	set_dim(label_->width()+hpadding*2,label_->height()+vpadding*2);
}

bool key_button::in_button(int xloc, int yloc) const
{
	translate_mouse_coords(&xloc, &yloc);
	return xloc > x() && xloc < x() + width() &&
	       yloc > y() && yloc < y() + height();
}

void key_button::handle_draw() const
{
	label_->set_loc(x()+width()/2 - label_->width()/2,y()+height()/2 - label_->height()/2);
	current_button_image_set_->blit(x(),y(),width(),height(), button_resolution_);
	label_->draw();
}

bool key_button::handle_event(const SDL_Event& event, bool claimed)
{
    if(claimed) {
		current_button_image_set_ = normal_button_image_set_;
    }

	if(event.type == SDL_MOUSEMOTION && !grab_keys_) {
		const SDL_MouseMotionEvent& e = event.motion;
		if(current_button_image_set_ == depressed_button_image_set_) {
			//pass
		} else if(in_button(e.x,e.y)) {
			current_button_image_set_ = focus_button_image_set_;
		} else {
			current_button_image_set_ = normal_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(in_button(e.x,e.y)) {
			current_button_image_set_ = depressed_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		const SDL_MouseButtonEvent& e = event.button;
		if(current_button_image_set_ == depressed_button_image_set_) {
			if(in_button(e.x,e.y)) {
				current_button_image_set_ = focus_button_image_set_;
				grab_keys_ = true;
				dynamic_cast<graphical_font_label*>(label_.get())->set_text("...");
				claimed = true;
			} else {
				current_button_image_set_ = normal_button_image_set_;
			}
		} else if (grab_keys_) {
			dynamic_cast<graphical_font_label*>(label_.get())->set_text(get_key_name(key_));
			current_button_image_set_ = normal_button_image_set_;
			grab_keys_ = false;
		}
	}

	if(event.type == SDL_KEYDOWN && grab_keys_) {
		key_ = event.key.keysym.scancode;
		if(key_ != SDL_SCANCODE_RETURN && key_ != SDL_SCANCODE_ESCAPE) {
			dynamic_cast<graphical_font_label*>(label_.get())->set_text(get_key_name(key_));
			claimed = true;
			current_button_image_set_ = normal_button_image_set_;
			grab_keys_ = false;
		}
	}

	return claimed;
}

SDL_Scancode key_button::get_key() {
	return key_;
}

void key_button::set_value(const std::string& key, const variant& v)
{
	widget::set_value(key, v);
}

variant key_button::get_value(const std::string& key) const
{
	if(key == "key") {
		return variant(key_);
	}
	return widget::get_value(key);
}

}
