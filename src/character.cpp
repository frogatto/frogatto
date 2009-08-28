#include <boost/bind.hpp>
#include <iostream>
#include <limits.h>

#include "character.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "functional.hpp"
#include "joystick.hpp"
#include "level.hpp"
#include "level_logic.hpp"
#include "powerup.hpp"
#include "preferences.hpp"
#include "random.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
int get_accel(int a) { return a == INT_MIN ? 0 : a; }
}

character::character(wml::const_node_ptr node)
  : entity(node),
    type_(character_type::get(node->attr("type"))),
	base_type_(type_),
	previous_y_(y()),
	velocity_x_(wml::get_int(node, "velocity_x")),
	velocity_y_(wml::get_int(node, "velocity_y")),
	invincible_(0),
	rotate_(0),
	lvl_(NULL),
	walk_formula_(game_logic::formula::create_optional_formula(node->attr("walk_formula"))),
	jump_formula_(game_logic::formula::create_optional_formula(node->attr("jump_formula"))),
	fly_formula_(game_logic::formula::create_optional_formula(node->attr("fly_formula"))),
	xpos_formula_(game_logic::formula::create_optional_formula(node->attr("xpos_formula"))),
	ypos_formula_(game_logic::formula::create_optional_formula(node->attr("ypos_formula"))),
	formula_test_frequency_(wml::get_int(node, "formula_test_frequency", 10)),
	time_since_last_formula_(0),
	walk_result_(0), jump_result_(0), fly_result_(0),
	collided_since_last_frame_(false),
	time_in_frame_(0),
	hitpoints_(wml::get_int(node, "hitpoints", type_->hitpoints())),
	max_hitpoints_(wml::get_int(node, "max_hitpoints", type_->hitpoints())),
	walk_speed_(wml::get_int(node, "walk_speed")),
	jump_power_(wml::get_int(node, "jump_power")),
	boost_power_(wml::get_int(node, "boost_power")),
	glide_speed_(wml::get_int(node, "glide_speed")),
	cycle_num_(0), current_traction_(100), last_jump_(false), last_walk_(0),
    frame_id_(0), loop_sound_(-1), interacting_(false), invisible_(false)
{
	set_label(node->attr("label"));

	current_frame_ = &type_->get_frame();
	assert(type_);

	wml::const_node_ptr driver = node->get_child("character");
	if(driver) {
		driver_ = new pc_character(driver);
	}

	foreach(const std::string& p, util::split(node->attr("powerups"))) {
		std::cerr << "get_powerup(" << p << ")\n";
		get_powerup(p);
	}

	foreach(const std::string& p, util::split(node->attr("abilities"))) {
		get_powerup(p);
	}
}

character::character(const std::string& type, int x, int y, bool face_right)
  : entity(x, y, face_right),
    type_(character_type::get(type)),
	base_type_(type_),
	previous_y_(y),
	velocity_x_(0),
	velocity_y_(0),
	rotate_(0),
	lvl_(NULL),
	formula_test_frequency_(1),
	time_since_last_formula_(0),
	walk_result_(0), jump_result_(0), fly_result_(0),
	collided_since_last_frame_(false),
	time_in_frame_(0),
	hitpoints_(type_->hitpoints()),
	max_hitpoints_(type_->hitpoints()),
	walk_speed_(0),
	jump_power_(0),
	boost_power_(0),
	glide_speed_(0),
	cycle_num_(0), current_traction_(100), last_jump_(false), last_walk_(0),
    frame_id_(0), loop_sound_(-1)
{
	current_frame_ = &type_->get_frame();
	assert(type_);
}

pc_character::pc_character(wml::const_node_ptr node)
	  : character(node), prev_left_(true), prev_right_(true),
	    last_left_(-1000), last_right_(-1000), running_(false),
		spawn_x_(x()), spawn_y_(y()),
		player_info_(*this, node)
{
}


character::~character()
{
	if(loop_sound_ >= 0) {
		sound::cancel_looped(loop_sound_);
		loop_sound_ = -1;
	}
}

void character::set_level(level* lvl)
{
	lvl_ = lvl;
	if(driver_) {
		driver_->set_level(lvl);
	}
}

wml::node_ptr character::write() const
{
	wml::node_ptr res(new wml::node("character"));
	res->set_attr("type", type_->id());
	res->set_attr("face_right", face_right() ? "true" : "false");
	res->set_attr("x", formatter() << x());
	res->set_attr("y", formatter() << y());
	res->set_attr("velocity_x", formatter() << velocity_x_);
	res->set_attr("velocity_y", formatter() << velocity_y_);
	res->set_attr("formula_test_frequency", formatter() << formula_test_frequency());
	if(hitpoints_ != type_->hitpoints()) {
		res->set_attr("hitpoints", formatter() << hitpoints_);
	}
	if(max_hitpoints_ != type_->hitpoints()) {
		res->set_attr("max_hitpoints", formatter() << max_hitpoints_);
	}
	if(walk_speed_ != 0) {
		res->set_attr("walk_speed", formatter() << walk_speed_);
	}
	if(jump_power_ != 0) {
		res->set_attr("jump_power", formatter() << jump_power_);
	}
	if(boost_power_ != 0) {
		res->set_attr("boost_power", formatter() << boost_power_);
	}
	if(glide_speed_ != 0) {
		res->set_attr("glide_speed", formatter() << glide_speed_);
	}
	if(walk_formula_) {
		res->set_attr("walk_formula", walk_formula_->str());
	}
	if(jump_formula_) {
		res->set_attr("jump_formula", jump_formula_->str());
	}
	if(fly_formula_) {
		res->set_attr("fly_formula", fly_formula_->str());
	}
	if(xpos_formula_) {
		res->set_attr("xpos_formula", xpos_formula_->str());
	}
	if(ypos_formula_) {
		res->set_attr("ypos_formula", ypos_formula_->str());
	}
	if(is_human()) {
		res->set_attr("is_human", "true");
	}

	if(group() >= 0) {
		res->set_attr("group", formatter() << group());
	}

	if(driver_) {
		res->add_child(driver_->write());
	}

	res->set_attr("powerups", util::join(map_vector<std::string>(powerups_, boost::bind(&powerup::id, _1))));
	res->set_attr("abilities", util::join(map_vector<std::string>(abilities_, boost::bind(&powerup::id, _1))));
	return res;
}

wml::node_ptr pc_character::write() const
{
	wml::node_ptr result = character::write();
	player_info_.write(result);
	return result;
}

void character::setup_drawing() const
{
	using namespace graphics;
	if(type_->radial_distortion()) {
		raster_distortion* distort = new radial_distortion(body_rect().mid_x(), body_rect().mid_y(), type_->radial_distortion(), type_->radial_distortion_intensity());
		distort->set_cycle(cycle_num_);
		distortion_ = raster_distortion_ptr(distort);
		graphics::add_raster_distortion(distortion_.get());
	}
}

