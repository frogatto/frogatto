#ifndef MULTIPLAYER_HPP_INCLUDED
#define MULTIPLAYER_HPP_INCLUDED

namespace multiplayer {
struct error {};
int slot();
void setup_networked_game(const std::string& server);

void sync_start_time();

void send_and_receive();

struct manager {
	manager(bool activate);
	~manager();
};
}

#endif
