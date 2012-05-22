
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
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
