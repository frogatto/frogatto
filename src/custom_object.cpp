#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "graphical_font.hpp"
#include "level.hpp"
#include "level_logic.hpp"
#include "playable_custom_object.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "unit_test.hpp"
#include "utils.hpp"

struct custom_object_text {
	std::string text;
	const_graphical_font_ptr font;
};

custom_object::custom_object(wml::const_node_ptr node)
  : entity(node),
    previous_y_(y()),
	custom_type_(node->get_child("type")),
    type_(custom_type_ ?
	      const_custom_object_type_ptr(new custom_object_type(custom_type_)) :
		  custom_object_type::get(node->attr("type"))),
    frame_(&type_->default_frame()),
	frame_name_(wml::get_str(node, "current_frame", "normal")),
	time_in_frame_(wml::get_int(node, "time_in_frame")),
	velocity_x_(wml::get_int(node, "velocity_x")),
	velocity_y_(wml::get_int(node, "velocity_y")),
	accel_x_(wml::get_int(node, "accel_x")),
	accel_y_(wml::get_int(node, "accel_y")),
	rotate_(0), zorder_(wml::get_int(node, "zorder", type_->zorder())),
	hitpoints_(wml::get_int(node, "hitpoints", type_->hitpoints())),
	was_underwater_(false), invincible_(0),
	lvl_(NULL),
	vars_(new game_logic::map_formula_callable(node->get_child("vars"))),
	tmp_vars_(new game_logic::map_formula_callable),
	last_hit_by_anim_(0),
	current_animation_id_(0),
	cycle_(wml::get_int(node, "cycle")),
	loaded_(false),
	can_interact_with_(false)
{
	if(node->has_attr("label")) {
		set_label(node->attr("label"));
	} else {
		set_distinct_label();
	}

	if(!type_->respawns()) {
		set_respawn(false);
	}

	assert(type_.get());
	set_frame(frame_name_);

	next_animation_formula_ = type_->next_animation_formula();

	custom_object_type::init_event_handlers(node, event_handlers_);
	std::cerr << "custom_object: " << body_rect().w() << "\n";

	can_interact_with_ = (event_handlers_.count("interact") || type_->get_event_handler("interact"));

	wml::const_node_ptr editor_info = node->get_child("editor_info");
	if(editor_info) {
		std::cerr << "CREATE EDITOR INFO\n";
		set_editor_info(const_editor_entity_info_ptr(new editor_entity_info(editor_info)));
	}

	wml::const_node_ptr text_node = node->get_child("text");
	if(text_node) {
		text_.reset(new custom_object_text);
		text_->text = text_node->attr("text");
		text_->font = graphical_font::get(text_node->attr("font"));
	}
}

custom_object::custom_object(const std::string& type, int x, int y, bool face_right)
  : entity(x, y, face_right),
    previous_y_(y),
    type_(custom_object_type::get(type)),
	frame_(&type_->default_frame()),
    frame_name_("normal"),
	time_in_frame_(0),
	velocity_x_(0), velocity_y_(0),
	accel_x_(0), accel_y_(0),
	rotate_(0), zorder_(type_->zorder()),
	hitpoints_(type_->hitpoints()),
	was_underwater_(false), invincible_(0),
	lvl_(NULL),
	vars_(new game_logic::map_formula_callable),
	tmp_vars_(new game_logic::map_formula_callable),
	last_hit_by_anim_(0),
	cycle_(0),
	loaded_(false)
{
	{
		//generate a random label for the object
		char buf[64];
		sprintf(buf, "_%x", rand());
		set_label(buf);
	}

	assert(type_.get());
	set_frame(frame_name_);

	next_animation_formula_ = type_->next_animation_formula();
}

custom_object::~custom_object()
{
}

wml::node_ptr custom_object::write() const
{
	wml::node_ptr res(new wml::node("character"));
	if(label().empty() == false) {
		res->set_attr("label", label());
	}

	if(cycle_ > 1) {
		res->set_attr("cycle", formatter() << cycle_);
	}

	if(frame_name_ != "default") {
		res->set_attr("current_frame", frame_name_);
	}

	res->set_attr("custom", "yes");
	res->set_attr("type", type_->id());
	res->set_attr("x", formatter() << x());
	res->set_attr("y", formatter() << y());
	res->set_attr("velocity_x", formatter() << velocity_x_);
	res->set_attr("velocity_y", formatter() << velocity_y_);

	if(zorder_ != type_->zorder()) {
		res->set_attr("zorder", formatter() << y());
	}

	res->set_attr("face_right", face_right() ? "yes" : "no");
	if(upside_down()) {
		res->set_attr("upside_down", "yes");
	}

	res->set_attr("time_in_frame", formatter() << time_in_frame_);

	if(group() >= 0) {
		res->set_attr("group", formatter() << group());
	}

	for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = event_handlers_.begin(); i != event_handlers_.end(); ++i) {
		if(!i->second) {
			continue;
		}
		res->set_attr("on_" + i->first, i->second->str());
	}

	wml::node_ptr vars(new wml::node("vars"));
	vars_->write(vars);
	res->add_child(vars);

	if(custom_type_) {
		res->add_child(wml::deep_copy(custom_type_));
	}

	if(editor_info()) {
		res->add_child(editor_info()->write());
	}

	if(text_) {
		wml::node_ptr node(new wml::node("text"));
		node->set_attr("text", text_->text);
		if(text_->font) {
			node->set_attr("font", text_->font->id());
		}

		res->add_child(node);
	}
	
	return res;
}

