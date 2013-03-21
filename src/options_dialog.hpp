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
#ifndef OPTIONS_DIALOG_HPP_INCLUDED
#define OPTIONS_DIALOG_HPP_INCLUDED

#include <string>
#include <vector>

#include "texture.hpp"
#include "geometry.hpp"
#include "dialog.hpp"


class options_dialog : public gui::dialog
{
public:	
	void draw() const;
	options_dialog(int x, int y, int w, int h);
protected:
	virtual void handle_draw() const;
};

typedef boost::intrusive_ptr<options_dialog> options_dialog_ptr;


#endif
