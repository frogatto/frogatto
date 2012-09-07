#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <set>

#include "tbs_ai_player.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

namespace tbs {

struct game_type;

class game : public game_logic::formula_callable
{
public:

	struct error {
		error(const std::string& m);
		std::string msg;
	};

	static boost::intrusive_ptr<game> create(const variant& v);
	static game* current();

	explicit game(const game_type& type);
	explicit game(const variant& v);
	virtual ~game();

	virtual variant write(int nplayer=-1) const;
	virtual void handle_message(int nplayer, const variant& msg);

	int game_id() const { return game_id_; }

	struct message {
		std::vector<int> recipients;
		std::string contents;
	};

	void swap_outgoing_messages(std::vector<message>& msg);

	bool started() const { return started_; }
	virtual bool full() const { return players_.size() == 2; }

	virtual void add_player(const std::string& name);
	virtual void add_ai_player(const std::string& name);

	std::vector<std::string> get_ai_players() const;

	virtual void remove_player(const std::string& name);

	struct player {
		player();
		std::string name;
		int side;
		bool is_human;
	};

	int get_player_index(const std::string& nick) const;

	std::vector<player>& players() { return players_; }
	const std::vector<player>& players() const { return players_; }

	void queue_message(const std::string& msg, int nplayer=-1);
	void queue_message(const char* msg, int nplayer=-1);
	void queue_message(const variant& msg, int nplayer=-1);

	virtual void setup_game();

	virtual void set_as_current_game(bool set) {}

protected:
	void start_game();
	virtual void send_game_state(int nplayer=-1);

	void ai_play();

	void send_notify(const std::string& msg, int nplayer=-1);
	void send_error(const std::string& msg, int nplayer=-1);

	int state_id() const { return state_id_; }

	void set_message(const std::string& msg);

private:

	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);

	virtual ai_player* create_ai() const { return NULL; }

	const game_type& type_;

	int game_id_;

	bool started_;

	int state_id_; //upward counting integer keeping track of the game state.

	//a message which explains the reason for the last game state change.
	std::string current_message_;

	std::vector<player> players_;

	std::vector<message> outgoing_messages_;

	std::vector<std::string> log_;

	enum GAME_STATE { STATE_SETUP, STATE_PLAYING };
	GAME_STATE state_;

	std::vector<boost::shared_ptr<ai_player> > ai_;

	variant doc_;

	game_logic::formula_callable* backup_callable_;

	void handle_event(const std::string& name, game_logic::formula_callable* variables=NULL);
	void execute_command(variant cmd);
};

class game_context {
	game* old_game_;
public:
	explicit game_context(game* g);
	void set(game* g);
	~game_context();
};

typedef boost::intrusive_ptr<game> game_ptr;
typedef boost::intrusive_ptr<const game> const_game_ptr;

}

#endif