void character::draw() const
{
	if(invisible_) {
		return;
	}

	if(driver_) {
		driver_->draw();
	}

	if(draw_color_) {
		draw_color_->to_color().set_as_current_color();
	}

	const int slope = rotate_ + (current_frame().rotate_on_slope() ? -slope_standing_on(5)*face_dir() : 0);
	current_frame().draw(x(), y(), face_right(), false, time_in_frame_, slope);

	//if we blur then back up information about the frame here
	if(current_frame().blur()) {
		blur_.push_back(previous_draw());
		previous_draw& p = blur_.back();
		p.frame_drawn = &current_frame();
		p.x = x();
		p.y = y();
		p.face_right = face_right();
		p.time_in_frame = time_in_frame_;
		p.alpha = 100;
		p.blur = current_frame().blur();
		p.slope = slope;
	}

	//draw any blurred frames
	std::vector<previous_draw>::iterator p = blur_.begin();
	while(p != blur_.end()) {
		std::cerr << "draw blurred frame\n";
		p->alpha = (p->alpha*p->blur)/100;
		if(!draw_color_) {
			glColor4f(1.0, 1.0, 1.0, p->alpha/100.0);
		} else {
			const graphics::color c = draw_color_->to_color();
			glColor4ub(c.r(), c.g(), c.b(), c.a()*(p->alpha/100.0));
		}
		p->frame_drawn->draw(p->x, p->y, p->face_right, false, p->time_in_frame, p->slope);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		if(p->alpha < 5) {
			p = blur_.erase(p);
		} else {
			++p;
		}
	}

	if(draw_color_) {
		static const uint8_t AllWhite[4] = {0xFF, 0xFF, 0xFF, 0xFF};
		glColor4ubv(AllWhite);
	}

	draw_debug_rects();
}

void character::draw_group() const
{
	if(group() >= 0) {
		blit_texture(font::render_text(formatter() << group(), graphics::color_yellow(), 24), x(), y());
	}
}

void pc_character::draw() const
{
	if(is_human() && ((invincible()/5)%2) == 1 && &current_frame() != type().gethit_frame()) {
		return;
	}
	
	character::draw();
}

