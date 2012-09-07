#include <algorithm>
#include <string>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "tbs_ai_player.hpp"
#include "tbs_game.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace tbs {

struct game_type {
	game_type() {
	}

	explicit game_type(const variant& value)
	{
		variant functions_var = value["functions"];
		if(functions_var.is_string()) {
			functions.reset(new game_logic::function_symbol_table);
			game_logic::formula f(functions_var, functions.get());
		}

		variant handlers_var = value["handlers"];
		if(handlers_var.is_map()) {
			foreach(const variant& key, handlers_var.get_keys().as_list()) {
				handlers[key.as_string()] = game_logic::formula::create_optional_formula(handlers_var[key], functions.get());
			}
		}
	}

	boost::shared_ptr<game_logic::function_symbol_table> functions;
	std::map<std::string, game_logic::const_formula_ptr> handlers;
};

std::map<std::string, game_type> generate_game_types() {

	std::map<std::string, game_type> result;

	std::vector<std::string> files;
	module::get_files_in_dir("data/tbs", &files);
	foreach(const std::string& fname, files) {
		if(fname.size() > 4 && std::string(fname.end()-4,fname.end()) == ".cfg") {
			std::string type(fname.begin(), fname.end()-4);
			result[type] = game_type(json::parse_from_file("data/tbs/" + fname));
			std::cerr << "LOADED TBS GAME TYPE: " << type << "\n";
		}
	}

	return result;
}

std::map<std::string, game_type>& all_types() {
	static std::map<std::string, game_type> types = generate_game_types();
	return types;
}

extern std::string global_debug_str;

namespace {
game* current_game = NULL;

int generate_game_id() {
	static int id = int(time(NULL));
	return id++;
}

using namespace game_logic;
}

game::error::error(const std::string& m) : msg(m)
{
	std::cerr << "game error: " << m << "\n";
}

game_context::game_context(game* g) : old_game_(current_game)
{
	current_game = g;
	g->set_as_current_game(true);
}

game_context::~game_context()
{
	current_game->set_as_current_game(false);
	current_game = old_game_;
}

void game_context::set(game* g)
{
	current_game = g;
	g->set_as_current_game(true);
}

game* game::current()
{
	return current_game;
}

boost::intrusive_ptr<game> game::create(const variant& v)
{
	const variant type_var = v["game_type"];
	if(!type_var.is_string()) {
		return NULL;
	}

	boost::intrusive_ptr<game> result(new game(all_types()[type_var.as_string()]));
	game_logic::map_formula_callable_ptr vars(new game_logic::map_formula_callable);
	vars->add("msg", v);
	result->handle_event("create", vars.get());
	return result;
}

game::game(const game_type& type)
  : type_(type), game_id_(generate_game_id()),
    started_(false), state_(STATE_SETUP), state_id_(0)
{
}

game::game(const variant& value)
  : type_(all_types()[value["type"].as_string()]),
    game_id_(generate_game_id()),
    started_(value["started"].as_bool(false)),
	state_(STATE_SETUP),
	state_id_(0)
{
}

game::~game()
{
	std::cerr << "DESTROY GAME\n";
}

variant game::write(int nplayer) const
{
	variant_builder result;
	result.add("id", game_id_);
	result.add("type", "game");
	result.add("started", started_);
	result.add("state_id", state_id_);

	if(current_message_.empty() == false) {
		result.add("message", current_message_);
	}

	if(nplayer < 0) {
		result.add("observer", true);
	}

	if(type_.handlers.count("transform")) {
		variant msg = deep_copy_variant(doc_);
		game_logic::map_formula_callable_ptr vars(new game_logic::map_formula_callable);
		vars->add("message", msg);
		vars->add("nplayer", variant(nplayer));
		const_cast<game*>(this)->handle_event("transform", vars.get());

		result.add("state", msg);
	} else {
		result.add("state", doc_);
	}

	std::string log_str;
	foreach(const std::string& s, log_) {
		if(!log_str.empty()) {
			log_str += "\n";
		}
		log_str += s;
	}

	result.add("log", variant(log_str));

	return result.build();
}

void game::start_game()
{
	if(started_) {
		send_notify("The game has been restarted.");
	}

	state_ = STATE_PLAYING;
	started_ = true;
	handle_event("start");

	send_game_state();

	ai_play();
}

void game::swap_outgoing_messages(std::vector<message>& msg)
{
	outgoing_messages_.swap(msg);
	outgoing_messages_.clear();
}

void game::queue_message(const std::string& msg, int nplayer)
{
	outgoing_messages_.push_back(message());
	outgoing_messages_.back().contents = msg;
	if(nplayer >= 0) {
		outgoing_messages_.back().recipients.push_back(nplayer);
	}
}

void game::queue_message(const char* msg, int nplayer)
{
	queue_message(std::string(msg), nplayer);
}

void game::queue_message(const variant& msg, int nplayer)
{
	queue_message(msg.write_json(), nplayer);
}

void game::send_error(const std::string& msg, int nplayer)
{
	variant_builder result;
	result.add("type", "error");
	result.add("message", msg);
	queue_message(result.build(), nplayer);
}

