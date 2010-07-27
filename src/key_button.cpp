#include "iphone_controls.hpp"
#include "graphical_font_label.hpp"
#include "key_button.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"

namespace gui {


	
namespace {

int vpadding = 4;
int hpadding = 10;

}

key_button::key_button(SDLKey key, BUTTON_RESOLUTION button_resolution)
  : label_(widget_ptr(new graphical_font_label(SDL_GetKeyName(key), "door_label", 2))),
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
			dynamic_cast<graphical_font_label*>(label_.get())->set_text(SDL_GetKeyName(key_));
			current_button_image_set_ = normal_button_image_set_;
			grab_keys_ = false;
		}
	}

	if(event.type == SDL_KEYDOWN && grab_keys_) {
		key_ = event.key.keysym.sym;
		if ((key_ >= SDLK_a && key_ <= SDLK_z) ||
		    (key_ >= SDLK_SPACE && key_ <= SDLK_AT) ||
		    (key_ >= SDLK_UP && key_ <= SDLK_PAGEDOWN) ||
		    key_ == SDLK_BACKSPACE || key_ == SDLK_TAB || key_ == SDLK_DELETE) {
			dynamic_cast<graphical_font_label*>(label_.get())->set_text(SDL_GetKeyName(key_));
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