void character::process(level& lvl)
{
	set_level(&lvl);

	clear_control_status();
	read_controls(lvl.cycle());

	static const std::string ProcessStr = "process";
	handle_event(ProcessStr);

	if(y() > lvl.boundaries().y2()) {
		--hitpoints_;
	}

	previous_y_ = y();

	++cycle_num_;

	variant scheduled_command = get_scheduled_command(lvl.cycle());
	while(!scheduled_command.is_null()) {
		execute_command(scheduled_command);
		scheduled_command = get_scheduled_command(lvl.cycle());
	}

	const int start_x = x();
	const int start_y = y();

	++time_in_frame_;

	const std::string* event = current_frame_->get_event(time_in_frame_);
	if(event) {
		handle_event(*event);
	}

	//check if we're pushing but not pressing left or right, in which case
	//we stop pushing.
	if(last_walk_ == 0 && current_frame_ == type_->push_frame()) {
		change_to_stand_frame();
	}

	//check if we're half way through our crouch in which case we lock
	//the frame in place until uncrouch() is called.
	if(time_in_frame_ == current_frame_->duration()/2 &&
	   (current_frame_ == type_->crouch_frame() ||
	    current_frame_ == type_->lookup_frame())) {
	   --time_in_frame_;
	}

	//see if we're under water.
	const bool is_underwater = lvl.is_underwater(body_rect());
	bool is_swimming = is_in_swimming_frame();
	if(is_swimming && !is_underwater) {
		const static std::string LeaveWaterStr = "leave_water";
		handle_event(LeaveWaterStr);

		change_frame(type_->jump_frame());

		//give a little boost to our vertical velocity as we emerge from
		//the water by increasing it by 50%. This allows us to jump out
		//of water in a reasonable way.
		velocity_y_ += velocity_y_/2;
		if(velocity_y_ < -jump_power()) {
			velocity_y_ = -jump_power();
		}
		is_swimming = false;
	} else if(type().has_swim_frames() && !is_swimming && is_underwater) {
		change_frame(type_->swim_side_idle_frame());
		is_swimming = true;

		const static std::string EnterWaterStr = "enter_water";
		handle_event(EnterWaterStr);
	}

	lvl.get_current(*this, &velocity_x_, &velocity_y_);

	//executed when an animation ends, generally acts by switching to a new animation
	if(time_in_frame_ >= current_frame_->duration() && current_frame_ != type_->jump_frame() && current_frame_ != type_->fall_frame() && current_frame_ != type_->gethit_frame() && current_frame_ != type_->die_frame()) {

		//fire an event to signal the end of this animation.
		handle_event("end_" + current_frame_->id() + "_anim");

		if(current_frame_ == &type_->get_frame()) {
			change_frame((rng::generate()%5) == 0 ? type_->idle_frame() : &type_->get_frame());
		} else if(current_frame_ == type_->stand_up_slope_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->stand_down_slope_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->idle_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->walk_frame()) {
			time_in_frame_ = 0;
			change_to_stand_frame();
		} else if(current_frame_ == type_->push_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->fly_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->crouch_frame() ||
		          current_frame_ == type_->lookup_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->interact_frame()) {
			if(interacting_) {
				--time_in_frame_;
			} else {
				change_to_stand_frame();
			}
		} else if(current_frame_ == type_->attack_frame() ||
		          current_frame_ == type_->up_attack_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->run_attack_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->fall_spin_attack_frame()) {
			time_in_frame_ = 0;
		} else if(current_frame_ == type_->slide_frame()) {
			change_frame(type_->fall_frame());
		} else if(current_frame_ == type_->jump_attack_frame()) {
			change_frame(type_->fall_frame());
		} else if(current_frame_ == type_->spring_frame()) {
			time_in_frame_ = 0;
			change_to_stand_frame();
		} else if(current_frame_ == type_->turn_frame()) {
			change_to_stand_frame();
		} else if(current_frame_ == type_->run_frame()) {
			handle_event("leave_" + current_frame_->id() + "_anim");

			time_in_frame_ = 0;
//			current_frame_ = type_->run_frame();
			change_to_stand_frame();
		} else if(current_frame_ == type_->roll_frame()) {
			handle_event("leave_" + current_frame_->id() + "_anim");

			//see if we collide with anything solid right now
			const bool solid_in_roll = lvl_->solid(body_rect());

			//set to half way through crouching.
			current_frame_ = type_->crouch_frame();
			time_in_frame_ = current_frame_->duration()/2 - 1;

			const bool solid_in_crouch = lvl_->solid(body_rect());
			if(!solid_in_roll && solid_in_crouch) {
				//stopping the roll would cause a collission, so make it continue.
				current_frame_ = type_->roll_frame();
				time_in_frame_ = 0;
			}
		}
		
		if( lvl.is_underwater(body_rect()) && type_->swim_side_idle_frame() ) {
			time_in_frame_ = 0;
		}
	}

	if((current_frame_ == type_->walk_frame() || current_frame_ == type_->run_frame()) && velocity_x_/100 == 0) {
		change_to_stand_frame();
	}
	
	last_walk_ = 0;
	control(lvl);

	collided_since_last_frame_ = false;

	int adjust_y_pos = 0;
	const bool started_standing = is_standing(lvl, NULL, NULL, &adjust_y_pos);
	set_pos(x(), y() + adjust_y_pos);

	if(invincible_) {
		--invincible_;
	}

	int climb = type_->climb();
	for(int n = 0; n != std::abs(velocity_x_/100); ++n) {
		const int dir = velocity_x_/100 > 0 ? 1 : -1;
		int xpos = (dir < 0 ? body_rect().x() : body_rect().x2() - 1) + dir;

		bool collide = false, hit = false;
		const int ybegin = y() + current_frame().collide_y();
		const int yend = y() + current_frame().collide_y() + current_frame().collide_h();
		for(int ypos = ybegin; ypos != yend; ++ypos) {
			int damage = 0;
			if(lvl.solid(xpos, ypos, NULL, NULL, invincible_ ? NULL : &damage)) {
				if(!damage) {
					collide = true;
					break;
				} else {
					hit = true;
					break;
				}
			}
		}

		if(!hit) {
			entity_ptr collide_with = lvl.collide(rect(xpos, ybegin, 1, yend - ybegin), this);
			if(collide_with) {
				if(collide_with->body_harmful() && is_human()) {
					if(!invincible_) {
						hit = true;
					}
				} else {
					collide = true;
				}
			}
		}

		if(hit) {
			velocity_x_ = -dir*200;
			velocity_y_ = -400;
			hit = true;
			get_hit();
			break;
		}

		if(hit) {
			break;
		}

		if(collide) {
			if((current_frame_ == type_->fall_frame() || current_frame_ == type_->jump_frame()) && type_->slide_frame() && velocity_y_ <= 100 && (velocity_x_ > 0) == face_right() && std::abs(velocity_x_) > 300) {
				change_frame(type_->slide_frame());
				velocity_y_ = 0;
			} else if(current_frame_ == type_->walk_frame() && type_->push_frame()) {
				std::cerr << "PUSH\n";
				change_frame(type_->push_frame());
			}

			velocity_x_ = 0;
			collided_since_last_frame_ = true;
			break;
		}

		const int start_pos = y();

		set_pos(x() + dir, y());

		int adjust_y = 0;
 		set_pos(x(), y()-1);
 		while(is_standing(lvl, NULL, NULL, &adjust_y)) {
 			set_pos(x(), y()-1);
 		}

 		set_pos(x(), y()+1);
 		set_pos(x(), y()+adjust_y);

		if(started_standing) {
			try_to_make_standing();
		}
	}

	for(int n = 0; n < velocity_y_/100; ++n) {
		int adjust_y = 0;
		if(is_standing(lvl, NULL, NULL, NULL, &adjust_y)) {
			velocity_y_ = 0;
			set_pos(x(), y() + adjust_y);
			break;
		}

		//bounce off someone's head
		entity_ptr c = lvl.collide(feet_x() - type_->feet_width(), feet_y(), this);
		if(!c) {
			c = lvl.collide(feet_x() + type_->feet_width(), feet_y(), this);
		}

		if(c && c->spring_off_head(*this)) {
			if(c->springiness() > 0) {
				velocity_y_ = -c->springiness()*13;
				if(c->velocity_y() < 0) {
					velocity_y_ += c->velocity_y();
				}
			}

			//make sure we don't end up bouncing over and over without control
			//while getting hit by making bouncing off someone's head turn us
			//into the jump frame, which we have control over.
			if(current_frame_ == type_->gethit_frame()) {
				change_frame(type_->jump_frame());
			}
		}

		//see if we're boarding a vehicle
		if(!type_->is_vehicle()) {
			c = lvl.board(feet_x(), feet_y());
			if(c) {
				c->boarded(lvl, this);
				return;
			}
		}

		set_pos(x(), y() + 1);
	}

	for(int n = 0; n < -velocity_y_/100; ++n) {
		const int ypos = y() + current_frame().collide_y() - 1;
		bool collide = false, hit = false;
		const int left = collide_left();
		const int right = collide_right();
		for(int xpos = left; xpos < right; ++xpos) {
			int damage = 0;
			if(lvl.solid(xpos, ypos, NULL, NULL, invincible_ ? NULL : &damage)) {
				if(!damage) {
					collide = true;
					break;
				} else {
					hit = true;
				}
			}
		}

		if(!hit) {
			entity_ptr collide_with = lvl.collide(body_rect(), this);
			if(collide_with) {
				if(collide_with->body_harmful() && is_human()) {
					if(!invincible_) {
						hit = true;
					}
				} else {
					collide = true;
				}
			}
		}

		if(hit) {
			hit = true;
			get_hit();
			break;
		}

		if(!is_human() && !invincible_ && !boardable_vehicle()) {
			entity_ptr player = lvl.hit_by_player(body_rect());
			if(player) {
				set_face_right(!player->face_right());
				hit = true;
				get_hit();
				break;
			}
		}

		if(hit) {
			break;
		}

		if(collide) {
			collided_since_last_frame_ = true;
			velocity_y_ = 0;
			break;
		}

		set_pos(x(), y() - 1);
	}

	//adjust position if colliding with terrain
	{
		const int dir = face_right() ? 1 : -1;
		int x1 = current_frame().collide_x();
		if(dir > 0) {
			x1 += x();
		} else {
			x1 = x() + current_frame().width() - x1;
		}

		int x2 = x1 + current_frame().collide_w()*dir;
		if(x1 > x2) {
			std::swap(x1, x2);
		}

		const int ypos = y() + current_frame().collide_y() + current_frame().collide_h();
		const bool standing = is_standing(*lvl_);
		if(lvl.solid(x1,ypos) || !boardable_vehicle() && lvl.collide(x1, ypos)) {
			set_pos(x() + 1, y());
			if(started_standing) {
				try_to_make_standing();
			}
		}

		if(lvl.solid(x2,ypos) || !boardable_vehicle() && lvl.collide(x2, ypos)) {
			set_pos(x() - 1, y());
			if(started_standing) {
				try_to_make_standing();
			}
		}

		if(standing && !is_standing(*lvl_)) {
			std::cerr << "NOT STANDING!!!\n";
		}
	}

	if(is_human() && !invincible_) {
		entity_ptr collide_with = lvl_->collide(body_rect(), this);
		if(collide_with && collide_with->body_harmful()) {
			get_hit();
		}
	}

	if(!is_human() && !invincible_ && !boardable_vehicle()) {
		entity_ptr player = lvl.hit_by_player(body_rect());
		if(player) {
			set_face_right(!player->face_right());
			get_hit();
		}
	}

	int accel_x = get_accel(current_frame().accel_x());
	if(!face_right()) {
		accel_x *= -1;
	}

	int friction = 0;
	current_traction_ = 100;
	int damage = 0;
	entity_ptr standing_on;
	const bool standing = is_standing(lvl, &friction, &current_traction_, &damage, NULL, &standing_on);

	velocity_x_ += (accel_x*current_traction_)/100;

	if(is_underwater) {
		friction += lvl.water_resistance();
		velocity_y_ = (velocity_y_*(100-friction))/100;
	} else {
		friction += lvl.air_resistance();
		friction = (friction * type_->traction())/100;
	}

	velocity_x_ = (velocity_x_*(100-friction))/100;
	if(damage && !invincible()) {
		get_hit();
	} else if(standing && velocity_y_ >= 0) {
		if(standing_on) {
			standing_on->stood_on_by(this);
		}

		if(current_frame_ == type_->jump_frame() || current_frame_ == type_->fall_frame() || current_frame_ == type_->fall_spin_attack_frame() || current_frame_ == type_->gethit_frame() || current_frame_ == type_->slide_frame() || current_frame_ == type_->jump_attack_frame()) {
			change_to_stand_frame();
		}
	} else if(in_stand_frame() || current_frame_ == type_->walk_frame() ||
	          current_frame_ == type_->run_frame() ||
			  current_frame_ == type_->idle_frame() ||
			  velocity_y_ + get_accel(current_frame().accel_y()) > 0 &&
			  !is_swimming &&
			  current_frame_ != type_->jump_attack_frame() &&
			  current_frame_ != type_->fall_frame() &&
			  current_frame_ != type_->fall_spin_attack_frame() &&
			  current_frame_ != type_->gethit_frame() &&
			  current_frame_ != type_->fly_frame() &&
			  (current_frame_ != type_->slide_frame() ||
			   !can_continue_sliding())) {
		if(type_->fall_frame()) {
			change_frame(type_->fall_frame());
		}
	}

	velocity_y_ += get_accel(current_frame().accel_y());

	const int dx = x() - start_x;
	const int dy = y() - start_y;
	if(dx || dy) {
		foreach(const entity_ptr& ch, standing_on_) {
			ch->set_pos(point(ch->x() + dx, ch->y() + dy));
		}
	}

	standing_on_.clear();

	set_driver_position();
}