void custom_object::setup_drawing() const
{
	if(distortion_) {
		graphics::add_raster_distortion(distortion_.get());
	}
}

void custom_object::draw() const
{
	if(frame_ == NULL) {
		return;
	}

	if(is_human() && ((invincible_/5)%2) == 1) {
		return;
	}

	if(driver_) {
		driver_->draw();
	}

	if(draw_color_) {
		draw_color_->to_color().set_as_current_color();
	}

	frame_->draw(x(), y(), face_right(), upside_down(), time_in_frame_, rotate_);

	if(draw_color_) {
		if(!draw_color_->fits_in_color()) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			graphics::color_transform transform = *draw_color_;
			while(!transform.fits_in_color()) {
				transform = transform - transform.to_color();
				transform.to_color().set_as_current_color();
				frame_->draw(x(), y(), face_right(), upside_down(), time_in_frame_, rotate_);
			}

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		static const uint8_t AllWhite[4] = {0xFF, 0xFF, 0xFF, 0xFF};
		glColor4ubv(AllWhite);
	}

//	if(draw_color_int_ != DefaultColor) {
//		static const uint8_t AllWhite[4] = {0xFF, 0xFF, 0xFF, 0xFF};
//		glColor4ubv(AllWhite);
//	}

	draw_debug_rects();

	for(std::map<std::string, particle_system_ptr>::const_iterator i = particle_systems_.begin(); i != particle_systems_.end(); ++i) {
		i->second->draw(rect(last_draw_position().x/100, last_draw_position().y/100, graphics::screen_width(), graphics::screen_height()), *this);
	}

	if(text_ && text_->font) {
		text_->font->draw(x(), y(), text_->text);
	}
}

void custom_object::draw_group() const
{
	if(label().empty() == false && label()[0] != '_') {
		blit_texture(font::render_text(label(), graphics::color_yellow(), 32), x(), y() + 26);
	}

	if(group() >= 0) {
		blit_texture(font::render_text(formatter() << group(), graphics::color_yellow(), 24), x(), y());
	}
}

namespace {
class collide_with_callable : public game_logic::formula_callable {
	entity* e_;
public:
	explicit collide_with_callable(entity* e) : game_logic::formula_callable(false), e_(e) {}
	variant get_value(const std::string& key) const {
		if(key == "collide_with") {
			return variant(e_);
		} else {
			return variant();
		}
	}
};
}

