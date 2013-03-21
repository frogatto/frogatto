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
#ifndef AI_PLAYER_HPP_INCLUDED
#define AI_PLAYER_HPP_INCLUDED

#include <vector>

#include "variant.hpp"

namespace tbs {

class game;

class ai_player {
public:
	static ai_player* create(game& g, int nplayer);
	ai_player(const game& g, int nplayer);
	virtual ~ai_player();

	virtual variant play() = 0;
	int player_id() const { return nplayer_; }
protected:
	const game& get_game() const { return game_; }
private:
	const game& game_;
	int nplayer_;
};

}

#endif