bool character::is_in_swimming_frame() const
{
	if(!type().has_swim_frames()) {
		return false;
	}

	const frame* swim_frames[] = {type().swim_side_idle_frame(),
	                              type().swim_up_idle_frame(),
	                              type().swim_down_idle_frame(),
	                              type().swim_side_frame(),
	                              type().swim_up_frame(),
	                              type().swim_down_frame()};
	return std::count(swim_frames, swim_frames + sizeof(swim_frames)/sizeof(swim_frames[0]), &current_frame());
}

void character::set_driver_position()
{
	if(driver_) {
		const int pos_right = x() + type_->passenger_x();
		const int pos_left = x() + current_frame().width() - driver_->current_frame().width() - type_->passenger_x();
		driver_->set_face_right(face_right());

		if(current_frame_ == type_->turn_frame()) {
			int weight_left = time_in_frame_;
			int weight_right = current_frame_->duration() - time_in_frame_;
			if(face_right()) {
				std::swap(weight_left, weight_right);
			}
			const int pos = (pos_right*weight_right + pos_left*weight_left)/current_frame_->duration();
			driver_->set_pos(pos, y() + type_->passenger_y());
		} else {
			driver_->set_pos(face_right() ? pos_right : pos_left, y() + type_->passenger_y());
		}
	}
}

void character::try_to_make_standing()
{
	const int MaxStep = 3;
	int max_down = MaxStep;

	while(!is_standing(*lvl_) && --max_down) {
		set_pos(x(), y()+1);
	}

	if(!is_standing(*lvl_)) {
		set_pos(x(), y() - MaxStep);

		int max_up = MaxStep;
		while(!is_standing(*lvl_) && --max_up) {
			set_pos(x(), y()-1);
		}

		if(!is_standing(*lvl_)) {
			set_pos(x(), y() + MaxStep);
		}
	}
}

bool character::is_standing(const level& lvl, int* friction, int* traction, int* damage, int* adjust_y, entity_ptr* standing_on) const
{
	return lvl.standable(feet_x()-type_->feet_width(), feet_y(), friction, traction, damage, adjust_y, standing_on, this) ||
	       lvl.standable(feet_x()+type_->feet_width(), feet_y(), friction, traction, damage, adjust_y, standing_on, this);
}

bool character::destroyed() const
{
	return hitpoints_ <= 0 && (current_frame_ != type_->die_frame() || time_in_frame_ >= current_frame_->duration());
}

void character::set_face_right(bool facing)
{
	if(facing == face_right()) {
		return;
	}

	if(is_standing(*lvl_) || current_frame_ == type_->fly_frame()) {
		change_frame(type_->turn_frame());
	}

	const int original_pos = feet_x();
	entity::set_face_right(facing);
	const int diff_x = feet_x() - original_pos;
	set_pos(x() - diff_x, y());
}

int character::springiness() const
{
	return type_->springiness();
}

bool character::spring_off_head(const entity& jumped_on_by)
{
	const int weight = jumped_on_by.weight();
	if(weight > 1) {
		hitpoints_  -= (weight - 1);
	}
	if(type_->spring_frame()) {
		change_frame(type_->spring_frame());
	}

	if(hitpoints_ <= 0) {
		lvl_->add_character(entity_ptr(new custom_object("dust_cloud", x()-50, feet_y()-54, true)));
	}

	return hitpoints_ > 0;
}

void character::boarded(level& lvl, const character_ptr& player)
{
	pc_character* pc_player = dynamic_cast<pc_character*>(player.get());
	if(pc_player == NULL) {
		return;
	}

	player->invincible_ = 0;
	player->current_frame_ = &player->type_->get_frame();
	pc_character_ptr new_player(new pc_character(*this));
	new_player->driver_ = pc_player;

	if(type_->loop_sound().empty() == false) {
		new_player->loop_sound_ = sound::play_looped(type_->loop_sound());
	}
	lvl.add_player(new_player);
	hitpoints_ = 0;

	new_player->get_player_info()->swap_player_state(*pc_player->get_player_info());
}

void character::unboarded(level& lvl)
{
	if(loop_sound_ >= 0) {
		sound::cancel_looped(loop_sound_);
		loop_sound_ = -1;
	}
	character_ptr vehicle(new character(*this));
	vehicle->driver_ = pc_character_ptr();
	lvl.add_character(vehicle);
	lvl.add_player(driver_);
	if(vehicle->velocity_x() > 100) {
		driver_->set_face_right(false);
	}
	if(vehicle->velocity_x() < -100) {
		driver_->set_face_right(true);
	}
	driver_->set_velocity(600 * (driver_->face_right() ? 1 : -1), -600);

	if(pc_character* pc = dynamic_cast<pc_character*>(this)) {
		driver_->get_player_info()->swap_player_state(*pc->get_player_info());
	}
}

int character::weight() const
{
	return type_->weight();
}

int character::collide_left() const
{
	if(face_right()) {
		return x() + current_frame().collide_x();
	} else {
		return x() + current_frame().width() - current_frame().collide_x() - current_frame().collide_w();
	}
}

int character::collide_right() const
{
	if(face_right()) {
		return x() + current_frame().collide_x() + current_frame().collide_w();
	} else {
		return x() + current_frame().width() - current_frame().collide_x();
	}
}

void character::walk(const level& lvl, bool move_right)
{
	last_walk_ = move_right ? 1 : -1;

	if(current_frame_ == type_->slide_frame() || current_frame_ == type_->spring_frame() || current_frame_ == type_->die_frame() || current_frame_ == type_->turn_frame() || current_frame_ == type_->gethit_frame()) {
		return;
	}

	if((look_up() || look_down()) && is_standing(lvl)) {
		return;
	}

	const bool standing = is_standing(lvl);
	set_face_right(move_right);
	const int run_bonus = current_frame_ == type_->run_frame() ? 2 : 1;
	velocity_x_ += (standing ? (walk_speed()*run_bonus*current_traction_)/100 : glide_speed())*(move_right ? 1 : -1);
	if(standing && current_frame_ != type_->walk_frame() && type_->walk_frame() && current_frame_ != type_->jump_frame() && current_frame_ != type_->turn_frame() && current_frame_ != type_->run_frame()) {

		//see if we are pushing into something, in which case we go into a
		//pushing frame.
		const rect& body = body_rect();
		const int x = move_right ? (body.x2() + 1) : (body.x() - 1);
		const int y = (body.y() + body.y2())/2;

		bool collide_entity = false;
		if(is_human()) {
			entity_ptr e = lvl.collide(x, y, this);
			collide_entity = e && !e->body_harmful();
		}

		int damage;
		if(type_->push_frame() && (collide_entity || lvl.solid(x, y, NULL, NULL, &damage) && damage == 0)) {
			if(current_frame_ != type_->push_frame()) {
				change_frame(type_->push_frame());
			}
		} else {
			//we're not pushing, so enter the walking frame.
			change_frame(type_->walk_frame());
		}
	}

}

