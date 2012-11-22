#ifndef MULTIPLAYER_HPP_INCLUDED
#define MULTIPLAYER_HPP_INCLUDED

#include <boost/function.hpp>

class level;

namespace multiplayer {
struct error {};
int slot();
void setup_networked_game(const std::string& server);

void sync_start_time(const level& lvl, boost::function<bool()> idle_fn);

void send_and_receive();
void receive();

struct manager {
	manager(bool activate);
	~manager();
};
}

#endif
