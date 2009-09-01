#ifndef CHARACTER_HPP_INCLUDED
#define CHARACTER_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "boost/intrusive_ptr.hpp"

#include "character_type.hpp"
#include "color_utils.hpp"
#include "controls.hpp"
#include "entity.hpp"
#include "formula_callable.hpp"
#include "formula_fwd.hpp"
#include "geometry.hpp"
#include "key.hpp"
#include "player_info.hpp"
#include "powerup_fwd.hpp"
#include "raster_distortion.hpp"
#include "wml_node_fwd.hpp"

class level;
class character;
class pc_character;

typedef boost::intrusive_ptr<character> character_ptr;
typedef boost::intrusive_ptr<const character> const_character_ptr;
typedef boost::intrusive_ptr<pc_character> pc_character_ptr;
typedef boost::intrusive_ptr<const pc_character> const_pc_character_ptr;

class character : public entity
{
public:
	explicit character(wml::const_node_ptr node);
	character(const std::string& type, int x, int y, bool face_right);
	virtual ~character();
	level* get_level() { return lvl_; }
	void set_level(level* lvl);
	virtual wml::node_ptr write() const;
	virtual void setup_drawing() const;
	virtual void draw() const;
	void draw_group() const;
	void process(level& lvl);
	bool is_standing(const level& lvl, int* friction=NULL, int* traction=NULL, int* damage=NULL, int* adjust_y=NULL, entity_ptr* standing_on=NULL) const;
	int collide_left() const;
	int collide_right() const;

	void walk(const level& lvl, bool move_right);
	void run(const level& lvl, bool move_right);
	void fly(const level& lvl, bool move_right, int lift);
	void jump(const level& lvl);
	void jump_down(const level& lvl);
	void crouch(const level& lvl);
	void uncrouch(const level& lvl);
	void lookup(const level& lvl);
	void unlookup(const level& lvl);
	bool attack(const level& lvl, bool down_key_pressed=false);
	void roll(const level& lvl);

	int velocity_x() const { return velocity_x_; }
	int velocity_y() const { return velocity_y_; }
	void set_velocity(int x, int y) { velocity_x_ = x; velocity_y_ = y; }
	virtual int mass() const { return type_->mass(); }

	bool point_collides(int x, int y) const;
	bool rect_collides(const rect& r) const;
	void hit_by(entity& e);
	void move_to_standing(level& lvl);
	int hitpoints() const;
	int max_hitpoints() const;
	void heal() { hitpoints_ = max_hitpoints_; }
	int walk_speed() const;
	int jump_power() const;
	int boost_power() const;
	int glide_speed() const;
	void get_hit();

	bool is_standable(int x, int y, int* friction=NULL, int* traction=NULL, int* adjust_y=NULL) const;
	void stood_on_by(const entity_ptr& ch) { standing_on_.push_back(ch); }

	bool enter() const;
	virtual bool look_up() const { return false; }
	virtual bool look_down() const { return false; }

	bool destroyed() const;
	int invincible() const { return invincible_; }

	int formula_test_frequency() const { return formula_test_frequency_; }

	void set_face_right(bool facing);

	int time_in_frame() const { return time_in_frame_; }

	int springiness() const;
	bool spring_off_head(const entity& jumped_on_by);

	int current_animation_id() const { return frame_id_; }

	void boarded(level& lvl, const entity_ptr& player);
	void unboarded(level& lvl);

	void board_vehicle();
	void unboard_vehicle();

	virtual int weight() const;

	entity_ptr driver();
	const_entity_ptr driver() const;

	const frame& portrait_frame() const;
	const frame& name_frame() const;
	const frame& icon_frame() const;

	int num_powerups() const;
	virtual void get_powerup(const std::string& id);
	virtual void get_powerup(const_powerup_ptr powerup);
	virtual void remove_powerup();
	virtual int remove_powerup(const_powerup_ptr powerup);
	const std::vector<const_powerup_ptr>& powerups() const { return powerups_; }
	const std::vector<const_powerup_ptr>& abilities() const { return abilities_; }

	void preload_powerup(const_powerup_ptr powerup);
	bool is_powerup_loaded(const_powerup_ptr powerup) const;

	virtual void generate_current(const entity& target, int* velocity_x, int* velocity_y) const;

	virtual entity_ptr backup() const;
	virtual entity_ptr clone() const;

	void set_invisible(bool value) { invisible_ = value; }

	std::string debug_description() const;
	const character_type& type() const { return *type_; }

protected:
	const frame& current_frame() const;
	void set_last_jump(bool val) { last_jump_ = val; }

