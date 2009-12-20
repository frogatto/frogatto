#ifndef LEVEL_RUNNER_HPP_INCLUDED
#define LEVEL_RUNNER_HPP_INCLUDED

#include <string>

#include <boost/scoped_ptr.hpp>

#include "key.hpp"
#include "level.hpp"

//an exception which is thrown if we go through a portal which takes us
//to a level with a different number of players, which indicates we are going
//into or out of multiplayer.
struct multiplayer_exception {
};

class level_runner {
public:
	level_runner(boost::scoped_ptr<level>& lvl, std::string& level_cfg);

	bool play_level();
	bool play_cycle();
private:
	boost::scoped_ptr<level>& lvl_;
	std::string& level_cfg_;

	bool quit_;
	boost::scoped_ptr<level> start_lvl_;
	time_t current_second_;

	int current_fps_, next_fps_, current_delay_, next_delay_,
	    current_draw_, next_draw_, current_process_, next_process_,
		current_flip_, next_flip_;

	CKey key;

	int cycle;
	bool paused;
	bool done;
	int start_time_;
	int pause_time_;
};

class pause_scope
{
	int ticks_;
	bool active_;
public:
	pause_scope();
	~pause_scope();
};


#endif
