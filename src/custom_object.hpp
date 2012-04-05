#ifndef CUSTOM_OBJECT_HPP_INCLUDED
#define CUSTOM_OBJECT_HPP_INCLUDED

#include <set>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <stack>

#include "blur.hpp"
#include "color_utils.hpp"
#include "custom_object_type.hpp"
#include "decimal.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_variable_storage.hpp"
#include "light.hpp"
#include "particle_system.hpp"
#include "raster_distortion.hpp"
#include "variant.hpp"

class collision_info;
class level;

struct custom_object_text;

class custom_object : public entity
{
public:
	static std::set<custom_object*>& get_all();
	static std::set<custom_object*>& get_all(const std::string& type);
	static void init();

	explicit custom_object(variant node);
	custom_object(const std::string& type, int x, int y, bool face_right);
	custom_object(const custom_object& o);
	virtual ~custom_object();

	//finish_loading(): called when a level finishes loading all objects,
	//and allows us to do any final setup such as finding our parent.
	void finish_loading();
	virtual variant write() const;
	virtual void setup_drawing() const;
	virtual void draw() const;
	virtual void draw_group() const;
	virtual void process(level& lvl);
	void set_level(level& lvl) { }

	int parallax_scale_millis_x() const {
		if(parallax_scale_millis_.get() == NULL){
			return type_->parallax_scale_millis_x();
		}else{
			return parallax_scale_millis_->first;
		}
	}
	int parallax_scale_millis_y() const {
		if(parallax_scale_millis_.get() == NULL){
			return type_->parallax_scale_millis_y();
		}else{
			return parallax_scale_millis_->second;
		}
	}

	
	virtual int zorder() const;
	virtual int zsub_order() const;

	virtual int velocity_x() const;
	virtual int velocity_y() const;
	virtual int mass() const { return type_->mass(); }

	int teleport_offset_x() const { return type_->teleport_offset_x(); }
	int teleport_offset_y() const { return type_->teleport_offset_y(); }
	bool no_move_to_standing() const { return type_->no_move_to_standing(); };
	bool reverse_global_vertical_zordering() const { return type_->reverse_global_vertical_zordering(); };

	bool has_feet() const;

	
	virtual bool is_standable(int x, int y, int* friction=NULL, int* traction=NULL, int* adjust_y=NULL) const;

	virtual bool destroyed() const;
	virtual bool point_collides(int x, int y) const;
	virtual bool rect_collides(const rect& r) const;

	virtual const frame& current_frame() const { return *frame_; }

	void set_frame(const std::string& name);

	virtual rect draw_rect() const;

	//bare setting of the frame without adjusting position/checking solidity
	//etc etc.
	void set_frame_no_adjustments(const std::string& name);
	void die();
	void die_with_no_event();
	virtual bool is_active(const rect& screen_area) const;
	bool dies_on_inactive() const;
	bool always_active() const;
	bool move_to_standing(level& lvl, int max_displace=10000);

	bool body_harmful() const;
	bool body_passthrough() const;

	int time_in_frame() const { return time_in_frame_; }

	formula_callable* vars() { return vars_.get(); }
	const formula_callable* vars() const { return vars_.get(); }

	int cycle() const { return cycle_; }

	int surface_friction() const;
	int surface_traction() const;

	variant get_child(const std::string& key) const {
		return type_->get_child(key);
	}

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

	void set_text(const std::string& text, const std::string& font, int size, bool centered);

	virtual int hitpoints() const { return hitpoints_; }

	virtual bool boardable_vehicle() const;

	virtual void boarded(level& lvl, const entity_ptr& player);
	virtual void unboarded(level& lvl);

	virtual void board_vehicle();
	virtual void unboard_vehicle();

	void set_driver_position();

	virtual int current_animation_id() const { return current_animation_id_; }

	virtual const_editor_entity_info_ptr editor_info() const;

	virtual void handle_event(const std::string& event, const formula_callable* context=NULL);
	virtual void handle_event(int event, const formula_callable* context=NULL);

	virtual bool serializable() const;

	void set_blur(const blur_info* blur);
	void set_sound_volume(const int volume);
	void set_zsub_order(const int zsub_order) {zsub_order_ = zsub_order;}
	
	bool execute_command(const variant& var);

	bool allow_level_collisions() const;

	//statistic on how many FFL events are handled every second.
	static int events_handled_per_second;

