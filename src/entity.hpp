#ifndef ENTITY_HPP_INCLUDED
#define ENTITY_HPP_INCLUDED

#include <string>

#include "boost/intrusive_ptr.hpp"

#include "controls.hpp"
#include "current_generator.hpp"
#include "editor_variable_info.hpp"
#include "entity_fwd.hpp"
#include "formula_callable.hpp"
#include "formula_fwd.hpp"
#include "geometry.hpp"
#include "key.hpp"
#include "light.hpp"
#include "solid_map_fwd.hpp"
#include "wml_formula_callable.hpp"
#include "variant.hpp"

namespace game_logic {
class formula_callable_definition;
}

class character;
class frame;
class level;
class pc_character;
class player_info;

typedef boost::intrusive_ptr<character> character_ptr;

class entity : public game_logic::wml_serializable_formula_callable
{
public:
	static entity_ptr build(variant node);
	explicit entity(variant node);
	entity(int x, int y, bool face_right);
	virtual ~entity() {}

	virtual void add_to_level();

	virtual void finish_loading() {}
	virtual variant write() const = 0;
	virtual void setup_drawing() const {}
	virtual void draw(int x, int y) const = 0;
	virtual void draw_group() const = 0;
	player_info* get_player_info() { return is_human(); }
	const player_info* get_player_info() const { return is_human(); }
	virtual const player_info* is_human() const { return NULL; }
	virtual player_info* is_human() { return NULL; }
	virtual void process(level& lvl);
	virtual bool execute_command(const variant& var) = 0;

	const std::string& label() const { return label_; }
	void set_label(const std::string& lb) { label_ = lb; }
	void set_distinct_label();

	virtual void shift_position(int x, int y) { x_ += x*100; y_ += y*100; prev_feet_x_ += x; prev_feet_y_ += y; calculate_solid_rect(); }
	
	void set_pos(const point& p) { x_ = p.x*100; y_ = p.y*100; calculate_solid_rect(); }
	void set_pos(int x, int y) { x_ = x*100; y_ = y*100; calculate_solid_rect(); }
	void set_x(int x) { x_ = x*100; calculate_solid_rect(); }
	void set_y(int y) { y_ = y*100; calculate_solid_rect(); }

	void set_centi_x(int x) { x_ = x; calculate_solid_rect(); }
	void set_centi_y(int y) { y_ = y; calculate_solid_rect(); }
	virtual void set_zsub_order(const int zsub_order) = 0;

	int x() const { return x_/100 - (x_ < 0 && x_%100 ? 1 : 0); }
	int y() const { return y_/100 - (y_ < 0 && y_%100 ? 1 : 0); }

	virtual int parallax_scale_millis_x() const { return 1000;}
	virtual int parallax_scale_millis_y() const { return 1000;}
		
	virtual int zorder() const { return 0; }
	virtual int zsub_order() const { return 0; }

	virtual const std::pair<int,int>* parallax_scale_millis() const { return 0; }

	int centi_x() const { return x_; }
	int centi_y() const { return y_; }

	virtual int velocity_x() const { return 0; }
	virtual int velocity_y() const { return 0; }

	int group() const { return group_; }
	void set_group(int group) { group_ = group; }

	virtual bool is_standable(int x, int y, int* friction=NULL, int* traction=NULL, int* adjust_y=NULL) const { return false; }

	virtual bool destroyed() const = 0;

	virtual int surface_friction() const { return 0; }
	virtual int surface_traction() const { return 0; }

	virtual bool point_collides(int x, int y) const = 0;
	virtual bool rect_collides(const rect& r) const = 0;
	const solid_info* platform() const { return platform_.get(); }
	const solid_info* solid() const { return solid_.get(); }
	const rect& solid_rect() const { return solid_rect_; }
	const rect& frame_rect() const { return frame_rect_; }
	rect platform_rect() const { return platform_rect_; }
	virtual rect platform_rect_at(int xpos) const { return platform_rect(); }
	virtual int platform_slope_at(int xpos) const { return 0; }
	virtual bool solid_platform() const { return false; }
	rect body_rect() const;
	rect hit_rect() const;
	point midpoint() const;

	virtual const frame& icon_frame() const = 0;
	virtual const frame& current_frame() const = 0;

	virtual rect draw_rect() const = 0;

	bool is_alpha(int xpos, int ypos) const;