void custom_object::process(level& lvl)
{
	int surface_friction = 0, surface_traction = 0, surface_damage = 0, surface_adjust_y = 0;
	entity_ptr standing_on;
	const bool started_standing = is_standing(lvl, &surface_friction, &surface_traction, &surface_damage, &surface_adjust_y, &standing_on);
	previous_y_ = y();
	if(started_standing && velocity_y_ > 0) {
		velocity_y_ = 0;
	}

	const int start_x = x();
	++cycle_;
	lvl_ = &lvl;

	if(invincible_) {
		--invincible_;
	}

	if(!loaded_) {
		handle_event("load");
		loaded_ = true;
	}

	if(cycle_ == 1) {
		handle_event("create");
		handle_event("done_create");
	}

	lvl_ = &lvl;

	variant scheduled_command = get_scheduled_command(lvl.cycle());
	while(!scheduled_command.is_null()) {
		execute_command(scheduled_command);
		scheduled_command = get_scheduled_command(lvl.cycle());
	}

	++time_in_frame_;

	if(surface_damage) {
		handle_event("surface_damage");
	}

	if(time_in_frame_ == frame_->duration()) {
		if(next_animation_formula_) {
			variant var = next_animation_formula_->execute(*this);
			set_frame(var.as_string());
		}

		handle_event("end_" + frame_name_ + "_anim");
		handle_event("end_anim");
	}

	const std::string* event = frame_->get_event(time_in_frame_);
	if(event) {
		handle_event(*event);
	}

	velocity_x_ += accel_x_ * (face_right() ? 1 : -1);
	velocity_y_ += accel_y_;

	if(type_->friction()) {
		velocity_x_ = (velocity_x_*(100-(surface_friction+type_->friction())))/100;
		velocity_y_ = (velocity_y_*(100-(surface_friction+type_->friction())))/100;
	}

	if(type_->affected_by_currents()) {
		lvl.get_current(*this, &velocity_x_, &velocity_y_);
	}

	bool collide = false;

	if(type_->ignore_collide()) {
		move_centipixels(velocity_x_, velocity_y_);
	}

	for(int n = 0; n <= std::abs(velocity_x_/100) && !collide && !type_->ignore_collide(); ++n) {
		const int dir = velocity_x_/100 > 0 ? 1 : -1;
		int xpos = dir < 0 ? body_rect().x() : (body_rect().x2() - 1);

		const int ybegin = y() + current_frame().collide_y();
		const int yend = ybegin + current_frame().collide_h();
		int damage = 0;
		for(int ypos = ybegin; ypos != yend; ++ypos) {
			if(lvl.solid(xpos, ypos, NULL, &damage)) {
				collide = true;
				break;
			}
		}

		if(!collide) {
			entity_ptr collide_with = lvl.collide(rect(xpos, ybegin, 1, yend - ybegin), this);
			if(collide_with.get() != NULL) {
				game_logic::formula_callable_ptr callable(new collide_with_callable(collide_with.get()));
				std::cerr << "collide_with\n";
				handle_event("collide_with", callable.get());
				collide = true;
			}
		}

		if(collide) {
			//undo the move to cancel out the collision
			if(n != 0) {
				set_pos(x() - dir, y());
			}
			break;
		}

		//we don't adjust the position on the last time through, since it's only
		//used to see if there was a collision after the last movement, and
		//doesn't actually execute a movement.
		if(n < std::abs(velocity_x_/100)) {
			set_pos(x() + dir, y());

			//if we go up or down a slope, and we began the frame standing,
			//move the character up or down as appropriate to try to keep
			//them standing.

			if(started_standing && !is_standing(lvl)) {
				set_pos(x(), y()+1);
				int max_drop = 2;
				while(--max_drop && started_standing && !is_standing(lvl)) {
					set_pos(x(), y()+1);
				}
			} else if(started_standing) {
				const int original_y = y();
				int max_slope = 3;
				set_pos(x(), y()-1);
				while(--max_slope && is_standing(lvl)) {
					set_pos(x(), y()-1);
				}

				if(!max_slope) {
					set_pos(x(), original_y);
				} else {
					set_pos(x(), y()+1);
				}
			}

		}
	}

	if(collide) {
		handle_event("collide");
	}

	if(!body_passthrough()) {
		entity_ptr collide_with = lvl.collide(body_rect(), this);
		if(collide_with.get() != NULL) {
				game_logic::formula_callable_ptr callable(new collide_with_callable(collide_with.get()));
			std::cerr << "collide_with\n";
			handle_event("collide_with", callable.get());
			collide = true;
		}
	}

	//std::cerr << "velocity_y: " << velocity_y_ << "\n";
	collide = false;
	for(int n = 0; n <= std::abs(velocity_y_/100) && !collide && !type_->ignore_collide(); ++n) {
		const int dir = velocity_y_/100 > 0 ? 1 : -1;
		int ypos = dir < 0 ? body_rect().y() : (body_rect().y2() - 1);

		const int xbegin = body_rect().x();
		const int xend = body_rect().x2();
		int damage = 0;
// TODO: at the moment we don't consider it a collision if we 
//		for(int xpos = xbegin; xpos != xend; ++xpos) {
//			if(lvl.solid(xpos, ypos, NULL, &damage)) {
//				collide = true;
//				break;
//			}
//		}

		if(type_->id() == "breakable_block") {
			std::cerr << "COLLIDE " << n << "/" << velocity_y_/100 << "\n";
		}

		if(!collide && !type_->ignore_collide() && velocity_y_ > 0) {
			entity_ptr bounce = lvl.collide(feet_x() - type_->feet_width(), feet_y(), this);
			if(!bounce) {
				bounce = lvl.collide(feet_x() + type_->feet_width(), feet_y(), this);
			}

			if(bounce && bounce->spring_off_head(*this)) {
				tmp_vars_->add("bounce_off", variant(bounce.get()));
				handle_event("bounce");
				break;
			}
		}

		entity_ptr jumped_on;
		if(!collide && !type_->ignore_collide() && velocity_y_ > 0 && is_standing(lvl, NULL, NULL, NULL, NULL, &jumped_on)) {
			collide = true;
		}

		if(collide) {
			std::cerr << "collide y!\n";
			break;
		}

		//we don't adjust the position on the last time through, since it's only
		//used to see if there was a collision after the last movement, and
		//doesn't actually execute a movement.
		if(n < std::abs(velocity_y_/100)) {
			set_pos(x(), y() + dir);
		}
	}

	if(collide) {
		std::cerr << "COLLIDE!\n";
		if(velocity_y_ < 0 || !started_standing) {
			std::cerr << "COLLIDE EVENT\n";
			handle_event(velocity_y_ < 0 ? "collide_head" : "collide_feet");
		}
	}

	if(!invincible_) {
		if(on_players_side()) {
			entity_ptr collide_with = lvl_->collide(body_rect(), this);
			if(collide_with && collide_with->body_harmful()) {
				handle_event("get_hit");
			}
		} else {
			entity_ptr player = lvl.hit_by_player(body_rect());
			if(player && (last_hit_by_ != player || last_hit_by_anim_ != player->current_animation_id())) {
				last_hit_by_ = player;
				last_hit_by_anim_ = player->current_animation_id();
				handle_event("hit_by_player");
			}

			if(driver_) {
				//if this is a vehicle with a driver, handle the driver being
				//hit by the player.
				entity_ptr player = lvl.hit_by_player(driver_->body_rect());
				if(player && (last_hit_by_ != player || last_hit_by_anim_ != player->current_animation_id())) {
					last_hit_by_ = player;
					last_hit_by_anim_ = player->current_animation_id();
					handle_event("driver_hit_by_player");
				}
			}
		}
	}

	foreach(entity_ptr& p, lvl.players()) {
		if(rects_intersect(body_rect(), p->body_rect())) {
			lvl.set_touched_player(p);
			handle_event(p->enter() ? "interact" : "touch");
		}
	}

	if(lvl.players().empty() == false) {
		lvl.set_touched_player(lvl.players().front());
	}

	static const std::string ProcessStr = "process";
	handle_event(ProcessStr);
	handle_event("process_" + frame_name_);

	if(type_->timer_frequency() > 0 && (cycle_%type_->timer_frequency()) == 0) {
		static const std::string TimerStr = "timer";
		handle_event(TimerStr);
	}

	//adjust anyone who is standing on us by the amount we've moved.
	if(stood_on_by_.empty() == false) {
		const int dx = x() - start_x;
		const int dy = y() - previous_y_;

		foreach(const entity_ptr& c, stood_on_by_) {
			const int start_x = c->x();
			const int start_y = c->y();
			c->set_pos(c->x() + dx, c->y() + dy);
			int damage = 0;
			if(lvl_->solid(c->body_rect(), NULL, NULL, &damage) && damage == 0) {
				//doing this movement would make the character collide with
				//something solid, so undo it.
				c->set_pos(start_x, start_y);
			}
		}
		stood_on_by_.clear();
	}
	
	const bool is_underwater = lvl.is_underwater(body_rect());
	if( is_underwater && !was_underwater_){
		//event on_enter_water
		const static std::string EnterWaterStr = "enter_water";
		handle_event(EnterWaterStr);
		was_underwater_ = true;
	}else if ( !is_underwater && was_underwater_ ){
		//event on_exit_water
		const static std::string ExitWaterStr = "exit_water";
		handle_event(ExitWaterStr);
		was_underwater_ = false;
	}

	for(std::map<std::string, particle_system_ptr>::iterator i = particle_systems_.begin(); i != particle_systems_.end(); ) {
		i->second->process(*this);
		if(i->second->is_destroyed()) {
			particle_systems_.erase(i++);
		} else {
			++i;
		}
	}

	set_driver_position();
}

