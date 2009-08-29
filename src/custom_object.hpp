#ifndef CUSTOM_OBJECT_HPP_INCLUDED
#define CUSTOM_OBJECT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <inttypes.h>

#include "color_utils.hpp"
#include "custom_object_type.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "particle_system.hpp"
#include "raster_distortion.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

class level;

struct custom_object_text;

class custom_object : public entity
{
public:
	explicit custom_object(wml::const_node_ptr node);
	custom_object(const std::string& type, int x, int y, bool face_right);
	virtual ~custom_object();
	virtual wml::node_ptr write() const;
	virtual void setup_drawing() const;
	virtual void draw() const;
	virtual void draw_group() const;
	virtual void process(level& lvl);
	void set_level(level& lvl) { lvl_ = &lvl; }

	virtual int zorder() const;

	virtual int velocity_x() const;
	virtual int velocity_y() const;
	virtual int mass() const { return type_->mass(); }
	
	virtual bool is_standable(int x, int y, int* friction=NULL, int* traction=NULL, int* adjust_y=NULL) const;

	virtual void stood_on_by(const entity_ptr& ch);
	virtual bool destroyed() const;
	virtual bool point_collides(int x, int y) const;

	virtual bool on_players_side() const;

	virtual const frame& current_frame() const { return *frame_; }

	void set_frame(const std::string& name);
	void die();
	bool dies_on_inactive() const;
	bool always_active() const;
	void hit_player();
	void hit_by(entity& e);

	bool body_harmful() const;
	bool body_passthrough() const;
	int springiness() const;
	bool spring_off_head(const entity& landed_on_by);

	int time_in_frame() const { return time_in_frame_; }

	formula_callable* vars() { return vars_.get(); }
	const formula_callable* vars() const { return vars_.get(); }

	int cycle() const { return cycle_; }

	wml::const_node_ptr get_child(const std::string& key) const {
		return type_->get_child(key);
	}

	const frame& portrait_frame() const;
	const frame& icon_frame() const;

	virtual entity_ptr clone() const;
	virtual entity_ptr backup() const;

	game_logic::const_formula_ptr get_event_handler(const std::string& key) const;
	void set_event_handler(const std::string& key, game_logic::const_formula_ptr f);

	bool can_interact_with() const;

	std::string debug_description() const;

	void map_entities(const std::map<entity_ptr, entity_ptr>& m);

	void add_particle_system(const std::string& key, const std::string& type);
	void remove_particle_system(const std::string& key);

	void set_text(const std::string& text, const std::string& font);

	virtual int hitpoints() const { return hitpoints_; }

	virtual bool boardable_vehicle() const;

	virtual void boarded(level& lvl, const entity_ptr& player);
	virtual void unboarded(level& lvl);

	virtual void board_vehicle();
	virtual void unboard_vehicle();

	void set_driver_position();

	virtual int current_animation_id() const { return current_animation_id_; }

protected:
	virtual void control(const level& lvl);
	variant get_value(const std::string& key) const;

private:
	bool is_standing(const level& lvl) const;

	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;
	void set_value(const std::string& key, const variant& value);

	void handle_event(const std::string& event, const formula_callable* context=NULL);
	void execute_command(const variant& var);

	int previous_y_;

	wml::const_node_ptr custom_type_;
	const_custom_object_type_ptr type_;
	const frame* frame_;
	std::string frame_name_;
	int time_in_frame_;

	int velocity_x_, velocity_y_;
	int accel_x_, accel_y_;
	int rotate_;

	int zorder_;

	int hitpoints_;
	bool was_underwater_;

	game_logic::const_formula_ptr next_animation_formula_;

	level* lvl_;

	game_logic::map_formula_callable_ptr vars_;

	entity_ptr last_hit_by_;
	int last_hit_by_anim_;
	int current_animation_id_;

	int cycle_;

	std::map<std::string, game_logic::const_formula_ptr> event_handlers_;

	std::vector<entity_ptr> stood_on_by_;

	graphics::const_raster_distortion_ptr distortion_;

	void make_draw_color();
	const graphics::color_transform& draw_color() const;
	boost::shared_ptr<graphics::color_transform> draw_color_;

	bool can_interact_with_;

	std::map<std::string, particle_system_ptr> particle_systems_;

	typedef boost::shared_ptr<custom_object_text> custom_object_text_ptr;
	custom_object_text_ptr text_;

	entity_ptr driver_;
};

#endif