	virtual bool has_feet() const;
	int feet_x() const;
	int feet_y() const;

	int last_move_x() const;
	int last_move_y() const;

	void set_platform_motion_x(int value);
	int platform_motion_x() const;
	int map_platform_pos(int xpos) const;

	bool face_right() const { return face_right_; }
	virtual void set_face_right(bool facing);

	bool upside_down() const { return upside_down_; }
	virtual void set_upside_down(bool facing);

	int face_dir() const { return face_right() ? 1 : -1; }

	virtual bool body_harmful() const { return true; }

	virtual int time_in_frame() const = 0;

	virtual int teleport_offset_x() const { return 0; }
	virtual int teleport_offset_y() const { return 0; }
	virtual bool no_move_to_standing() const { return 0; }
	virtual bool reverse_global_vertical_zordering() const { return 0; }

	virtual entity_ptr standing_on() const = 0;

	virtual void die_with_no_event() = 0;
	virtual bool is_active(const rect& screen_area) const = 0;
	virtual bool dies_on_inactive() const { return false; } 
	virtual bool always_active() const { return false; } 
	
	virtual formula_callable* vars() { return NULL; }
	virtual const formula_callable* vars() const { return NULL; }

	virtual bool body_passthrough() const { return false; }

	//the number of pixels up or down to adjust the scroll position if this
	//object is focused.
	virtual int vertical_look() const { return 0; }

	void set_id(int id) { id_ = id; }
	int get_id() const { return id_; }

	bool respawn() const { return respawn_; }

	virtual bool boardable_vehicle() const { return false; }
	virtual void boarded(level& lvl, const entity_ptr& player) {}
	virtual void unboarded(level& lvl) {}

	virtual void board_vehicle() {}
	virtual void unboard_vehicle() {}

	virtual void set_sound_volume(const int volume) = 0;
	virtual int weight() const { return 1; }
	
	virtual int mass() const = 0;

	void draw_debug_rects() const;

#ifndef NO_EDITOR
	virtual const_editor_entity_info_ptr editor_info() const { return const_editor_entity_info_ptr(); }
#endif // !NO_EDITOR

	virtual entity_ptr clone() const { return entity_ptr(); }
	virtual entity_ptr backup() const = 0;

	virtual void generate_current(const entity& target, int* velocity_x, int* velocity_y) const;

	virtual game_logic::const_formula_ptr get_event_handler(int key) const { return game_logic::const_formula_ptr(); }
	virtual void set_event_handler(int, game_logic::const_formula_ptr f) { return; }

	virtual bool handle_event(const std::string& id, const formula_callable* context=NULL) { return false; }
	virtual bool handle_event(int id, const formula_callable* context=NULL) { return false; }
	virtual bool handle_event_delay(int id, const formula_callable* context=NULL) { return false; }
	virtual void resolve_delayed_events() = 0;

	//function which returns true if this object can be 'interacted' with.
	//i.e. if the player ovelaps with the object and presses up if they will
	//talk to or enter the object.
	virtual bool can_interact_with() const { return false; }

	virtual bool serializable() const { return true; }

	virtual std::string debug_description() const = 0;

	//a function call which tells us to get any references to other entities
	//that we hold, and map them according to the mapping given. This is useful
	//when we back up an entire level and want to make references match.
	virtual void map_entities(const std::map<entity_ptr, entity_ptr>& m) {}
	virtual void cleanup_references() {}

	void add_scheduled_command(int cycle, variant cmd);
	std::vector<variant> pop_scheduled_commands();

	virtual void save_game() {}

	virtual entity_ptr driver() { return entity_ptr(); }
	virtual const_entity_ptr driver() const { return const_entity_ptr(); }

	virtual bool move_to_standing(level& lvl, int max_displace=10000) { return false; }
	virtual int hitpoints() const { return 1; }
	virtual int max_hitpoints() const { return 1; }

	void set_control_status(const std::string& key, bool value);
	void set_control_status(controls::CONTROL_ITEM ctrl, bool value) { controls_[ctrl] = value; }
	void clear_control_status() { for(int n = 0; n != controls::NUM_CONTROLS; ++n) { controls_[n] = false; } }

	virtual bool enter() const { return false; }

	virtual void set_invisible(bool value) {}
	virtual void record_stats_movement() {}

	virtual entity_ptr save_condition() const { return entity_ptr(); }
	virtual void respawn_player() {}