void custom_object::set_driver_position()
{
	if(driver_) {
		const int pos_right = x() + type_->passenger_x();
		const int pos_left = x() + current_frame().width() - driver_->current_frame().width() - type_->passenger_x();
		driver_->set_face_right(face_right());

		driver_->set_pos(face_right() ? pos_right : pos_left, y() + type_->passenger_y());
	}
}

int custom_object::zorder() const
{
	return zorder_;
}

int custom_object::velocity_x() const
{
	return velocity_x_;
}

int custom_object::velocity_y() const
{
	return velocity_y_;
}

bool custom_object::is_standable(int xpos, int ypos, int* friction, int* traction, int* adjust_y) const
{
	if(!body_passthrough() && springiness() == 0 && !body_harmful() && point_collides(xpos, ypos)) {
		if(friction) {
			*friction = type_->surface_friction();
		}

		if(traction) {
			*traction = type_->surface_traction();
		}

		if(adjust_y) {
			if(type_->use_image_for_collisions()) {
				for(*adjust_y = 0; point_collides(xpos, ypos - *adjust_y - 1); --(*adjust_y)) {
				}
			} else {
				*adjust_y = ypos - body_rect().y();
			}
		}

		return true;
	}

	if(frame_->has_platform()) {
		const frame& f = *frame_;
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
			*friction = type_->surface_friction();
		}

		if(traction) {
			*traction = type_->surface_traction();
		}

		if(adjust_y) {
			*adjust_y = y() + f.platform_y() - ypos;
		}

		return true;
	}

	return false;
}

void custom_object::stood_on_by(const entity_ptr& ch)
{
	handle_event("stood_on");
	stood_on_by_.push_back(ch);
}

bool custom_object::destroyed() const
{
	return hitpoints_ <= 0;
}

bool custom_object::point_collides(int xpos, int ypos) const
{
	if(type_->use_image_for_collisions()) {
		const bool result = !current_frame().is_alpha(xpos - x(), ypos - y(), time_in_frame_, face_right());
		return result;
	} else {
		return point_in_rect(point(xpos, ypos), body_rect());
	}
}

bool custom_object::rect_collides(const rect& r) const
{
	if(type_->use_image_for_collisions()) {
		rect myrect(x(), y(), current_frame().width(), current_frame().height());
		if(rects_intersect(myrect, r)) {
			rect intersection = intersection_rect(myrect, r);
			for(int y = intersection.y(); y < intersection.y2(); ++y) {
				for(int x = intersection.x(); x < intersection.x2(); ++x) {
					if(point_collides(x, y)) {
						return true;
					}
				}
			}

			return false;
		} else {
			return false;
		}
	} else {
		return rects_intersect(r, body_rect());
	}
}

bool custom_object::on_players_side() const
{
	return type_->on_players_side() || is_human();
}

void custom_object::control(const level& lvl)
{
}

bool custom_object::is_standing(const level& lvl, int* friction, int* traction, int* damage, int* adjust_y, entity_ptr* standing_on) const
{
	return (current_frame().feet_x() || current_frame().feet_y()) &&
	       (lvl.standable(feet_x() - type_->feet_width(), feet_y(), friction, traction, damage, adjust_y, standing_on) ||
	        lvl.standable(feet_x() + type_->feet_width(), feet_y(), friction, traction, damage, adjust_y, standing_on));
}

