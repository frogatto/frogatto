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
