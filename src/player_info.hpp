#ifndef PLAYER_INFO_HPP_INCLUDED
#define PLAYER_INFO_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "wml_node_fwd.hpp"

//class which contains information about the player.
class player_info
{
public:
	explicit player_info(entity& e) : entity_(&e), score_(0), slot_(0)
	{}
	player_info(entity& e, wml::const_node_ptr node);
	
	void item_destroyed(const std::string& level_id, int item);
	const std::vector<int>& get_items_destroyed(const std::string& level_id) const;

	void object_destroyed(const std::string& level_id, int item);
	const std::vector<int>& get_objects_destroyed(const std::string& level_id) const;

	int score(int points) { score_ += points; return score_; }
	int score() const { return score_; }

	void write(wml::node_ptr node) const;

	void swap_player_state(player_info& player) {
		items_destroyed_.swap(player.items_destroyed_);
		objects_destroyed_.swap(player.objects_destroyed_);
	}

	const entity& get_entity() const { return *entity_; }
	entity& get_entity() { return *entity_; }

	void set_entity(entity& e) { entity_ = &e; }

	const std::string& current_level() const { return current_level_; }
	void set_current_level(const std::string& lvl) { current_level_ = lvl; }

	void set_player_slot(int slot) { slot_ = slot; }

	void read_controls(int cycle);

private:
	entity* entity_;

	mutable std::map<std::string, std::vector<int> > items_destroyed_;
	mutable std::map<std::string, std::vector<int> > objects_destroyed_;
	
	int score_;

	//the number of the player.
	int slot_;

	std::string current_level_;
};

#endif