namespace {
typedef variant (*object_accessor)(const custom_object& obj);
//typedef boost::function<variant(const custom_object& obj)> object_accessor;
std::map<std::string, object_accessor> object_accessor_map;
}

//A utility class which is used to calculate the value of a custom object's
//attributes for the formula system.
struct custom_object::Accessor {
#define CUSTOM_ACCESSOR(name, expression) static variant name(const custom_object& obj) { return variant(expression); }
#define SIMPLE_ACCESSOR(name) static variant name(const custom_object& obj) { return variant(obj.name##_); }
	CUSTOM_ACCESSOR(type, obj.type_->id())
	CUSTOM_ACCESSOR(time_in_animation, obj.time_in_frame_)
	CUSTOM_ACCESSOR(level, obj.lvl_)
	CUSTOM_ACCESSOR(animation, obj.frame_name_)
	SIMPLE_ACCESSOR(hitpoints)
	CUSTOM_ACCESSOR(max_hitpoints, obj.type_->hitpoints())
	CUSTOM_ACCESSOR(mass, obj.type_->mass())
	CUSTOM_ACCESSOR(label, obj.label())
	CUSTOM_ACCESSOR(x, obj.x())
	CUSTOM_ACCESSOR(y, obj.y())
	CUSTOM_ACCESSOR(z, obj.zorder_)
	CUSTOM_ACCESSOR(x1, obj.body_rect().x())
	CUSTOM_ACCESSOR(y1, obj.body_rect().y())
	CUSTOM_ACCESSOR(x2, obj.body_rect().x2())
	CUSTOM_ACCESSOR(y2, obj.body_rect().y2())
	CUSTOM_ACCESSOR(w, obj.body_rect().w())
	CUSTOM_ACCESSOR(h, obj.body_rect().h())

	//note that we're taking the image midpoint, NOT the collision-rect midpoint
	//in practice, we've always calculated this from the image for our scripting,
	//and many object actually lack non-zero collision-rect widths.
	CUSTOM_ACCESSOR(midpoint_x, obj.x() + obj.current_frame().width()/2)
	CUSTOM_ACCESSOR(midpoint_y, obj.y() + obj.current_frame().height()/2)

	CUSTOM_ACCESSOR(img_w, obj.current_frame().width());
	CUSTOM_ACCESSOR(img_h, obj.current_frame().height());
	CUSTOM_ACCESSOR(front, obj.face_right() ? obj.body_rect().x2() : obj.body_rect().x());
	CUSTOM_ACCESSOR(back, obj.face_right() ? obj.body_rect().x() : obj.body_rect().x2());
	SIMPLE_ACCESSOR(cycle);
	CUSTOM_ACCESSOR(facing, obj.face_right() ? 1 : -1);
	CUSTOM_ACCESSOR(upside_down, obj.upside_down() ? 1 : 0);
	CUSTOM_ACCESSOR(up, obj.upside_down() ? 1 : -1);
	CUSTOM_ACCESSOR(down, obj.upside_down() ? -1 : 1);
	SIMPLE_ACCESSOR(velocity_x);
	SIMPLE_ACCESSOR(velocity_y);
	SIMPLE_ACCESSOR(accel_x);
	SIMPLE_ACCESSOR(accel_y);
	CUSTOM_ACCESSOR(vars, obj.vars_.get());
	CUSTOM_ACCESSOR(tmp, obj.tmp_vars_.get());
	CUSTOM_ACCESSOR(group, obj.group());
	SIMPLE_ACCESSOR(rotate);
	CUSTOM_ACCESSOR(me, &obj);
	CUSTOM_ACCESSOR(stood_on, obj.stood_on_by_.size());
	CUSTOM_ACCESSOR(red, obj.draw_color().r());
	CUSTOM_ACCESSOR(green, obj.draw_color().g());
	CUSTOM_ACCESSOR(blue, obj.draw_color().b());
	CUSTOM_ACCESSOR(alpha, obj.draw_color().a());
	CUSTOM_ACCESSOR(damage, obj.current_frame().damage());
	CUSTOM_ACCESSOR(hit_by, obj.last_hit_by_.get());
	CUSTOM_ACCESSOR(jumped_on_by, obj.last_jumped_on_by_.get());
	CUSTOM_ACCESSOR(distortion, obj.distortion_.get());
	CUSTOM_ACCESSOR(is_standing, (obj.lvl_ ? variant(obj.is_standing(*obj.lvl_)) : variant()));
	CUSTOM_ACCESSOR(near_cliff_edge, obj.is_standing(*obj.lvl_) && cliff_edge_within(*obj.lvl_, obj.feet_x(), obj.feet_y(), obj.face_dir()*15));
	CUSTOM_ACCESSOR(distance_to_cliff, ::distance_to_cliff(*obj.lvl_, obj.feet_x(), obj.feet_y(), obj.face_dir()));
	CUSTOM_ACCESSOR(slope_standing_on, -obj.slope_standing_on(obj.type_->feet_width()*2)*obj.face_dir());
	CUSTOM_ACCESSOR(underwater, obj.lvl_->is_underwater(obj.body_rect()));
	CUSTOM_ACCESSOR(driver, obj.driver_.get());
	CUSTOM_ACCESSOR(is_human, obj.is_human() ? 1 : 0);
	SIMPLE_ACCESSOR(invincible);
	CUSTOM_ACCESSOR(springiness, obj.springiness());
	CUSTOM_ACCESSOR(destroyed, obj.destroyed());
#undef SIMPLE_ACCESSOR
#undef CUSTOM_ACCESSOR

