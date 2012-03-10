#include "iphone_controls.hpp"
#include "graphical_font_label.hpp"
#include "i18n.hpp"
#include "key_button.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"

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

std::string get_key_name(SDLKey key) {
	switch(key) {
	// these characters are not contained in the font
	case SDLK_CARET:
		return std::string("CARET");
	case SDLK_GREATER:
		return std::string("GREATER");
	case SDLK_HASH:
		return std::string("HASH");
	case SDLK_LESS:
		return std::string("LESS");
	// abreviations for some long names
	case SDLK_MODE:
		return std::string("ALTGR");
	case SDLK_BACKSPACE:
		return std::string("BKSP");
	case SDLK_CAPSLOCK:
		return std::string("CAPS");
	case SDLK_COMPOSE:
		return std::string("COMP");
	case SDLK_DELETE:
		return std::string("DEL");
	case SDLK_INSERT:
		return std::string("INS");
	case SDLK_NUMLOCK:
		return std::string("NUM");
	case SDLK_PAGEDOWN:
		return std::string("PGDN");
	case SDLK_PAGEUP:
		return std::string("PGUP");
	case SDLK_LEFT:
		return std::string(("←"));
	case SDLK_RIGHT:
		return std::string(("→"));
	case SDLK_UP:
		return std::string(("↑"));
	case SDLK_DOWN:
		return std::string(("↓"));
	// other names can be shortened by taking only the
	// first letter of the first word, i.e. "LEFT SHIFT" --> "LSHIFT"
	default:
		std::string s = SDL_GetKeyName(key);
		stoupper(s);
		size_t pos = s.find_first_of(' ');
		if (pos == std::string::npos)
			return s;
		else
			return s.erase(1, pos);
	}
}

key_button::key_button(SDLKey key, BUTTON_RESOLUTION button_resolution)
  : label_(widget_ptr(new graphical_font_label(get_key_name(key), "door_label", 2))),
	key_(key), button_resolution_(button_resolution),
	normal_button_image_set_(framed_gui_element::get("regular_button")),
	depressed_button_image_set_(framed_gui_element::get("regular_button_pressed")),
	focus_button_image_set_(framed_gui_element::get("regular_button_focus")),
	current_button_image_set_(normal_button_image_set_), grab_keys_(false)
	
{
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
		key_ = event.key.keysym.sym;
		if(key_ != SDLK_RETURN && key_ != SDLK_ESCAPE) {
			dynamic_cast<graphical_font_label*>(label_.get())->set_text(get_key_name(key_));
			claimed = true;
			current_button_image_set_ = normal_button_image_set_;
			grab_keys_ = false;
		}
	}

	return claimed;
}

SDLKey key_button::get_key() {
	return key_;
}

}