void game::send_notify(const std::string& msg, int nplayer)
{
	variant_builder result;
	result.add("type", "message");
	result.add("message", msg);
	queue_message(result.build(), nplayer);
}

game::player::player()
{
}

void game::add_player(const std::string& name)
{
	players_.push_back(player());
	players_.back().name = name;
	players_.back().side = players_.size() - 1;
	players_.back().is_human = true;
}

void game::add_ai_player(const std::string& name)
{
	ai_player* ai = create_ai();
	if(!ai) {
		return;
	}

	players_.push_back(player());
	players_.back().name = name;
	players_.back().side = players_.size() - 1;
	players_.back().is_human = false;
	ai_.push_back(boost::shared_ptr<ai_player>(ai));
}

void game::remove_player(const std::string& name)
{
	for(int n = 0; n != players_.size(); ++n) {
		if(players_[n].name == name) {
			players_.erase(players_.begin() + n);
			for(int m = 0; m != ai_.size(); ++m) {
				if(ai_[m]->player_id() == n) {
					ai_.erase(ai_.begin() + m);
					break;
				}
			}

			break;
		}
	}
}

std::vector<std::string> game::get_ai_players() const
{
	std::vector<std::string> result;
	foreach(boost::shared_ptr<ai_player> a, ai_) {
		ASSERT_LOG(a->player_id() >= 0 && a->player_id() < players_.size(), "BAD AI INDEX: " << a->player_id());
		result.push_back(players_[a->player_id()].name);
	}

	return result;
}

int game::get_player_index(const std::string& nick) const
{
	int nplayer = 0;
	foreach(const player& p, players_) {
		if(p.name == nick) {
			foreach(boost::shared_ptr<ai_player> ai, ai_) {
				if(ai->player_id() == nplayer) {
					return -1;
				}
			}
			return nplayer;
		}

		++nplayer;
	}

	return -1;
}

void game::send_game_state(int nplayer)
{
	if(nplayer == -1) {
		++state_id_;

		for(int n = 0; n != players().size(); ++n) {
			send_game_state(n);
		}

		//Send to observers.
		queue_message(write(-1));
		outgoing_messages_.back().recipients.push_back(-1);

		current_message_ = "";
	} else if(nplayer >= 0 && nplayer < players().size() && players()[nplayer].is_human) {
		queue_message(write(nplayer), nplayer);
	}
}

void game::ai_play()
{
	for(int n = 0; n != ai_.size(); ++n) {
		for(;;) {
			variant msg = ai_[n]->play();
			if(msg.is_null()) {
				break;
			}

			handle_message(ai_[n]->player_id(), msg);
		}
	}
}

void game::set_message(const std::string& msg)
{
	current_message_ = msg;
}

variant game::get_value(const std::string& key) const
{
	if(key == "game") {
		return variant(this);
	} else if(key == "doc") {
		return doc_;
	} else if(backup_callable_) {
		return backup_callable_->query_value(key);
	} else {
		return variant();
	}
}

void game::set_value(const std::string& key, const variant& value)
{
	if(key == "doc") {
		doc_ = value;
	} else if(key == "event") {
		if(value.is_string()) {
			handle_event(value.as_string());
		} else if(value.is_map()) {
			handle_event(value["event"].as_string(), map_into_callable(value["arg"]).get());
		}
	} else if(key == "log_message") {
		if(!value.is_null()) {
			log_.push_back(value.as_string());
		}
	} else if(backup_callable_) {
		backup_callable_->mutate_value(key, value);
	}
}

void game::handle_message(int nplayer, const variant& msg)
{
	const std::string type = msg["type"].as_string();
	if(type == "start_game") {
		start_game();
		return;
	} else if(type == "request_updates") {
		return;
	}

	game_logic::map_formula_callable_ptr vars(new game_logic::map_formula_callable);
	vars->add("message", msg);
	vars->add("player", variant(nplayer));
	handle_event("message", vars.get());
	send_game_state();
}

void game::setup_game()
{
}

namespace {
struct backup_callable_scope {
	game_logic::formula_callable** ptr_;
	game_logic::formula_callable* backup_;
	backup_callable_scope(game_logic::formula_callable** ptr, game_logic::formula_callable* var) : ptr_(ptr), backup_(*ptr) {
		if(var) {
			*ptr_ = var;
		} else {
			ptr_ = NULL;
		}
	}

	~backup_callable_scope() {
		if(ptr_) {
			*ptr_ = backup_;
		}
	}
};
}

void game::handle_event(const std::string& name, game_logic::formula_callable* variables)
{
	const backup_callable_scope backup_scope(&backup_callable_, variables);

	std::map<std::string, game_logic::const_formula_ptr>::const_iterator itor = type_.handlers.find(name);
	if(itor == type_.handlers.end() || !itor->second) {
		return;
	}

	variant v = itor->second->execute(*this);
	execute_command(v);
}

void game::execute_command(variant cmd)
{
	if(cmd.is_list()) {
		for(int n = 0; n != cmd.num_elements(); ++n) {
			execute_command(cmd[n]);
		}
	} else if(cmd.is_callable()) {
		const game_logic::command_callable* command = cmd.try_convert<game_logic::command_callable>();
		if(command) {
			command->execute(*this);
		}
	}
}

}