	static variant standing_on(const custom_object& obj) {
		if(!obj.lvl_) {
			return variant();
		}
		
		entity_ptr stand_on;
		obj.is_standing(*obj.lvl_, NULL, NULL, NULL, NULL, &stand_on);
		return variant(stand_on.get());
	}

#define CUSTOM_ACCESSOR(name, expression) static variant name(const custom_object& obj) { return variant(expression); }

	static void init() {
#define ACCESSOR(name) object_accessor_map.insert(std::pair<std::string,object_accessor>(#name, name))
		ACCESSOR(type);
		ACCESSOR(time_in_animation);
		ACCESSOR(level);
		ACCESSOR(animation);
		ACCESSOR(hitpoints);
		ACCESSOR(max_hitpoints);
		ACCESSOR(mass);
		ACCESSOR(label);
		ACCESSOR(x);
		ACCESSOR(y);
		ACCESSOR(z);
		ACCESSOR(x1);
		ACCESSOR(y1);
		ACCESSOR(x2);
		ACCESSOR(y2);
		ACCESSOR(w);
		ACCESSOR(h);
		ACCESSOR(midpoint_x);
		ACCESSOR(midpoint_y);
		ACCESSOR(img_w);
		ACCESSOR(img_h);
		ACCESSOR(front);
		ACCESSOR(back);
		ACCESSOR(cycle);
		ACCESSOR(facing);
		ACCESSOR(upside_down);
		ACCESSOR(up);
		ACCESSOR(down);
		ACCESSOR(velocity_x);
		ACCESSOR(velocity_y);
		ACCESSOR(accel_x);
		ACCESSOR(accel_y);
		ACCESSOR(vars);
		ACCESSOR(group);
		ACCESSOR(rotate);
		ACCESSOR(me);
		ACCESSOR(stood_on);
		ACCESSOR(red);
		ACCESSOR(green);
		ACCESSOR(blue);
		ACCESSOR(alpha);
		ACCESSOR(damage);
		ACCESSOR(hit_by);
		ACCESSOR(jumped_on_by);
		ACCESSOR(distortion);
		ACCESSOR(is_standing);
		ACCESSOR(near_cliff_edge);
		ACCESSOR(distance_to_cliff);
		ACCESSOR(slope_standing_on);
		ACCESSOR(underwater);
		ACCESSOR(driver);
		ACCESSOR(is_human);
		ACCESSOR(invincible);
		ACCESSOR(springiness);
		ACCESSOR(destroyed);
		ACCESSOR(standing_on);
	}
};

void custom_object::init()
{
	Accessor::init();
}

variant custom_object::get_value(const std::string& key) const
{
	std::map<std::string, object_accessor>::const_iterator accessor_itor = object_accessor_map.find(key);
	if(accessor_itor != object_accessor_map.end()) {
		return accessor_itor->second(*this);
	}

	variant var_result = tmp_vars_->query_value(key);
	if(!var_result.is_null()) {
		return var_result;
	}

	var_result = vars_->query_value(key);
	if(!var_result.is_null()) {
		return var_result;
	}

	std::map<std::string, variant>::const_iterator i = type_->variables().find(key);
	if(i != type_->variables().end()) {
		return i->second;
	}

	std::map<std::string, particle_system_ptr>::const_iterator particle_itor = particle_systems_.find(key);
	if(particle_itor != particle_systems_.end()) {
		return variant(particle_itor->second.get());
	}

	return variant();
}

void custom_object::get_inputs(std::vector<game_logic::formula_input>* inputs) const
{
	inputs->push_back(game_logic::formula_input("time_in_animation", game_logic::FORMULA_READ_WRITE));
	inputs->push_back(game_logic::formula_input("level", game_logic::FORMULA_READ_ONLY));
	inputs->push_back(game_logic::formula_input("animation", game_logic::FORMULA_READ_ONLY));
	inputs->push_back(game_logic::formula_input("hitpoints", game_logic::FORMULA_READ_WRITE));
}

