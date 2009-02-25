#ifndef CUSTOM_OBJECT_HPP_INCLUDED
#define CUSTOM_OBJECT_HPP_INCLUDED

#include <inttypes.h>

#include "custom_object_type.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "raster_distortion.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

class level;

class custom_object : public entity
{
public:
	explicit custom_object(wml::const_node_ptr node);
	custom_object(const std::string& type, int x, int y, bool face_right);
	virtual ~custom_object() {}
	virtual wml::node_ptr write() const;
	virtual void setup_drawing() const;
	virtual void draw() const;
	virtual void draw_group() const;
	virtual void process(level& lvl);

	virtual int zorder() const;

	virtual int velocity_x() const;
	virtual int velocity_y() const;

	virtual bool is_standable(int x, int y, int* friction=NULL, int* adjust_y=NULL) const;

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

private:
	virtual void control(const level& lvl);

	bool is_standing(const level& lvl) const;

	variant get_value(const std::string& key) const;
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
	int rotate_;

	int hitpoints_;
	bool was_underwater_;

	game_logic::const_formula_ptr next_animation_formula_;

	level* lvl_;

	game_logic::map_formula_callable_ptr vars_;

	entity_ptr last_hit_by_;
	int last_hit_by_anim_;

	int cycle_;

	std::map<std::string, game_logic::const_formula_ptr> event_handlers_;

	std::vector<entity_ptr> stood_on_by_;

	graphics::const_raster_distortion_ptr distortion_;

	union {
	uint8_t draw_color_[4];
	uint32_t draw_color_int_;
	};
};

#endif
