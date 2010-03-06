#ifndef CUSTOM_OBJECT_HPP_INCLUDED
#define CUSTOM_OBJECT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <inttypes.h>
#include <stack>

#include "blur.hpp"
#include "color_utils.hpp"
#include "custom_object_type.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_variable_storage.hpp"
#include "particle_system.hpp"
#include "raster_distortion.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

class collision_info;
class level;

struct custom_object_text;

class custom_object : public entity
{
public:
	static void init();

	explicit custom_object(wml::const_node_ptr node);
	custom_object(const std::string& type, int x, int y, bool face_right);
	custom_object(const custom_object& o);
	virtual ~custom_object();
	virtual wml::node_ptr write() const;
	virtual void setup_drawing() const;
	virtual void draw() const;
	virtual void draw_group() const;
	virtual void process(level& lvl);
	void set_level(level& lvl) { }

	virtual int zorder() const;

	virtual int velocity_x() const;
	virtual int velocity_y() const;
	virtual int mass() const { return type_->mass(); }

	int teleport_offset_x() const { return type_->teleport_offset_x(); }
	int teleport_offset_y() const { return type_->teleport_offset_y(); }

	bool has_feet() const;
	
	virtual bool is_standable(int x, int y, int* friction=NULL, int* traction=NULL, int* adjust_y=NULL) const;

	virtual bool destroyed() const;
	virtual bool point_collides(int x, int y) const;
	virtual bool rect_collides(const rect& r) const;

	virtual bool on_players_side() const;

	virtual const frame& current_frame() const { return *frame_; }

	void set_frame(const std::string& name);

	virtual rect draw_rect() const;

	//bare setting of the frame without adjusting position/checking solidity
	//etc etc.
	void set_frame_no_adjustments(const std::string& name);
	void die();
	virtual bool is_active(const rect& screen_area) const;
	bool dies_on_inactive() const;
	bool always_active() const;
	void hit_player();
	void hit_by(entity& e);
	void move_to_standing(level& lvl);

	bool body_harmful() const;
	bool body_passthrough() const;

	int time_in_frame() const { return time_in_frame_; }

	formula_callable* vars() { return vars_.get(); }
	const formula_callable* vars() const { return vars_.get(); }

	int cycle() const { return cycle_; }

	int surface_friction() const;
	int surface_traction() const;

	wml::const_node_ptr get_child(const std::string& key) const {
		return type_->get_child(key);
	}

	const frame& portrait_frame() const;
	const frame& icon_frame() const;

	virtual entity_ptr clone() const;
	virtual entity_ptr backup() const;

	game_logic::const_formula_ptr get_event_handler(int key) const;
	void set_event_handler(int, game_logic::const_formula_ptr f);

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

	virtual const_editor_entity_info_ptr editor_info() const;

	void handle_event(const std::string& event, const formula_callable* context=NULL);
	void handle_event(int event, const formula_callable* context=NULL);

	void set_blur(const blur_info* blur);
	void set_sound_volume(const int volume);
	
	bool execute_command(const variant& var);

	//statistic on how many FFL events are handled every second.
	static int events_handled_per_second;

protected:
	virtual void control(const level& lvl);
	variant get_value(const std::string& key) const;
	variant get_value_by_slot(int slot) const;
	void set_value(const std::string& key, const variant& value);
	void set_value_by_slot(int slot, const variant& value);

	const std::pair<int,int>* position_scale_millis() const { return position_scale_millis_.get(); }

private:
	custom_object& operator=(const custom_object& o);
	struct Accessor;

	void process_frame();

	const_solid_info_ptr calculate_solid() const;
	const_solid_info_ptr calculate_platform() const;

	bool is_standing(const level& lvl, collision_info* info=NULL) const;

	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;

	int slope_standing_on(int range) const;

	int previous_y_;

	wml::const_node_ptr custom_type_;
	const_custom_object_type_ptr type_; //the type after variations are applied
	const_custom_object_type_ptr base_type_; //the type without any variation
	std::vector<std::string> current_variation_;
	const frame* frame_;
	std::string frame_name_;
	int time_in_frame_;
	int time_in_frame_delta_;

	int velocity_x_, velocity_y_;
	int accel_x_, accel_y_;
	int rotate_;

	boost::scoped_ptr<std::pair<int, int> > position_scale_millis_;

	int zorder_;

	int hitpoints_, max_hitpoints_;
	bool was_underwater_;

	bool has_feet_;

	int invincible_;
	
	int sound_volume_;	//see sound.cpp; valid values are 0-128, note that this affects all sounds spawned by this object

	game_logic::const_formula_ptr next_animation_formula_;

	game_logic::formula_variable_storage_ptr vars_, tmp_vars_;
	game_logic::map_formula_callable_ptr tags_;

	entity_ptr last_jumped_on_by_;
	entity_ptr last_hit_by_;
	int last_hit_by_anim_;
	int current_animation_id_;

	int cycle_;

	//variable which is always set to false on construction, and then the
	//first time process is called will fire the on_load event and set to false
	bool loaded_;

	std::vector<game_logic::const_formula_ptr> event_handlers_;

	entity_ptr standing_on_;

	int standing_on_prev_x_, standing_on_prev_y_;

	graphics::const_raster_distortion_ptr distortion_;

	void make_draw_color();
	const graphics::color_transform& draw_color() const;
	boost::shared_ptr<graphics::color_transform> draw_color_;

	boost::shared_ptr<rect> draw_area_, activation_area_;

	bool can_interact_with_;

	std::map<std::string, particle_system_ptr> particle_systems_;

	typedef boost::shared_ptr<custom_object_text> custom_object_text_ptr;
	custom_object_text_ptr text_;

	entity_ptr driver_;

	boost::shared_ptr<blur_info> blur_;

	//set if we should fall through platforms. This is decremented automatically
	//at the end of every cycle.
	int fall_through_platforms_;

	//current shader we're using to draw with.
	std::vector<std::string> fragment_shaders_, vertex_shaders_;
	mutable GLuint shader_;

	mutable game_logic::map_formula_callable_ptr shader_vars_;

	bool always_active_;

	std::stack<const formula_callable*> backup_callable_stack_;
};

#endif
