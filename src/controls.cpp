#include <vector>

#include "SDL.h"

#include "controls.hpp"
#include "foreach.hpp"
#include "joystick.hpp"
#include "key.hpp"

namespace controls {
namespace {
const int MAX_PLAYERS = 8;

std::vector<unsigned char> controls[MAX_PLAYERS];
int starting_cycles;
int nplayers = 1;
int local_player;

CKey& keyboard() {
	static CKey key;
	return key;
}
}

void new_level(int level_starting_cycles, int level_nplayers, int level_local_player)
{
	starting_cycles = level_starting_cycles;
	nplayers = level_nplayers;
	local_player = level_local_player;
	foreach(std::vector<unsigned char>& v, controls) {
		v.clear();
	}
}

void read_local_controls()
{
	if(local_player < 0 || local_player >= nplayers) {
		return;
	}

	unsigned char state = 0;
	if(keyboard()[SDLK_UP] || joystick::up()) { state |= (1 << CONTROL_UP); }
	if(keyboard()[SDLK_DOWN] || joystick::down()) { state |= (1 << CONTROL_DOWN); }
	if(keyboard()[SDLK_LEFT] || joystick::left()) { state |= (1 << CONTROL_LEFT); }
	if(keyboard()[SDLK_RIGHT] || joystick::right()) { state |= (1 << CONTROL_RIGHT); }
	if(keyboard()[SDLK_s] || joystick::button(0) || joystick::button(2)) { state |= (1 << CONTROL_ATTACK); }
	if(keyboard()[SDLK_a] || joystick::button(1) || joystick::button(3)) { state |= (1 << CONTROL_JUMP); }

	controls[local_player].push_back(state);
}

void get_control_status(int cycle, int player, bool* output)
{
	cycle -= starting_cycles;

	unsigned char state = 0;
	if(cycle < controls[player].size()) {
		state = controls[player][cycle];
	} else if(!controls[player].empty()) {
		state = controls[player].back();
	}

	for(int n = 0; n != NUM_CONTROLS; ++n) {
		output[n] = (state&(1 << n)) ? true : false;
	}
}

}
