#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <assert.h>
#include <inttypes.h>

#include <stdio.h>

#include <vector>

#include "SDL.h"

#include "asserts.hpp"
#include "controls.hpp"
#include "foreach.hpp"
#include "joystick.hpp"
#include "key.hpp"
#include "iphone_controls.hpp"

namespace controls {
namespace {
int npackets_received;
int ngood_packets;
int last_packet_size_;

const int MAX_PLAYERS = 8;

std::vector<unsigned char> controls[MAX_PLAYERS];

//for each player, the highest confirmed cycle we have
int32_t highest_confirmed[MAX_PLAYERS];

//for each player, the highest confirmed cycle of ours that they have
int32_t remote_highest_confirmed[MAX_PLAYERS];

int starting_cycles;
int nplayers = 1;
int local_player;

int first_invalid_cycle_var = -1;

int32_t our_highest_confirmed() {
	int32_t res = -1;
	for(int n = 0; n != nplayers; ++n) {
		if(res == -1 || highest_confirmed[n] < res) {
			res = highest_confirmed[n];
		}
	}

	return res;
}

CKey& keyboard() {
	static CKey key;
	return key;
}
}

int their_highest_confirmed() {
	int32_t res = -1;
	for(int n = 0; n != nplayers; ++n) {
		if(n == local_player) {
			continue;
		}

		if(res == -1 || remote_highest_confirmed[n] < res) {
			res = remote_highest_confirmed[n];
		}
	}

	return res;
}

void new_level(int level_starting_cycles, int level_nplayers, int level_local_player)
{
	std::cerr << "SET STARTING CYCLES: " << level_starting_cycles << "\n";
	starting_cycles = level_starting_cycles;
	nplayers = level_nplayers;
	local_player = level_local_player;
	foreach(std::vector<unsigned char>& v, controls) {
		v.clear();
	}

	foreach(int32_t& highest, highest_confirmed) {
		highest = 0;
	}

	foreach(int32_t& highest, remote_highest_confirmed) {
		highest = 0;
	}
}

void read_local_controls()
{
	if(local_player < 0 || local_player >= nplayers) {
		return;
	}

	unsigned char state = 0;
	if(keyboard()[SDLK_UP] || joystick::up() || iphone_controls::up()) { state |= (1 << CONTROL_UP); }
	if(keyboard()[SDLK_DOWN] || joystick::down() || iphone_controls::down()) { state |= (1 << CONTROL_DOWN); }
	if(keyboard()[SDLK_LEFT] || joystick::left() || iphone_controls::left()) { state |= (1 << CONTROL_LEFT); }
	if(keyboard()[SDLK_RIGHT] || joystick::right() || iphone_controls::right()) { state |= (1 << CONTROL_RIGHT); }
	if(keyboard()[SDLK_s] || joystick::button(0) || joystick::button(2) || iphone_controls::attack()) { state |= (1 << CONTROL_ATTACK); }
	if(keyboard()[SDLK_a] || joystick::button(1) || joystick::button(3) || iphone_controls::jump()) { state |= (1 << CONTROL_JUMP); }

	controls[local_player].push_back(state);
	highest_confirmed[local_player]++;

	//advance networked player's controls based on the assumption that they
	//just did the same thing as last time; incoming packets will correct
	//any assumptions.
	for(int n = 0; n != nplayers; ++n) {
		while(n != local_player && controls[n].size() < controls[local_player].size()) {
			if(controls[n].empty()) {
				controls[n].push_back(0);
			} else {
				controls[n].push_back(controls[n].back());
			}
		}
	}
}

void get_control_status(int cycle, int player, bool* output)
{
	--cycle;
	cycle -= starting_cycles;

	ASSERT_INDEX_INTO_VECTOR(cycle, controls[player]);

	unsigned char state = controls[player][cycle];

	for(int n = 0; n != NUM_CONTROLS; ++n) {
		output[n] = (state&(1 << n)) ? true : false;
	}
}

void read_control_packet(const char* buf, size_t len)
{
	++npackets_received;

	if(len < 13) {
		fprintf(stderr, "ERROR: CONTROL PACKET TOO SHORT: %d\n", (int)len);
		return;
	}

	const char* end_buf = buf + len;

	int slot = *buf++;

	if(slot < 0 || slot >= nplayers) {
		fprintf(stderr, "ERROR: BAD SLOT NUMBER: %d/%d\n", slot, nplayers);
		return;
	}

	if(slot == local_player) {
		fprintf(stderr, "ERROR: NETWORK PLAYER SAYS THEY HAVE THE SAME SLOT AS US!\n");
		return;
	}

	int32_t current_cycle;
	memcpy(&current_cycle, buf, 4);
	current_cycle = ntohl(current_cycle);
	buf += 4;

	if(current_cycle < highest_confirmed[slot]) {
		fprintf(stderr, "DISCARDING PACKET -- OUT OF ORDER: %d < %d\n", current_cycle, highest_confirmed[slot]);
		return;
	}

	int32_t highest_cycle;
	memcpy(&highest_cycle, buf, 4);
	highest_cycle = ntohl(highest_cycle);
	buf += 4;

	if(highest_cycle > remote_highest_confirmed[slot]) {
		remote_highest_confirmed[slot] = highest_cycle;
	}

	int32_t ncycles;
	memcpy(&ncycles, buf, 4);
	ncycles = ntohl(ncycles);
	buf += 4;

	if(end_buf - buf != ncycles) {
		fprintf(stderr, "ERROR: BAD NUMBER OF CYCLES: %d vs %d\n", ncycles, (end_buf - buf));
		return;
	}

	int start_cycle = 1 + current_cycle - ncycles;

	//if we already have data up to this point, don't reprocess it.
	if(start_cycle < highest_confirmed[slot]) {
		int diff = highest_confirmed[slot] - start_cycle;
		ncycles -= diff;
		buf += diff;
		start_cycle = highest_confirmed[slot];
	}

	for(int cycle = start_cycle; cycle <= current_cycle; ++cycle) {
		if(cycle < controls[slot].size()) {
			if(controls[slot][cycle] != *buf) {
				fprintf(stderr, "RECEIVED CORRECTION\n");
				controls[slot][cycle] = *buf;
				if(first_invalid_cycle_var == -1 || first_invalid_cycle_var > cycle) {
					//mark us as invalid back to this point, so game logic
					//will be recalculated from here.
					first_invalid_cycle_var = cycle;
				}
			}
		} else {
			fprintf(stderr, "RECEIVED FUTURE PACKET!\n");
			while(controls[slot].size() <= cycle) {
				controls[slot].push_back(*buf);
			}
		}

		++buf;
	}

	//extend the current control out to the end, to keep the assumption that
	//controls don't change unless we get an explicit signal
	if(current_cycle < static_cast<int>(controls[slot].size()) - 1) {
		for(int n = current_cycle + 1; n < controls[slot].size(); ++n) {
			controls[slot][n] = controls[slot][current_cycle];
		}
	}

	//mark our highest confirmed cycle for this player
	highest_confirmed[slot] = current_cycle;

	assert(buf == end_buf);

	fprintf(stderr, "PROCESSED REMOTE PACKET OKAY\n");
	++ngood_packets;
}

void write_control_packet(std::vector<char>& v)
{
	if(local_player < 0 || local_player >= nplayers) {
		fprintf(stderr, "NO VALID LOCAL PLAYER\n");
		return;
	}

	//write our slot to the packet
	v.push_back(local_player);

	//write our current cycle
	int32_t current_cycle = controls[local_player].size()-1;
	int32_t current_cycle_net = htonl(current_cycle);
	v.resize(v.size() + 4);
	memcpy(&v[v.size()-4], &current_cycle_net, 4);

	//write our highest confirmed cycle
	int32_t highest_cycle = htonl(our_highest_confirmed());
	v.resize(v.size() + 4);
	memcpy(&v[v.size()-4], &highest_cycle, 4);

	int32_t ncycles_to_write = 1+current_cycle - their_highest_confirmed();

	last_packet_size_ = ncycles_to_write;
	if(ncycles_to_write > controls[local_player].size()) {
		ncycles_to_write = controls[local_player].size();
	}

	int32_t ncycles_to_write_net = htonl(ncycles_to_write);
	v.resize(v.size() + 4);
	memcpy(&v[v.size()-4], &ncycles_to_write_net, 4);

	v.insert(v.end(), controls[local_player].end() - ncycles_to_write, controls[local_player].end());
}

int first_invalid_cycle()
{
	return first_invalid_cycle_var;
}

void mark_valid()
{
	first_invalid_cycle_var = -1;
}

int num_players()
{
	return nplayers;
}

int num_errors()
{
	return npackets_received - ngood_packets;
}

int packets_received()
{
	return npackets_received;
}

int cycles_behind()
{
	if(local_player < 0 || local_player >= nplayers) {
		return 0;
	}

	return highest_confirmed[local_player] - our_highest_confirmed();
}

int last_packet_size()
{
	return last_packet_size_;
}

void debug_dump_controls()
{
	fprintf(stderr, "CONTROLS:");
	for(int n = 0; n < nplayers; ++n) {
		fprintf(stderr, " %d:", n);
		for(int m = 0; m < controls[n].size() && m < highest_confirmed[n]; ++m) {
			char c = controls[n][m];
			fprintf(stderr, "%02x", (int)c);
		}
	}

	fprintf(stderr, "\n");

	for(int n = 0; n < nplayers; ++n) {
		for(int m = 0; m < controls[n].size() && m < highest_confirmed[n]; ++m) {
			fprintf(stderr, "CTRL PLAYER %d CYCLE %d: ", n, m);
			for(int j = 0; j != NUM_CONTROLS; ++j) {
				fprintf(stderr, (1 << j)&controls[n][m] ? "1" : "0");
			}
			fprintf(stderr, "\n");
		}
	}
}

}