void character::run(const level& lvl, bool move_right)
{
	if(current_frame_ == type_->walk_frame()) {
		change_frame(type_->run_frame());
	}
}

void character::fly(const level& lvl, bool move_right, int lift)
{
	set_face_right(move_right);
	velocity_x_ += glide_speed()*(move_right ? 1 : -1);
	velocity_y_ += lift;

	if(current_frame_ != type_->fly_frame() && current_frame_ != type_->turn_frame() && current_frame_ != type_->spring_frame()) {
		change_frame(type_->fly_frame());
	}
}

void character::jump(const level& lvl)
{
	entity_ptr platform;
	if(!last_jump_ && current_frame_ == type_->slide_frame()) {
		set_face_right(!face_right());
		velocity_x_ += glide_speed()*(face_right() ? 15 : -15);
		velocity_y_ = (-jump_power()*3)/4;
		if(type_->jump_frame()) {
			change_frame(type_->jump_frame());
		}
	} else if(!last_jump_ && is_standing(lvl, NULL, NULL, NULL, NULL, &platform)) {
		if(platform) {
			velocity_x_ += platform->velocity_x();
			velocity_y_ += platform->velocity_y();
		}
		velocity_y_ = -jump_power();
		if(type_->jump_frame()) {
			change_frame(type_->jump_frame());
		}
	} else if(velocity_y_ < 0) {
		velocity_y_ -= boost_power();
	}
}

void character::jump_down(const level& lvl)
{
	if(driver_) {
		unboarded(*lvl_);
	}

	if(is_standing(lvl)) {
		set_pos(x(), y() + 1);
		if(is_standing(lvl)) {
			set_pos(x(), y() - 1);
		}
	}
}

void character::crouch(const level& lvl)
{
	if(is_standing(lvl) && current_frame_ != type_->crouch_frame()) {
		change_frame(type_->crouch_frame());
	}
}

void character::uncrouch(const level& lvl)
{
	if(time_in_frame_ == current_frame_->duration()/2 - 1) {
		// See if we can stand up without colliding with terrain. If we can't,
		// then don't let us stand.

		current_frame_ = &type().get_frame();

		int damage = 0;
		if(lvl.solid(body_rect(), NULL, NULL, &damage) && damage == 0) {
			// Looks like this will make us collide, so don't let us
			// uncrouch.
			current_frame_ = type().crouch_frame();
			return;
		}

		current_frame_ = type().crouch_frame();

		++time_in_frame_;
	}
}

void character::lookup(const level& lvl)
{
	if(!is_standing(lvl)) {
		return;
	}

	if(type_->interact_frame() && current_frame_ != type_->interact_frame() && lvl_->can_interact(body_rect())) {
		change_frame(type_->interact_frame());
	}
	
	if(type_->lookup_frame() && current_frame_ != type_->lookup_frame() && current_frame_ != type_->interact_frame()) {
		change_frame(type_->lookup_frame());
	}
}

void character::unlookup(const level& lvl)
{
	if(time_in_frame_ == current_frame_->duration()/2 - 1) {
		++time_in_frame_;
	}
}

void character::roll(const level& lvl)
{
	change_frame(type_->roll_frame());
}

bool character::attack(const level& lvl, bool down_key_pressed)
{
	static const std::string Event = "attack_pressed";
	handle_event(Event);
	if(is_standing(lvl)) {
		if(type_->run_attack_frame() && current_frame_ == type_->run_frame()) {
			change_frame(type_->run_attack_frame());
			return true;
		} else if(type_->up_attack_frame() && look_up()) {
			change_frame(type_->up_attack_frame());
			return true;
		} else if(type_->attack_frame()) {
			change_frame(type_->attack_frame());
			return true;
		} else {
			return false;
		}
	} else if(current_frame_ == type_->jump_frame() || current_frame_ == type_->fall_frame()) {
		if( type_->fall_spin_attack_frame() && down_key_pressed){
			change_frame(type_->fall_spin_attack_frame());
			return true;
		} else if(type_->jump_attack_frame()) {
			change_frame(type_->jump_attack_frame());
			return true;
		} else {
			return false;
		}
	}

	return false;
}

entity_ptr character::driver() { return driver_; }
const_entity_ptr character::driver() const { return driver_; }

const frame& character::portrait_frame() const
{
	return type_->portrait_frame();
}

const frame& character::name_frame() const
{
	return type_->name_frame();
}

const frame& character::icon_frame() const
{
	return type_->icon_frame();
}

int character::num_powerups() const
{
	return powerups_.size();
}

void character::get_powerup(const std::string& id)
{
	const_powerup_ptr p = powerup::get(id);
	if(p) {
		int ticks = SDL_GetTicks();
		get_powerup(p);
		std::cerr << "POWERUP: " << (SDL_GetTicks() - ticks) << "\n";
	} else {
		std::cerr << "POWERUP '" << id << "' NOT FOUND!\n";
	}
}

void character::get_powerup(const_powerup_ptr p)
{
	if(p->is_permanent()) {
		
		//check if we already have it - if so don't apply it.
		std::vector<const_powerup_ptr>::iterator itor = std::find(abilities_.begin(), abilities_.end(), p);
		if(itor == abilities_.end()) {
			old_types_.push_back(type_);
			old_types_.push_back(base_type_);
			type_ = type_->get_modified(p->id(), p->modifier());
			base_type_ = base_type_->get_modified(p->id(), p->modifier());
		
			abilities_.insert(itor,p);
		}
		return;
	}

	//if we already have this kind of power up, then add it next
	//to the existing powerup
	std::vector<const_powerup_ptr>::iterator itor = std::find(powerups_.begin(), powerups_.end(), p);
	if(itor != powerups_.end()) {
		powerups_.insert(itor, p);
		return;
	}

	blur_.clear();
	powerups_.push_back(p);
	old_types_.push_back(type_);
	type_ = base_type_->get_modified(p->id(), p->modifier());
	change_to_stand_frame();
}

void character::remove_powerup()
{
	if(powerups_.empty() == false) {
		handle_event("remove_powerup");
		blur_.clear();
		powerups_.pop_back();
		old_types_.push_back(type_);
		if(powerups_.empty()) {
			type_ = base_type_;
		} else {
			type_ = base_type_->get_modified(powerups_.back()->id(), powerups_.back()->modifier());
		}
		change_to_stand_frame();
	}
}

int character::remove_powerup(const_powerup_ptr powerup)
{
	handle_event("remove_powerup");

	const int result = std::count(powerups_.begin(), powerups_.end(), powerup);
	powerups_.erase(std::remove(powerups_.begin(), powerups_.end(), powerup), powerups_.end());

	old_types_.push_back(type_);
	//set the new current powerup
	if(powerups_.empty()) {
		type_ = base_type_;
	} else {
		type_ = base_type_->get_modified(powerups_.back()->id(), powerups_.back()->modifier());
	}
	change_to_stand_frame();
	return result;
}

void character::generate_current(const entity& target, int* velocity_x, int* velocity_y) const
{
	if(type_->current_generator()) {
		const rect& my_rect = body_rect();
		const rect& target_rect = target.body_rect();
		type_->current_generator()->generate(my_rect.mid_x(), my_rect.mid_y(),
		                                     target_rect.mid_x(), target_rect.mid_y(), target.mass(),
		                                     velocity_x, velocity_y);
	}

	entity::generate_current(target, velocity_x, velocity_y);
}