void custom_object::set_value(const std::string& key, const variant& value)
{
	if(key == "animation") {
		set_frame(value.as_string());
	} else if(key == "time_in_animation") {
		time_in_frame_ = value.as_int();
	} else if(key == "x") {
		set_x(value.as_int());
	} else if(key == "y") {
		set_y(value.as_int());
	} else if(key == "z") {
		zorder_ = value.as_int();
	} else if(key == "midpoint_x") {
		set_pos(value.as_int() - body_rect().w()/2, y());
	} else if(key == "midpoint_y") {
		set_pos(x(), value.as_int() - body_rect().h()/2);
	} else if(key == "facing") {
		set_face_right(value.as_int() > 0);
	} else if(key == "upside_down") {
		set_upside_down(value.as_int());
	} else if(key == "hitpoints") {
		hitpoints_ = value.as_int();
		if(hitpoints_ <= 0) {
			die();
		}
	} else if(key == "velocity_x") {
		velocity_x_ = value.as_int();
	} else if(key == "velocity_y") {
		velocity_y_ = value.as_int();
	} else if(key == "accel_x") {
		accel_x_ = value.as_int();
	} else if(key == "accel_y") {
		accel_y_ = value.as_int();
	} else if(key == "rotate") {
		rotate_ = value.as_int();
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
	} else if(key == "brightness"){
		make_draw_color();
		draw_color_->buf()[0] = value.as_int();
		draw_color_->buf()[1] = value.as_int();
		draw_color_->buf()[2] = value.as_int();
	} else if(key == "distortion") {
		distortion_ = value.try_convert<graphics::raster_distortion>();
	} else if(key == "current_generator") {
		set_current_generator(value.try_convert<current_generator>());
	} else if(key == "invincible") {
		invincible_ = value.as_int();
	} else {
		vars_->add(key, value);
	}
}

void custom_object::set_frame(const std::string& name)
{
	//fire an event to say that we're leaving the current frame.
	if(frame_) {
		handle_event("leave_" + frame_name_ + "_anim");
	}

	const int start_x = feet_x();
	const int start_y = feet_y();

	frame_ = &type_->get_frame(name);
	++current_animation_id_;

	const int diff_x = feet_x() - start_x;
	const int diff_y = feet_y() - start_y;

	set_pos(x() - diff_x, y() - diff_y);

	//try to resolve collision by adjusting position
	if(lvl_ && !type_->ignore_collide()) {
		const int MaxAdjust = 10;
		rect body = body_rect();
		const int middle = (body.y() + body.y2())/2;
		int n = 0;
		while(n != MaxAdjust && lvl_->solid(body.x() + n, middle)) {
			++n;
		}

		set_pos(x() + n, y());

		n = 0;
		while(n != MaxAdjust && lvl_->solid(body.x2() - n, middle)) {
			++n;
		}

		set_pos(x() - n, y());
	}

	frame_name_ = name;
	time_in_frame_ = 0;
	if(frame_->velocity_x() != INT_MIN) {
		velocity_x_ = frame_->velocity_x() * (face_right() ? 1 : -1);
	}

	if(frame_->velocity_y() != INT_MIN) {
		velocity_y_ = frame_->velocity_y();
	}

	if(frame_->accel_x() != INT_MIN) {
		accel_x_ = frame_->accel_x();
	}
	
	if(frame_->accel_y() != INT_MIN) {
		accel_y_ = frame_->accel_y();
	}
	
	
	frame_->play_sound(this);
}

void custom_object::die()
{
	hitpoints_ = 0;
	handle_event("die");
}

void custom_object::hit_player()
{
	handle_event("hit_player");
}

void custom_object::hit_by(entity& e)
{
	std::cerr << "hit_by!\n";
	last_hit_by_ = &e;
	handle_event("hit_by_player");
}

bool custom_object::dies_on_inactive() const
{
	return type_->dies_on_inactive();
}

bool custom_object::always_active() const
{
	return type_->always_active();
}

bool custom_object::body_harmful() const
{
	return type_->body_harmful();
}

bool custom_object::body_passthrough() const
{
	return type_->body_passthrough();
}

int custom_object::springiness() const
{
	return type_->springiness();
}

bool custom_object::spring_off_head(entity& landed_on_by)
{
	last_jumped_on_by_ = entity_ptr(&landed_on_by);
	handle_event("jumped_on");
	return true;
}

const frame& custom_object::portrait_frame() const
{
	return type_->get_frame("portrait");
}

const frame& custom_object::icon_frame() const
{
	return type_->default_frame();
}

entity_ptr custom_object::clone() const
{
	entity_ptr res(new custom_object(*this));
	res->set_distinct_label();
	return res;
}

entity_ptr custom_object::backup() const
{
	entity_ptr res(new custom_object(*this));
	return res;
}

void custom_object::handle_event(const std::string& event, const formula_callable* context)
{
	if(hitpoints_ <= 0 && event != "die") {
		return;
	}

	std::vector<game_logic::const_formula_ptr> handlers;

	std::map<std::string, game_logic::const_formula_ptr>::const_iterator handler_itor = event_handlers_.find(event);
	if(handler_itor != event_handlers_.end()) {
		game_logic::const_formula_ptr handler = handler_itor->second;
		if(handler) {
			handlers.push_back(handler);
		}
	}


	game_logic::const_formula_ptr handler = type_->get_event_handler(event);
	if(handler) {
		handlers.push_back(handler);
	}

	foreach(const game_logic::const_formula_ptr& handler, handlers) {
		variant var;

		if(context) {
			game_logic::formula_callable_with_backup callable(*this, *context);
			var = handler->execute(callable);
		} else {
			var = handler->execute(*this);
		}

		const bool result = execute_command(var);
		if(!result) {
			break;
		}
	}
}

