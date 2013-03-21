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
#ifndef KEY_BUTTON_HPP_INCLUDED
#define KEY_BUTTON_HPP_INCLUDED

#include <boost/function.hpp>

#include "button.hpp"
#include "controls.hpp"
#include "texture.hpp"
#include "widget.hpp"
#include "framed_gui_element.hpp"


namespace gui {

std::string get_key_name(key_type key);

//a key selection button widget. Does not derive from button as we don't need the onclick event.
class key_button : public widget
{
public:
	key_button(key_type key, BUTTON_RESOLUTION button_resolution);
	key_button(const variant& v, game_logic::formula_callable* e);

	key_type get_key();

	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;
private:
	bool in_button(int x, int y) const;
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	BUTTON_RESOLUTION button_resolution_;
	widget_ptr label_;
	key_type key_;
	bool grab_keys_;

	const_framed_gui_element_ptr normal_button_image_set_,depressed_button_image_set_,focus_button_image_set_,current_button_image_set_;
};

typedef boost::intrusive_ptr<key_button> key_button_ptr;

}

#endif
