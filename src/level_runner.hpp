#ifndef LEVEL_RUNNER_HPP_INCLUDED
#define LEVEL_RUNNER_HPP_INCLUDED

#include <string>

#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "button.hpp"
#include "debug_console.hpp"
#include "geometry.hpp"
#include "key.hpp"
#include "level.hpp"
#include "pause_game_dialog.hpp"
#include "slider.hpp"

//an exception which is thrown if we go through a portal which takes us
//to a level with a different number of players, which indicates we are going
//into or out of multiplayer.
struct multiplayer_exception {
};

class editor;
struct editor_resolution_manager;

class level_runner {
public:
	static level_runner* get_current();
	level_runner(boost::intrusive_ptr<level>& lvl, std::string& level_cfg, std::string& original_level_cfg);

	bool play_level();
	bool play_cycle();

	void toggle_pause();
	void toggle_history_trails();
private:
	void start_editor();
	void close_editor();
	void reverse_cycle();
	void handle_pause_game_result(PAUSE_GAME_RESULT result);
	typedef boost::intrusive_ptr<level> level_ptr;
	level_ptr& lvl_;
	std::string& level_cfg_;
	std::string& original_level_cfg_;

	bool quit_;
	boost::intrusive_ptr<level> start_lvl_;
	time_t current_second_;

	int current_fps_, next_fps_, current_cycles_, next_cycles_, current_delay_, next_delay_,
	    current_draw_, next_draw_, current_process_, next_process_,
		current_flip_, next_flip_, current_events_;
	std::string profiling_summary_;
	int nskip_draw_;

	CKey key;

	int cycle;
	int die_at;
	bool paused;
	bool done;
	int start_time_;
	int pause_time_;

	point last_stats_point_;
	std::string last_stats_point_level_;
	bool handle_mouse_events(const SDL_Event &event);
	void video_resize_event(const SDL_Event &event);
	void show_pause_title();

	editor* editor_;
#ifndef NO_EDITOR
	boost::scoped_ptr<editor_resolution_manager> editor_resolution_manager_;
	gui::slider_ptr history_slider_;
	gui::button_ptr history_button_;
	std::vector<entity_ptr> history_trails_;
	std::string history_trails_label_;
	int history_trails_state_id_;
	int object_reloads_state_id_;
	int tile_rebuild_state_id_;
	void init_history_slider();
	void on_history_change(double value);
	void update_history_trails();

	boost::scoped_ptr<debug_console::console_dialog> console_;
#endif
};

class pause_scope
{
	int ticks_;
	bool active_;
public:
	pause_scope();
	~pause_scope();
};

void begin_skipping_game();
void end_skipping_game();
bool is_skipping_game();

void video_resize( SDL_Event &event );

#endif