	const std::vector<light_ptr>& lights() const { return lights_; }
	void swap_lights(std::vector<light_ptr>& lights) { lights_.swap(lights); }

	void shift_position(int x, int y);

	bool appears_at_difficulty(int difficulty) const;

	int min_difficulty() const { return min_difficulty_; }
	int max_difficulty() const { return max_difficulty_; }

	void set_difficulty(int min, int max) { min_difficulty_ = min; max_difficulty_ = max; }

	void update_type(const_custom_object_type_ptr old_type,
	                 const_custom_object_type_ptr new_type);

protected:
	//components of per-cycle process() that can be done even on
	//static objects.
	void static_process(level& lvl);

	virtual void control(const level& lvl);
	variant get_value(const std::string& key) const;
	variant get_value_by_slot(int slot) const;
	void set_value(const std::string& key, const variant& value);
	void set_value_by_slot(int slot, const variant& value);

	//function which indicates if the object wants to walk up or down stairs.
	//-1 = up stairs, 0 = no change, 1 = down stairs
	virtual int walk_up_or_down_stairs() const { return 0; }

	bool is_underwater() const {
		return was_underwater_;
	}

	const std::pair<int,int>* parallax_scale_millis() const { return parallax_scale_millis_.get(); }

	enum STANDING_STATUS { NOT_STANDING, STANDING_BACK_FOOT, STANDING_FRONT_FOOT };
	STANDING_STATUS is_standing(const level& lvl, collision_info* info=NULL) const;

	void set_parent(entity_ptr e, const std::string& pivot_point);

	virtual int parent_depth(int cur_depth=0) const;

	virtual bool editor_force_standing() const;

	entity_ptr standing_on() const { return standing_on_; }

	virtual rect platform_rect_at(int xpos) const;

	virtual bool solid_platform() const;


private:
	custom_object& operator=(const custom_object& o);
	struct Accessor;

	bool move_to_standing_internal(level& lvl, int max_displace);

	void process_frame();

	const_solid_info_ptr calculate_solid() const;
	const_solid_info_ptr calculate_platform() const;

	virtual void get_inputs(std::vector<game_logic::formula_input>* inputs) const;

	int slope_standing_on(int range) const;

	int previous_y_;

	variant custom_type_;
	const_custom_object_type_ptr type_; //the type after variations are applied
	const_custom_object_type_ptr base_type_; //the type without any variation
	std::vector<std::string> current_variation_;
	const frame* frame_;
	std::string frame_name_;
	int time_in_frame_;
	int time_in_frame_delta_;

	int velocity_x_, velocity_y_;
	int accel_x_, accel_y_;
	int gravity_shift_;
	decimal rotate_;

	boost::scoped_ptr<std::pair<int, int> > parallax_scale_millis_;

	int zorder_;
	int zsub_order_;
	
	int hitpoints_, max_hitpoints_;
	bool was_underwater_;

	bool has_feet_;

	int invincible_;
	
	int sound_volume_;	//see sound.cpp; valid values are 0-128, note that this affects all sounds spawned by this object

	game_logic::const_formula_ptr next_animation_formula_;

	game_logic::formula_variable_storage_ptr vars_, tmp_vars_;
	game_logic::map_formula_callable_ptr tags_;

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

	boost::shared_ptr<decimal> draw_scale_;
	boost::shared_ptr<rect> draw_area_, activation_area_, clip_area_;
	int activation_border_;
	
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

	int last_cycle_active_;

	struct position_schedule {
		position_schedule() : speed(1) {}
		int speed;
		std::vector<int> x_pos;
		std::vector<int> y_pos;
		std::vector<decimal> rotation;
	};

	boost::scoped_ptr<position_schedule> position_schedule_;

	std::vector<light_ptr> lights_;

	boost::scoped_ptr<rect> platform_area_;
	const_solid_info_ptr platform_solid_info_;

	point parent_position() const;

	//storage of the parent object while we're loading the object still.
	variant parent_loading_;

	entity_ptr parent_;
	std::string parent_pivot_;
	int parent_prev_x_, parent_prev_y_;
	bool parent_prev_facing_;

	int min_difficulty_, max_difficulty_;

	boost::shared_ptr<const std::vector<frame::CustomPoint> > custom_draw_;

	void set_platform_area(const rect& area);

	std::vector<int> platform_offsets_;
};

#endif
