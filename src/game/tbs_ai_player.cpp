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
