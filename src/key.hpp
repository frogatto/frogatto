/* $Id: key.hpp 19552 2007-08-15 13:41:56Z mordante $ */
/*
   Copyright (C) 2003 - 2007 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef KEY_HPP_INCLUDED
#define KEY_HPP_INCLUDED

#include "graphics.hpp"

#include <string>

//object which keeps track of all the keys on the keyboard, and
//whether any key is pressed or not can be found by using its
//operator[]. Note though that it is generally better to use
//key events to see when keys are pressed rather than poll using
//this object.
class CKey {
public:
	CKey();

	int operator[](int) const;
	void SetEnabled(bool enable);
	void RequireRelease();

	void Write(std::string* output);
	void Read(const std::string& input);
private:
	Uint8 *key_list;
	Uint8 custom_key_list[512];
	bool is_enabled;
	mutable bool require_key_release;
	int num_keys;
};

#endif
