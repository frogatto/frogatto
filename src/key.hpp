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
	mutable Uint8 *key_list;
	Uint8 custom_key_list[512];
	bool is_enabled;
	mutable bool require_key_release;
	mutable int num_keys;
};

#endif
