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
#ifndef MESSAGE_DIALOG_HPP_INCLUDED
#define MESSAGE_DIALOG_HPP_INCLUDED

#include <string>
#include <vector>

#include "geometry.hpp"
#include "texture.hpp"

class message_dialog
{
public:
	static void show_modal(const std::string& text, const std::vector<std::string>* options=NULL);
	static void clear_modal();
	static message_dialog* get();
	void draw() const;
	void process();

	int selected_option() const { return selected_option_; }
private:
	message_dialog(const std::string& text, const rect& pos,
	               const std::vector<std::string>* options=NULL);
	std::string text_;
	rect pos_;
	int viewable_lines_;
	int line_height_;

	int cur_row_, cur_char_, cur_wait_;

	std::vector<graphics::texture> lines_;
	std::vector<graphics::texture> options_;
	int selected_option_;
};

typedef boost::intrusive_ptr<message_dialog> message_dialog_ptr;

#endif
