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
#ifndef TEXTURE_FRAME_BUFFER_HPP_INCLUDED
#define TEXTURE_FRAME_BUFFER_HPP_INCLUDED

namespace texture_frame_buffer {

bool unsupported();
void init(int width=128, int height=128);
void switch_texture();

void set_as_current_texture();
int width();
int height();
void rebuild();

void set_render_to_texture();
void set_render_to_screen();

struct render_scope {
	render_scope();
	~render_scope();
};

}

#endif
