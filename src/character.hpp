#ifndef CHARACTER_HPP_INCLUDED
#define CHARACTER_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "boost/intrusive_ptr.hpp"

#include "character_type.hpp"
#include "entity.hpp"
#include "formula_callable.hpp"
#include "formula_fwd.hpp"
#include "geometry.hpp"
#include "key.hpp"
#include "powerup_fwd.hpp"
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
	virtual void draw() const;
	void draw_group() const;
	void draw_portrait(int x, int y) const;
	void process(level& lvl);
	bool is_standing(const level& lvl, int* friction=NULL, int* damage=NULL, int* adjust_y=NULL, entity_ptr* standing_on=NULL) const;
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
	void attack(const level& lvl, bool down_key_pressed=false);
	void roll(const level& lvl);

	int velocity_x() const { return velocity_x_; }
	int velocity_y() const { return velocity_y_; }
	void set_velocity(int x, int y) { velocity_x_ = x; velocity_y_ = y; }

	bool point_collides(int x, int y) const;
	void hit_by(entity& e);
	void move_to_standing(level& lvl);
	int hitpoints() const;
	int max_hitpoints() const;
	int walk_speed() const;
	int jump_power() const;
	int boost_power() const;
	int glide_speed() const;
	void get_hit();

	bool is_standable(int x, int y, int* friction=NULL, int* adjust_y=NULL) const;
	void stood_on_by(const entity_ptr& ch) { standing_on_.push_back(ch); }

	bool enter() const { return look_up() || look_down(); }
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

	void boarded(level& lvl, character_ptr player);
	void unboarded(level& lvl);

	virtual int weight() const;

	pc_character_ptr& driver() { return driver_; }
	const_pc_character_ptr driver() const { return driver_; }

	const frame& portrait_frame() const;
	const frame& name_frame() const;
	const frame& icon_frame() const;

	int num_powerups() const;
	virtual void get_powerup(const std::string& id);
	virtual void get_powerup(const_powerup_ptr powerup);
	virtual void remove_powerup();
	virtual int remove_powerup(const_powerup_ptr powerup);
	const std::vector<const_powerup_ptr>& powerups() const { return powerups_; }

protected:
	const frame& current_frame() const;
	const character_type& type() const { return *type_; }
	void set_last_jump(bool val) { last_jump_ = val; }

	int cycle() const { return cycle_num_; }
	virtual void set_value(const std::string& key, const variant& value);

	virtual bool body_passthrough() const;
	virtual bool body_harmful() const;

	virtual bool boardable_vehicle() const;
	virtual void swap_player_state(pc_character& player) {}
private:

	void set_driver_position();

	void try_to_make_standing();
	int slope_standing_on(int range=1) const;

	bool can_continue_sliding() const;
	void change_to_stand_frame();
	bool in_stand_frame() const;

	virtual int invincibility_duration() const { return 10; }
	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;
	virtual void control(const level& lvl);
	void change_frame(const frame* new_frame);

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

	std::vector<entity_ptr> standing_on_;

	std::map<std::string, variant> vars_;

	pc_character_ptr driver_;

	bool last_jump_;

	//ID incremented every time we make a new move. This is mostly used
	//so that we can make sure an enemy isn't hit multiple times in
	//a single animation.
	int frame_id_;

	int loop_sound_;

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
};

class pc_character : public character {
public:
	explicit pc_character(wml::const_node_ptr node);

	explicit pc_character(character& c) : character(c), score_(0)
	{}

	virtual ~pc_character() {}

	virtual wml::node_ptr write() const;

	virtual void draw() const;
	virtual pc_character* is_human() { return this; }
	virtual const pc_character* is_human() const { return this; }

	virtual bool look_up() const;
	virtual bool look_down() const;

	void item_destroyed(const std::string& level_id, int item);
	const std::vector<int>& get_items_destroyed(const std::string& level_id) const;

	void object_destroyed(const std::string& level_id, int item);
	const std::vector<int>& get_objects_destroyed(const std::string& level_id) const;

	void save_game();
	boost::intrusive_ptr<pc_character> save_condition() const { return save_condition_; }
	const std::string& current_level() const { return current_level_; }
	void set_current_level(const std::string& lvl) { current_level_ = lvl; }

	void set_key_state(const std::string& keys) { key_.Read(keys); }
	virtual void swap_player_state(pc_character& player) {
		items_destroyed_.swap(player.items_destroyed_);
		objects_destroyed_.swap(player.objects_destroyed_);
	}

	int score(int points) { score_ += points; return score_; }
	int score() const { return score_; }
private:

	virtual void set_value(const std::string& key, const variant& value);
	virtual int invincibility_duration() const { return 150; }
	virtual void control(const level& lvl);
	CKey key_;

	bool prev_left_, prev_right_;
	int last_left_, last_right_;
	bool running_;

	mutable std::map<std::string, std::vector<int> > items_destroyed_;
	mutable std::map<std::string, std::vector<int> > objects_destroyed_;

	boost::intrusive_ptr<pc_character> save_condition_;
	std::string current_level_;

	int score_;
};

#endif
