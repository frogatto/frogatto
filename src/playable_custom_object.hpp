#ifndef PLAYABLE_CUSTOM_OBJECT_HPP_INCLUDED
#define PLAYABLE_CUSTOM_OBJECT_HPP_INCLUDED

#include "custom_object.hpp"
#include "key.hpp"
#include "player_info.hpp"
#include "wml_node_fwd.hpp"

class level;

class playable_custom_object : public custom_object
{
public:
	playable_custom_object(const custom_object& obj);
	playable_custom_object(const playable_custom_object& obj);
	playable_custom_object(wml::const_node_ptr node);

	virtual wml::node_ptr write() const;

	virtual player_info* is_human() { return &player_info_; }
	virtual const player_info* is_human() const { return &player_info_; }

	void save_game();
	entity_ptr save_condition() const { return save_condition_; }

	virtual entity_ptr backup() const;
	virtual entity_ptr clone() const;

	virtual bool look_up() const;
	virtual bool look_down() const;

private:
	virtual void process(level& lvl);
	variant get_value(const std::string& key) const;	

	player_info player_info_;

	entity_ptr save_condition_;

	void operator=(const playable_custom_object);
};

#endif
