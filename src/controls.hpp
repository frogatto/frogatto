#ifndef CONTROLS_HPP_INCLUDED
#define CONTROLS_HPP_INCLUDED

#include <vector>
#include <cstddef>

#include "graphics.hpp"

namespace controls {

enum CONTROL_ITEM {
	CONTROL_UP,
	CONTROL_DOWN,
	CONTROL_LEFT,
	CONTROL_RIGHT,
	CONTROL_ATTACK,
	CONTROL_JUMP,
	CONTROL_TONGUE,
	NUM_CONTROLS,
};

void set_sdlkey (CONTROL_ITEM item, SDLKey key);
SDLKey get_sdlkey (CONTROL_ITEM item);

void new_level(int starting_cycle, int nplayers, int local_player);

//an object which can lock controls into a specific state for the duration
//of its scope.
class local_controls_lock {
public:
	explicit local_controls_lock(unsigned char state=0);
	~local_controls_lock();
};

void read_until(int ncycle);
void read_local_controls();
void unread_local_controls();
void ignore_current_keypresses();

void get_control_status(int cycle, int player, bool* output);
void set_delay(int delay);

void read_control_packet(const char* buf, size_t len);
void write_control_packet(std::vector<char>& v);

int first_invalid_cycle();
void mark_valid();

int num_players();
int num_errors();
int packets_received();
int cycles_behind();

int their_highest_confirmed();
int last_packet_size();

void set_checksum(int cycle, int sum);

void debug_dump_controls();

}

#endif