	int cycle() const { return cycle_num_; }
	virtual void set_value(const std::string& key, const variant& value);

	virtual bool body_passthrough() const;
	virtual bool body_harmful() const;

	virtual bool boardable_vehicle() const;
	virtual void swap_player_state(pc_character& player) {}

	game_logic::const_formula_ptr get_event_handler(const std::string& key) const;
	void set_event_handler(const std::string& key, game_logic::const_formula_ptr f);

	void map_entities(const std::map<entity_ptr, entity_ptr>& m);

protected:
	bool is_in_swimming_frame() const;
	void change_frame(const frame* new_frame);

	int rotate() const { return rotate_; }
	void set_rotate(int rotate) { rotate_ = rotate; }

	int current_traction() const { return current_traction_; }

	void change_to_stand_frame();
private:

	void set_driver_position();

	void try_to_make_standing();
	int slope_standing_on(int range=1) const;

	bool can_continue_sliding() const;
	bool in_stand_frame() const;

	virtual int invincibility_duration() const { return 10; }
	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;
	virtual void control(const level& lvl);

	void handle_event(const std::string& event_id);
	void execute_formula(const game_logic::const_formula_ptr& f);
	void execute_command(const variant& var);
	const_character_type_ptr type_;
	const_character_type_ptr base_type_;

	//types that we are keeping around until the next frame, to make sure
	//they don't get collected, because we might still be using their members.
	std::vector<const_character_type_ptr> old_types_;
	int previous_y_;
	int velocity_x_;
	int velocity_y_;
	int invincible_;
	int rotate_;
	level* lvl_;
	game_logic::const_formula_ptr walk_formula_, jump_formula_, fly_formula_;
	game_logic::const_formula_ptr xpos_formula_, ypos_formula_;
	int formula_test_frequency_;
	int time_since_last_formula_;
	int walk_result_, jump_result_, fly_result_;
	bool collided_since_last_frame_;
	int time_in_frame_;
	const frame* current_frame_;
	int hitpoints_;
	int max_hitpoints_;
	int walk_speed_;
	int jump_power_;
	int boost_power_;
	int glide_speed_;
	int cycle_num_;

	int current_traction_;

	std::vector<entity_ptr> standing_on_;

	std::map<std::string, variant> vars_;

	entity_ptr driver_;

	bool last_jump_;
	int last_walk_;

	//ID incremented every time we make a new move. This is mostly used
	//so that we can make sure an enemy isn't hit multiple times in
	//a single animation.
	int frame_id_;

	int loop_sound_;

	//if set to true, the character is currently interacting, and will be
	//locked in the interacting frame.
	bool interacting_;
	bool invisible_;

	//blur information
	struct previous_draw {
		const frame* frame_drawn;
		int x, y;
		bool face_right;
		int time_in_frame;
		int alpha;
		int blur;
		int slope;
	};

	mutable std::vector<previous_draw> blur_;

	std::vector<const_powerup_ptr> powerups_;
	std::vector<const_powerup_ptr> abilities_;

	//manages the memory of the distortion; doesn't do anything else.
	mutable graphics::const_raster_distortion_ptr distortion_;

	std::map<std::string, game_logic::const_formula_ptr> event_handlers_;

	void make_draw_color();
	boost::shared_ptr<graphics::color_transform> draw_color_;
};

class pc_character : public character {
public:
	explicit pc_character(wml::const_node_ptr node);

	pc_character(const pc_character& c);
	explicit pc_character(const character& c) : character(c), player_info_(*this)
	{}

	virtual ~pc_character() {}

	virtual wml::node_ptr write() const;

	virtual void draw() const;
	virtual player_info* is_human() { return &player_info_; }
	virtual const player_info* is_human() const { return &player_info_; }

	virtual bool look_up() const;
	virtual bool look_down() const;

	void save_game();
	entity_ptr save_condition() const { return save_condition_; }

	void set_key_state(const std::string& keys) { key_.Read(keys); }

	void record_stats_movement();

	virtual entity_ptr backup() const;
	virtual entity_ptr clone() const;

	void respawn_player();
private:

	virtual void set_value(const std::string& key, const variant& value);
	virtual int invincibility_duration() const { return 150; }
	virtual void control(const level& lvl);
	void swimming_control(const level& lvl);
	CKey key_;

	bool prev_left_, prev_right_;
	int last_left_, last_right_;
	bool running_;

	entity_ptr save_condition_;

	//the player's spawn location, in case they need respawning (in mp games)
	int spawn_x_, spawn_y_;

	//position last time we recorded a stats sample
	point last_stats_position_;

	player_info player_info_;

	void operator=(const pc_character&);
};

#endif