const frame& character::current_frame() const
{
	return *current_frame_;
}

bool character::can_continue_sliding() const
{
	const rect& r = body_rect();

	// see if there is a solid within reasonable range that we are clinging to.
	const int xpos = face_right() ? r.x2() + 5 : r.x() - 5;
	const int ypos = r.y2();
	return lvl_->solid(xpos, ypos);
}

void character::change_to_stand_frame()
{
	if(type_->stand_up_slope_frame() && type_->stand_down_slope_frame()) {
		const int slope = slope_standing_on();
		if(slope < 0) {
			change_frame(type_->stand_down_slope_frame());
			return;
		}

		if(slope > 0) {
			change_frame(type_->stand_up_slope_frame());
			return;
		}
	}

	change_frame(&type_->get_frame());
}

int character::slope_standing_on(int range) const
{
	if(lvl_ == NULL || !is_standing(*lvl_)) {
		return 0;
	}

	const int forward = face_right() ? 1 : -1;
	const int xpos = feet_x();
	int ypos = feet_y();


	for(int n = 0; !lvl_->solid(xpos, ypos) && n != 10; ++n) {
		++ypos;
	}

	if(range == 1) {
		if(lvl_->solid(xpos + forward, ypos - 1) &&
		   !lvl_->solid(xpos - forward, ypos)) {
			return 45;
		}

		if(!lvl_->solid(xpos + forward, ypos) &&
		   lvl_->solid(xpos - forward, ypos - 1)) {
			return -45;
		}

		return 0;
	} else {
		if(!is_standing(*lvl_)) {
			return 0;
		}

		int y1 = find_ground_level(*lvl_, xpos + forward*range, ypos, range+1);
		int y2 = find_ground_level(*lvl_, xpos - forward*range, ypos, range+1);
		while((y1 == INT_MIN || y2 == INT_MIN) && range > 0) {
			y1 = find_ground_level(*lvl_, xpos + forward*range, ypos, range+1);
			y2 = find_ground_level(*lvl_, xpos - forward*range, ypos, range+1);
			--range;
		}

		if(range == 0) {
			return 0;
		}

		const int dy = y2 - y1;
		const int dx = range*2;
		return (dy*45)/dx;
	}
}

bool character::in_stand_frame() const
{
	return current_frame_ == &type_->get_frame() || current_frame_ == type_->idle_frame() || current_frame_ == type_->stand_up_slope_frame() || current_frame_ == type_->stand_down_slope_frame();
}

void character::change_frame(const frame* new_frame)
{
	if(new_frame == NULL || current_frame_ == type_->die_frame()) {
		return;
	}

	if(current_frame_) {
		handle_event("leave_" + current_frame_->id() + "_anim");
	}

	const frame* old_frame = current_frame_;
	const int old_time_in_frame = time_in_frame_;

	//if we're rolling or crouching we want to disallow changes to frames that
	//might make us collide with solid surfaces etc.
	const bool solid_before_change = current_frame_ != type_->crouch_frame() && current_frame_ != type_->roll_frame() || lvl_->solid(body_rect());

	++frame_id_;

	time_in_frame_ = 0;

	const int start_x = feet_x();
	const int start_y = feet_y();

	current_frame_ = new_frame;

	rotate_ = 0;

	const int diff_x = feet_x() - start_x;
	const int diff_y = feet_y() - start_y;

	set_pos(x() - diff_x, y() - diff_y);

	if(!solid_before_change && lvl_->solid(body_rect())) {
		//changing frame like this causes a collission, so disallow it and roll back.
		current_frame_ = old_frame;
		time_in_frame_ = old_time_in_frame;
		set_pos(x() + diff_x, y() + diff_y);
		return;
	}

	if(new_frame->velocity_x() != INT_MIN) {
		velocity_x_ = new_frame->velocity_x()*(face_right() ? 1 : -1);
	}

	if(new_frame->velocity_y() != INT_MIN) {
		velocity_y_ = new_frame->velocity_y();
	}

	new_frame->play_sound(this);

	handle_event(new_frame->id());

	old_types_.clear();
}

game_logic::const_formula_ptr character::get_event_handler(const std::string& key) const
{
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator itor = event_handlers_.find(key);
	if(itor != event_handlers_.end()) {
		return itor->second;
	}

	return game_logic::const_formula_ptr();
}

void character::set_event_handler(const std::string& key, game_logic::const_formula_ptr f)
{
	std::cerr << "SET EVENT HANDLER: '" << key << "' -> " << (f ? f->str() : "(null)") << "\n";
	if(!f) {
		event_handlers_.erase(key);
	} else {
		event_handlers_[key] = f;
	}
}

void character::map_entities(const std::map<entity_ptr, entity_ptr>& m)
{
	foreach(entity_ptr& e, standing_on_) {
		std::map<entity_ptr, entity_ptr>::const_iterator i = m.find(e);
		if(i != m.end()) {
			e = i->second;
		}
	}
}

void character::handle_event(const std::string& event_id)
{
	game_logic::const_formula_ptr event;
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator event_itor = event_handlers_.find(event_id);
	if(event_itor != event_handlers_.end()) {
		event = event_itor->second;
	}

	if(!event) {
		event = type_->get_event_handler(event_id);
	}

	execute_formula(event);
}

void character::execute_formula(const game_logic::const_formula_ptr& f)
{
	if(f.get() == NULL) {
		return;
	}

	std::cerr << "execute command...\n";

	execute_command(f->execute(*this));
}

void character::execute_command(const variant& var)
{
	if(var.is_null()) { return; }
	if(var.is_list()) {
		for(int n = 0; n != var.num_elements(); ++n) {
			execute_command(var[n]);
		}
	} else {
		entity_command_callable* cmd = var.try_convert<entity_command_callable>();
		if(cmd != NULL) {
			std::cerr << "running!\n";
			cmd->execute(*lvl_, *this);
		}
	}
}

bool character::point_collides(int xpos, int ypos) const
{
	return point_in_rect(point(xpos, ypos), body_rect());
}

void character::hit_by(entity& e)
{
	if(!invincible_) {
		std::cerr << "GET HIT!!!!\n";
		get_hit();
	} else {
		std::cerr << "INVNCIBLE!\n";
	}
}

void character::move_to_standing(level& lvl)
{
	int start_y = y();
	lvl_ = &lvl;
	for(int n = 0; n != 10000; ++n) {
		if(is_standing(lvl)) {

			if(n == 0) {
				for(int n = 0; n != 10000; ++n) {
					set_pos(x(), y() - 1);
					if(!is_standing(lvl)) {
						set_pos(x(), y() + 1);

						if(y() < lvl.boundaries().y()) {
							//we are too high, out of the level. Move the
							//character down, under the solid, and then
							//call this function again to move them down
							//to standing on the solid below.
							for(int n = 0; n != 10000; ++n) {
								set_pos(x(), y() + 1);
								if(!is_standing(lvl)) {
									move_to_standing(lvl);
									return;
								}
							}
						}

						return;
					}
				}
				return;
			}
			return;
		}

		set_pos(x(), y() + 1);
	}

	set_pos(x(), start_y);
	std::cerr << "MOVE_TO_STANDING FAILED\n";
}

int character::hitpoints() const
{
	return hitpoints_;
}

int character::max_hitpoints() const
{
	return max_hitpoints_;
}

