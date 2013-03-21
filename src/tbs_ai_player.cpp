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
#include <algorithm>
#include <numeric>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "tbs_ai_player.hpp"
#include "tbs_game.hpp"

namespace tbs {

ai_player* ai_player::create(game& g, int nplayer)
{
	return NULL; //new default_ai_player(g, nplayer);
}

ai_player::ai_player(const game& g, int nplayer)
  : game_(g), nplayer_(nplayer)
{}

ai_player::~ai_player()
{}

}
