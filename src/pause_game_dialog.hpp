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
#ifndef PAUSE_GAME_DIALOG_INCLUDED
#define PAUSE_GAME_DIALOG_INCLUDED

enum PAUSE_GAME_RESULT { PAUSE_GAME_CONTINUE, PAUSE_GAME_CONTROLS, PAUSE_GAME_QUIT, PAUSE_GAME_GO_TO_TITLESCREEN, PAUSE_GAME_GO_TO_LOBBY };

PAUSE_GAME_RESULT show_pause_game_dialog();

struct interrupt_game_exception {
	PAUSE_GAME_RESULT result;
	interrupt_game_exception(PAUSE_GAME_RESULT res=PAUSE_GAME_QUIT) : result(res)
	{}
};

#endif