int character::walk_speed() const
{
	return type_->walk() + walk_speed_;
}

int character::jump_power() const
{
	return type_->jump() + jump_power_;
}

int character::boost_power() const
{
	return type_->boost() + boost_power_;
}

int character::glide_speed() const
{
	return type_->glide() + glide_speed_;
}

void character::get_hit()
{
	assert(!invincible_);
	remove_powerup();
	if(is_human()) {
		entity_ptr hitby = lvl_->collide(body_rect(), this);
		if(hitby) {
			hitby->hit_player();
		}
	}

	--hitpoints_;
	std::cerr << "GET HIT: " << hitpoints_ << "\n";
	if(hitpoints_ <= 0 && driver_) {
		unboarded(*lvl_);
		if(type_->vehicle_die_object().empty() == false) {
			entity_ptr ep(new custom_object(type_->vehicle_die_object(), x(), y(), face_right()));
			lvl_->add_character(ep);
		}
		return;
	}

	invincible_ = invincibility_duration();

	if(hitpoints_ <= 0 && type_->die_frame()) {
		change_frame(type_->die_frame());

	} else if(type_->gethit_frame()) {
		change_frame(type_->gethit_frame());
	}
}

bool character::is_standable(int xpos, int ypos, int* friction, int* traction, int* adjust_y) const
{
	const frame& f = current_frame();
	if(f.has_platform() == false) {
		return false;
	}

	int y1 = y() + f.platform_y();
	int y2 = previous_y_ + f.platform_y();

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	if(ypos < y1 || ypos > y2) {
		return false;
	}

	if(xpos < x() + f.platform_x() || xpos >= x() + f.platform_x() + f.platform_w()) {
		return false;
	}

	if(friction) {
		*friction = type_->friction();
	}

	if(traction) {
		*traction = 100;
	}

	if(adjust_y) {
		*adjust_y = y() + f.platform_y() - ypos;
	}

	return true;
}

bool character::enter() const
{
	return &current_frame() == type_->interact_frame() && (time_in_frame() == current_frame().duration()/2);
	//slightly ugly hack to make it fire the interact event after the animation
}

variant character::get_value(const std::string& key) const
{
	if(key == "x") {
		return variant(body_rect().x());
	} else if(key == "y") {
		return variant(y());
	} else if(key == "x1") {
		return variant(body_rect().x());
	} else if(key == "y1") {
		return variant(body_rect().y());
	} else if(key == "x2") {
		return variant(body_rect().x2());
	} else if(key == "y2") {
		return variant(body_rect().y2());
	} else if(key == "w") {
		return variant(body_rect().w());
	} else if(key == "h") {
		return variant(body_rect().h());
	} else if(key == "midpoint_x") {
		return variant(body_rect().x() + body_rect().w()/2);
	} else if(key == "midpoint_y") {
		return variant(body_rect().y() + body_rect().h()/2);
	} else if(key == "velocity_x") {
		return variant(velocity_x());
	} else if(key == "velocity_y") {
		return variant(velocity_y());
	} else if(key == "hit_x") {
		return variant(hit_rect().x());
	} else if(key == "hit_y") {
		return variant(hit_rect().y());
	} else if(key == "facing") {
		return variant(face_right() ? 1 : -1);
	} else if(key == "cycle") {
		return variant(cycle_num_);
	} else if(key == "player") {
		if(lvl_ && lvl_->player()) {
			return variant(&lvl_->player()->get_entity());
		} else {
			return variant();
		}
	} else if(key == "vehicle") {
		if(driver_) {
			return variant(this);
		} else {
			return variant();
		}
	} else if(key == "driver") {
		if(driver_) {
			return variant(driver_.get());
		} else {
			return variant(this);
		}
	} else if(key == "collided") {
		return variant(collided_since_last_frame_);
	} else if(key == "near_cliff_edge") {
		return variant(is_standing(*lvl_) &&
		               cliff_edge_within(*lvl_, feet_x(), feet_y(), face_dir()*15));
	} else if(key == "last_walk") {
		return variant(walk_result_);
	} else if(key == "last_jump") {
		return variant(jump_result_);
	} else if(key == "hitpoints") {
		return variant(hitpoints());
	} else if(key == "max_hitpoints") {
		return variant(max_hitpoints());
	} else if(key == "walk_speed") {
		return variant(walk_speed());
	} else if(key == "jump_power") {
		return variant(jump_power());
	} else if(key == "glide_speed") {
		return variant(glide_speed());
	} else if(key == "mass") {
		return variant(type_->mass());
	} else if(key == "damage") {
		return variant(current_frame().damage());
	} else if(key == "level") {
		return variant(lvl_);
	} else if(key == "water_level") {
		rect water_area;
		if(lvl_->is_underwater(body_rect(), &water_area)) {
			return variant(water_area.y());
		} else {
			return variant();
		}
	} else {
		
		std::map<std::string, variant>::const_iterator i = type_->variables().find(key);
		if(i != type_->variables().end()) {
			return i->second;
		}

		i = vars_.find(key);
		if(i != vars_.end()) {
			return i->second;
		}
		return variant();
	}
}

void character::get_inputs(std::vector<game_logic::formula_input>* inputs) const
{
}

void character::set_value(const std::string& key, const variant& value)
{
	if(key == "hitpoints") {
		hitpoints_ = value.as_int();
	} else if(key == "max_hitpoints") {
		max_hitpoints_ = value.as_int();
	} else if(key == "walk_speed") {
		walk_speed_ = value.as_int();
	} else if(key == "jump_power") {
		jump_power_ = value.as_int();
	} else if(key == "boost_power") {
		boost_power_ = value.as_int();
	} else if(key == "glide_speed") {
		glide_speed_ = value.as_int();
	} else if(key == "x") {
		set_pos(value.as_int(), y());
	} else if(key == "y") {
		set_pos(x(), value.as_int());
	} else if(key == "red") {
		make_draw_color();
		draw_color_->buf()[0] = truncate_to_char(value.as_int());
	} else if(key == "green") {
		make_draw_color();
		draw_color_->buf()[1] = truncate_to_char(value.as_int());
	} else if(key == "blue") {
		make_draw_color();
		draw_color_->buf()[2] = truncate_to_char(value.as_int());
	} else if(key == "alpha") {
		make_draw_color();
		draw_color_->buf()[3] = truncate_to_char(value.as_int());
	} else if(key == "interact") {
		interacting_ = value.as_bool();
		std::cerr << "INTERACT: " << (interacting_ ? "TRUE" : "FALSE") << "\n";
	} else if(key == "invisible") {
		invisible_ = value.as_bool();
	} else if(key == "invincible") {
		invincible_ = value.as_int();
	} else {
		vars_[key] = value;
	}
}

void pc_character::set_value(const std::string& key, const variant& value)
{
	if(key == "save") {
		save_game();
	} else {
		character::set_value(key, value);
	}
}

bool character::body_passthrough() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

bool character::body_harmful() const
{
	return !type_->is_vehicle() || driver_.get() != NULL;
}

bool character::boardable_vehicle() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