bool custom_object::execute_command(const variant& var)
{
	bool result = true;
	if(var.is_null()) { return result; }
	if(var.is_list()) {
		for(int n = 0; n != var.num_elements(); ++n) {
			result = execute_command(var[n]) && result;
		}
	} else {
		custom_object_command_callable* cmd = var.try_convert<custom_object_command_callable>();
		if(cmd != NULL) {
			cmd->execute(*lvl_, *this);
		} else {
			entity_command_callable* cmd = var.try_convert<entity_command_callable>();
			if(cmd != NULL) {
				cmd->execute(*lvl_, *this);
			} else {
				if(var.try_convert<swallow_object_command_callable>()) {
					result = false;
				}
			}
		}
	}

	return result;
}

int custom_object::slope_standing_on(int range) const
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

void custom_object::make_draw_color()
{
	if(!draw_color_.get()) {
		draw_color_.reset(new graphics::color_transform(draw_color()));
	}
}

const graphics::color_transform& custom_object::draw_color() const
{
	if(draw_color_.get()) {
		return *draw_color_;
	}

	static const graphics::color_transform white(0xFF, 0xFF, 0xFF, 0xFF);
	return white;
}

game_logic::const_formula_ptr custom_object::get_event_handler(const std::string& key) const
{
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator itor = event_handlers_.find(key);
	if(itor != event_handlers_.end()) {
		return itor->second;
	}

	return game_logic::const_formula_ptr();
}

void custom_object::set_event_handler(const std::string& key, game_logic::const_formula_ptr f)
{
	if(!f) {
		event_handlers_.erase(key);
	} else {
		event_handlers_[key] = f;
	}
}

bool custom_object::can_interact_with() const
{
	return can_interact_with_;
}

std::string custom_object::debug_description() const
{
	return type_->id();
}

void custom_object::map_entities(const std::map<entity_ptr, entity_ptr>& m)
{
	if(last_hit_by_) {
		std::map<entity_ptr, entity_ptr>::const_iterator i = m.find(last_hit_by_);
		if(i != m.end()) {
			last_hit_by_ = i->second;
		}
	}

	foreach(entity_ptr& e, stood_on_by_) {
		std::map<entity_ptr, entity_ptr>::const_iterator i = m.find(e);
		if(i != m.end()) {
			e = i->second;
		}
	}
}

void custom_object::add_particle_system(const std::string& key, const std::string& type)
{
	particle_systems_[key] = type_->get_particle_system_factory(type)->create(*this);
}

void custom_object::remove_particle_system(const std::string& key)
{
	particle_systems_.erase(key);
}

void custom_object::set_text(const std::string& text, const std::string& font)
{
	text_.reset(new custom_object_text);
	text_->text = text;
	text_->font = graphical_font::get(font);
}

bool custom_object::boardable_vehicle() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

void custom_object::boarded(level& lvl, const entity_ptr& player)
{
	if(!player) {
		return;
	}

	player->board_vehicle();

	if(player->is_human()) {
		playable_custom_object* new_player(new playable_custom_object(*this));
		new_player->driver_ = player;

		lvl.add_player(new_player);

		new_player->get_player_info()->swap_player_state(*player->get_player_info());
		lvl.remove_character(this);
	} else {
		driver_ = player;
		lvl.remove_character(player);
	}
}

void custom_object::unboarded(level& lvl)
{
	if(velocity_x() > 100) {
		driver_->set_face_right(false);
	}

	if(velocity_x() < -100) {
		driver_->set_face_right(true);
	}

	if(is_human()) {
		custom_object* vehicle(new custom_object(*this));
		vehicle->driver_ = entity_ptr();
		lvl.add_character(vehicle);

		lvl.add_player(driver_);

		driver_->unboard_vehicle();

		driver_->get_player_info()->swap_player_state(*get_player_info());
	} else {
		lvl.add_character(driver_);
		driver_->unboard_vehicle();
		driver_ = entity_ptr();
	}
}

void custom_object::board_vehicle()
{
}

void custom_object::unboard_vehicle()
{
}

BENCHMARK_ARG(custom_object_get_attr, const std::string& attr)
{
	static custom_object* obj = new custom_object("black_ant", 0, 0, false);
	BENCHMARK_LOOP {
		obj->query_value(attr);
	}
}

BENCHMARK_ARG_CALL(custom_object_get_attr, easy_lookup, "x");
BENCHMARK_ARG_CALL(custom_object_get_attr, hard_lookup, "xxxx");

BENCHMARK_ARG(custom_object_handle_event, const std::string& object_event)
{
	std::string::const_iterator i = std::find(object_event.begin(), object_event.end(), ':');
	ASSERT_LOG(i != object_event.end(), "custom_object_event_handle argument must have a pipe seperator: " << object_event);
	std::string obj_type(object_event.begin(), i);
	std::string event_name(i+1, object_event.end());
	static level* lvl = new level("titlescreen.cfg");
	static custom_object* obj = new custom_object(obj_type, 0, 0, false);
	obj->set_level(*lvl);
	BENCHMARK_LOOP {
		obj->handle_event(event_name);
	}
}

BENCHMARK_ARG_CALL(custom_object_handle_event, ant_collide, "black_ant:collide");
BENCHMARK_ARG_CALL(custom_object_handle_event, ant_non_exist, "black_ant:blahblah");

BENCHMARK_ARG_CALL_COMMAND_LINE(custom_object_handle_event);