	virtual int current_animation_id() const { return 0; }

	virtual void set_level(level* lvl) {}

	unsigned int solid_dimensions() const { return solid_dimensions_; }
	unsigned int collide_dimensions() const { return collide_dimensions_; }

	unsigned int weak_solid_dimensions() const { return weak_solid_dimensions_; }
	unsigned int weak_collide_dimensions() const { return weak_collide_dimensions_; }

	void set_attached_objects(const std::vector<entity_ptr>& v);

	virtual bool allow_level_collisions() const { return false; }

	virtual const std::vector<light_ptr>& lights() const = 0;
	virtual void swap_lights(std::vector<light_ptr>& lights) = 0;

	point pivot(const std::string& name, bool reverse_facing=false) const;

	virtual bool appears_at_difficulty(int difficulty) const = 0;

	virtual int parent_depth(bool* has_human_parent=NULL, int cur_depth=0) const { return 0; }

	virtual bool editor_force_standing() const = 0;

	void set_spawned_by(const std::string& key);
	const std::string& spawned_by() const;

	virtual const bool mouse_event_swallowed() {return false;}

	bool is_mouse_over_entity() const { return mouse_over_entity_; }
	void set_mouse_over_entity(bool val=true) { mouse_over_entity_=val; }
	void set_mouse_buttons(Uint8 buttons) { mouse_button_state_ = buttons; }
	Uint8 get_mouse_buttons() const { return mouse_button_state_; }
	bool is_being_dragged() const { return being_dragged_; }
	void set_being_dragged(bool val=true) { being_dragged_ = val; }
	virtual bool get_clip_area(rect* clip_area) = 0;

	virtual const game_logic::formula_callable_definition* get_definition() const = 0;

	virtual void create_object() = 0;

	virtual bool use_absolute_screen_coordinates() const = 0;

protected:
	virtual const_solid_info_ptr calculate_solid() const = 0;
	virtual const_solid_info_ptr calculate_platform() const = 0;
	void calculate_solid_rect();

	bool control_status(controls::CONTROL_ITEM ctrl) const { return controls_[ctrl]; }
	void read_controls(int cycle);

	void set_current_generator(current_generator* generator);

	void set_respawn(bool value) { respawn_ = value; }

	//move the entity by a number of centi pixels. Returns true if its
	//position is changed.
	bool move_centipixels(int dx, int dy);

	void set_solid_dimensions(unsigned int dim, unsigned int weak) { solid_dimensions_ = dim; weak_solid_dimensions_ = dim|weak; }
	void set_collide_dimensions(unsigned int dim, unsigned int weak) { collide_dimensions_ = dim; weak_collide_dimensions_ = dim|weak; }

	const std::vector<entity_ptr>& attached_objects() const { return attached_objects_; }

	virtual void control(const level& lvl) = 0;

	variant serialize_to_wml() const { return write(); }

	int prev_feet_x() const { return prev_feet_x_; }
	int prev_feet_y() const { return prev_feet_y_; }


private:
	std::string label_;

	int x_, y_;

	int prev_feet_x_, prev_feet_y_;
	int last_move_x_, last_move_y_;

	bool face_right_;
	bool upside_down_;

	//the entity group the entity is in.
	int group_;

	int id_;

	bool respawn_;

	bool mouse_over_entity_;
	Uint8 mouse_button_state_;
	bool being_dragged_;

	unsigned int solid_dimensions_, collide_dimensions_;
	unsigned int weak_solid_dimensions_, weak_collide_dimensions_;

	current_generator_ptr current_generator_;

	typedef std::pair<int, variant> ScheduledCommand;
	std::vector<ScheduledCommand> scheduled_commands_;

	bool controls_[controls::NUM_CONTROLS];	

	//attached objects are objects which are also drawn with this object.
	//attached objects should generally NOT be present in the level, and are
	//NOT processed independently of this object.
	std::vector<entity_ptr> attached_objects_;

	//caches of commonly queried rects.
	rect solid_rect_, frame_rect_, platform_rect_, prev_platform_rect_;
	const_solid_info_ptr solid_;
	const_solid_info_ptr platform_;

	int platform_motion_x_;

	std::string spawned_by_;
};

bool zorder_compare(const entity_ptr& e1, const entity_ptr& e2);	
struct entity_zorder_compare
{
	bool operator()(const entity_ptr& lhs, const entity_ptr& rhs);
};

#endif