void character::control(const level& lvl)
{
	if(current_frame_ == type_->die_frame()) {
		return;
	}

	if(type_->is_vehicle() && driver_.get() == NULL) {
		return;
	}

	++time_since_last_formula_;
	if(time_since_last_formula_ == formula_test_frequency_) {
		time_since_last_formula_ = 0;
	}

	if(fly_formula_) {
		if(time_since_last_formula_ == 0) {
			fly_result_ = fly_formula_->execute(*this).as_int();
		}
	}

	if(walk_formula_) {
		if(time_since_last_formula_ == 0) {
			walk_result_ = walk_formula_->execute(*this).as_int();
		}

		if(fly_formula_) {
			fly(lvl, walk_result_ < 0, fly_result_);
		} else if(walk_result_ < 0) {
			walk(lvl, false);
		} else if(walk_result_ > 0) {
			walk(lvl, true);
		}
	}

	if(jump_formula_) {
		if(time_since_last_formula_ == 0) {
			jump_result_ = jump_formula_->execute(*this).as_int();
		}
		if(jump_result_) {
			jump(lvl);
		}
	}

	if(xpos_formula_){
		set_pos(xpos_formula_->execute(*this).as_int(), y());
	}

	if(ypos_formula_){
		set_pos(x(), ypos_formula_->execute(*this).as_int());
	}
}

bool pc_character::look_up() const
{
	return control_status(controls::CONTROL_UP);
}

bool pc_character::look_down() const
{
	return control_status(controls::CONTROL_DOWN);
}

void pc_character::record_stats_movement()
{
	if(!get_level()) {
		return;
	}

	point pos = midpoint();
	if(last_stats_position_ != point() && last_stats_position_ != pos) {
		stats::record_event(get_level()->id(), stats::record_ptr(new stats::player_move_record(last_stats_position_, pos)));
	}
	last_stats_position_ = pos;
}

void pc_character::control(const level& lvl)
{
	if(player_info_.current_level() != lvl.id()) {
		//key_.RequireRelease();
		player_info_.set_current_level(lvl.id());
		last_stats_position_ = midpoint();
	}

	//periodically record a movement sample
	if((cycle()%10) == 0) {
		record_stats_movement();
	}

	if(is_in_swimming_frame()) {
		swimming_control(lvl);
		return;
	}

	if(&current_frame() == type().attack_frame() ||
	   &current_frame() == type().jump_attack_frame() ||
	   &current_frame() == type().up_attack_frame() ||
	   &current_frame() == type().run_attack_frame()) {
		if(&current_frame() == type().run_attack_frame()) {
			set_velocity(velocity_x() + (get_accel(current_frame().accel_x()) * current_traction() * (face_right() ? 1 : -1))/100,
			             velocity_y() + get_accel(current_frame().accel_y()));
			running_ = false;
			return;
		}

		running_ = false;
		return;
	}

	if(running_ && !control_status(controls::CONTROL_LEFT) && !control_status(controls::CONTROL_RIGHT)) {
		running_ = false;
	}


	if(control_status(controls::CONTROL_DOWN)) {
		if (&current_frame() != type().crouch_frame() && &current_frame() != type().roll_frame())
		{
			crouch(lvl);
		}
	} else if(&current_frame() == type().crouch_frame()) {
		uncrouch(lvl);
	}

	if(control_status(controls::CONTROL_UP)) {
		lookup(lvl);
	} else if(&current_frame() == type().lookup_frame()) {
		unlookup(lvl);
	}
	
	if(control_status(controls::CONTROL_JUMP)) {
		if(control_status(controls::CONTROL_DOWN)) {
			jump_down(lvl);
		} else {
			jump(lvl);
		}
		set_last_jump(true);
	} else {
		set_last_jump(false);
	}
	
	if(control_status(controls::CONTROL_ATTACK)) {
		if(&current_frame() == type().crouch_frame()){
			roll(lvl);
			return;
		} else if (&current_frame() != type().roll_frame()) {
			if(attack(lvl, control_status(controls::CONTROL_DOWN))) {
				return;
			}
		}		   
	}

	const int double_tap_cycles = 10;

	if(control_status(controls::CONTROL_LEFT)) {
		walk(lvl, false);
		if(!prev_left_ || running_) {
			if(last_left_ > cycle() - double_tap_cycles || running_) {
				run(lvl, false);
				running_ = true;
			}

			last_left_ = cycle();
		}
		prev_left_ = true;
	} else {
		prev_left_ = false;
	}

	if(control_status(controls::CONTROL_RIGHT)) {
		walk(lvl, true);
		if(!prev_right_ || running_) {
			if(last_right_ > cycle() - double_tap_cycles || running_) {
				run(lvl, true);
				running_ = true;
			}

			last_right_ = cycle();
		}
		prev_right_ = true;
	} else {
		prev_right_ = false;
	}
}

void pc_character::swimming_control(const level& lvl)
{
	bool idle = true;

	if(control_status(controls::CONTROL_LEFT)) {
		set_face_right(false);
		if(&current_frame() != type().swim_side_frame()) {
			change_frame(type().swim_side_frame());
		}

		if(look_up()) {
			set_rotate(45);
			set_velocity(velocity_x(), velocity_y() - get_accel(current_frame().accel_x()));
		} else if(look_down()) {
			set_rotate(-45);
			set_velocity(velocity_x(), velocity_y() + get_accel(current_frame().accel_x()));
		} else {
			set_rotate(0);
		}

		idle = false;
	} else if(control_status(controls::CONTROL_RIGHT)) {
		set_face_right(true);
		if(&current_frame() != type().swim_side_frame()) {
			change_frame(type().swim_side_frame());
		}

		if(look_up()) {
			set_rotate(-45);
			set_velocity(velocity_x(), velocity_y() - get_accel(current_frame().accel_x()));
		} else if(look_down()) {
			set_rotate(45);
			set_velocity(velocity_x(), velocity_y() + get_accel(current_frame().accel_x()));
		} else {
			set_rotate(0);
		}

		idle = false;
	} else if(control_status(controls::CONTROL_UP)) {
		if(&current_frame() != type().swim_up_frame()) {
			change_frame(type().swim_up_frame());
		}

		idle = false;
	} else if(control_status(controls::CONTROL_DOWN)) {
		if(&current_frame() != type().swim_down_frame()) {
			change_frame(type().swim_down_frame());
		}

		idle = false;
	}

	if(idle) {
		if(&current_frame() == type().swim_side_frame()) {
			change_frame(type().swim_side_idle_frame());
		} else if(&current_frame() == type().swim_up_frame()) {
			change_frame(type().swim_up_idle_frame());
		} else if(&current_frame() == type().swim_down_frame()) {
			change_frame(type().swim_down_idle_frame());
		}
	}
}

void pc_character::save_game()
{
	save_condition_ = NULL;
	save_condition_ = new pc_character(*this);

	// If we're in a vehicle with a driver we have to set the save condition
	// for the driver as well, and make a deep copy of the driver.
	if(driver()) {
		save_condition_->driver() = driver()->clone();
		driver()->save_game();
	}
}

entity_ptr character::backup() const
{
	return entity_ptr(new character(*this));
}

entity_ptr pc_character::backup() const
{
	return entity_ptr(new pc_character(*this));
}

void pc_character::respawn_player()
{
	heal();
	set_pos(spawn_x_, spawn_y_);
	set_velocity(0, 0);
	change_to_stand_frame();
}

std::string character::debug_description() const
{
	std::ostringstream s;
	s << type_->id() << "/" << current_frame().id() << "/" << time_in_frame_ << "/";

	s << "/velx=" << velocity_x_ << ";vely=" << velocity_y_;

	return s.str();
}

void character::make_draw_color()
{
	if(!draw_color_.get()) {
		draw_color_.reset(new graphics::color_transform(0xFF, 0xFF, 0xFF, 0xFF));
	}
}
