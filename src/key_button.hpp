#ifndef KEY_BUTTON_HPP_INCLUDED
#define KEY_BUTTON_HPP_INCLUDED

#include <boost/function.hpp>

#include "button.hpp"
#include "texture.hpp"
#include "widget.hpp"
#include "framed_gui_element.hpp"


namespace gui {

std::string get_key_name(SDLKey key);

//a key selection button widget. Does not derive from button as we don't need the onclick event.
class key_button : public widget
{
public:
	key_button(SDLKey key, BUTTON_RESOLUTION button_resolution);
	key_button(const variant& v, const game_logic::formula_callable_ptr& e);

	SDLKey get_key();

	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;
private:
	bool in_button(int x, int y) const;
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	BUTTON_RESOLUTION button_resolution_;
	widget_ptr label_;
	SDLKey key_;
	bool grab_keys_;

	const_framed_gui_element_ptr normal_button_image_set_,depressed_button_image_set_,focus_button_image_set_,current_button_image_set_;
};

typedef boost::intrusive_ptr<key_button> key_button_ptr;

}

#endif
