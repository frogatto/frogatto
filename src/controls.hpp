#ifndef CONTROLS_HPP_INCLUDED
#define CONTROLS_HPP_INCLUDED

namespace controls {

enum CONTROL_ITEM {
	CONTROL_UP,
	CONTROL_DOWN,
	CONTROL_LEFT,
	CONTROL_RIGHT,
	CONTROL_ATTACK,
	CONTROL_JUMP,
	NUM_CONTROLS,
};

void new_level(int starting_cycle, int nplayers, int local_player);
void read_local_controls();

void get_control_status(int cycle, int player, bool* output);


}

#endif
