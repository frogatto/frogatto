#include "iphone_controls.hpp"

#if TARGET_OS_HARMATTAN || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || TARGET_BLACKBERRY || defined(__ANDROID__)

#include "graphics.hpp"

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
#include <math.h> // sqrt
#endif

#include "foreach.hpp"
#include "formula.hpp"
#include "geometry.hpp"
#include "json_parser.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "variant.hpp"

namespace
{
	rect left_arrow, right_arrow, down_arrow, up_arrow, a_button, c_button, interact_button, jumpdown_button, spin_button;

	int underwater_circle_rad, underwater_circle_x, underwater_circle_y;

	bool is_underwater = false;
	bool can_interact = false;
	bool on_platform = false;
	bool is_standing = false;

	std::string loaded_control_scheme = "";
	static void setup_rects ()
	{
		const std::string& scheme_name = preferences::control_scheme();
		if (loaded_control_scheme == scheme_name)
		{
			return;
		}
		loaded_control_scheme = scheme_name;
		
		underwater_circle_y = preferences::virtual_screen_height()-underwater_circle_y;
		
		variant schemes = json::parse_from_file("data/control_schemes.cfg");
		variant scheme;
		foreach(const variant& candidate, schemes["control_scheme"].as_list()) {
			if(candidate["id"].as_string() == scheme_name) {
				scheme = candidate;
				break;
			}
		}
		
		underwater_circle_x = game_logic::formula(scheme["underwater_circle_x"]).execute().as_int();
		underwater_circle_y = game_logic::formula(scheme["underwater_circle_y"]).execute().as_int();
		underwater_circle_rad = game_logic::formula(scheme["underwater_circle_rad"]).execute().as_int();
		
		foreach(variant node, scheme["button"].as_list())
		{
			variant r = game_logic::formula(node["hit_rect"]).execute();
			rect hit_rect(r[0].as_int(), r[1].as_int(), r[2].as_int(), r[3].as_int());
			std::string id = node["id"].as_string();
			if (id == "left") {
				left_arrow = hit_rect;
			} else if (id == "right") {
				right_arrow = hit_rect;
			} else if (id == "up") {
				up_arrow = hit_rect;
			} else if (id == "down") {
				down_arrow = hit_rect;
			} else if (id == "b") {
				c_button = hit_rect;
			} else if (id == "a") {
				a_button = hit_rect;
			} else if (id == "interact") {
				interact_button = hit_rect;
			} else if (id == "jump_down") {
				jumpdown_button = hit_rect;
			} else if (id == "spin") {
				spin_button = hit_rect;
			}
		}
		
//		b_button = rect(vw - 102, vh - 300, 50*2, 60*2);
	}

	struct Mouse {
		bool active;
		int x, y;
		int starting_x, starting_y;
#if defined(__ANDROID__)
        int pressure; 
#endif
	};

	std::vector<Mouse> all_mice, active_mice;
}

void translate_mouse_coords (int *x, int *y)
{
	if(preferences::screen_rotated()) {
		*x = preferences::actual_screen_width() - *x;
		std::swap(*x, *y);
	}
	
	if(preferences::virtual_screen_width() >
	   (preferences::screen_rotated() ? preferences::actual_screen_height() : preferences::actual_screen_width())) {
		*x *= 2;
		*y *= 2;
	}
}

void iphone_controls::read_controls()
{
	active_mice.clear();

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
	// there is no SDL_Get_NumMice and SDL_SelectMouse support on
	// Harmattan, so all_mice has been updated via calls to handle_event
	const int nmice = all_mice.size();
#else
	const int nmice = SDL_GetNumMice();
	if(all_mice.size() > nmice) {
		all_mice.resize(nmice);
	}

	for(int i = 0; i < nmice; i++) {
		int x, y;
		SDL_SelectMouse(i);
		Uint8 button_state = SDL_GetMouseState(&x, &y);
		translate_mouse_coords(&x, &y);
		if(all_mice.size() == i) {
			all_mice.push_back(Mouse());
			all_mice[i].active = false;
		}

		if(!all_mice[i].active) {
			all_mice[i].starting_x = x;
			all_mice[i].starting_y = y;
		}

		all_mice[i].x = x;
		all_mice[i].y = y;
		all_mice[i].active = button_state & SDL_BUTTON(SDL_BUTTON_LEFT);
	}
#endif
	for(int i = 0; i < nmice; i++) {
		if(all_mice[i].active) {
			active_mice.push_back(all_mice[i]);
		}
	}
}

#if defined(__ANDROID__)
void iphone_controls::handle_event (const SDL_Event& event)
{
	int x = event.type == SDL_JOYBALLMOTION ? event.jball.xrel : event.jbutton.x;
	int y = event.type == SDL_JOYBALLMOTION ? event.jball.yrel : event.jbutton.y;
	int i = event.type == SDL_JOYBALLMOTION ? event.jball.ball : event.jbutton.button;
	std::string joy_txt = event.type == SDL_JOYBUTTONUP ? "up" : event.type == SDL_JOYBUTTONDOWN ? "down" : "move";
	LOG( "mouse " << joy_txt << " (" << x << "," << y << ";" << i << ")");
	translate_mouse_coords(&x, &y);
	while(all_mice.size() <= i) {
		all_mice.push_back(Mouse());
		all_mice[i].active = false;
	}

	if(!all_mice[i].active) {
		all_mice[i].starting_x = x;
		all_mice[i].starting_y = y;
	}

	all_mice[i].x = x;
	all_mice[i].y = y;
	all_mice[i].active = event.type != SDL_JOYBUTTONUP;
}
     
#elif defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY)

void iphone_controls::handle_event (const SDL_Event& event)
	int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
	int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
	int i = event.type == SDL_MOUSEMOTION ? event.motion.which : event.button.which;
	translate_mouse_coords(&x, &y);
	while(all_mice.size() <= i) {
		all_mice.push_back(Mouse());
		all_mice[i].active = false;
	}

	if(!all_mice[i].active) {
		all_mice[i].starting_x = x;
		all_mice[i].starting_y = y;
	}

	all_mice[i].x = x;
	all_mice[i].y = y;
	all_mice[i].active = event.type != SDL_MOUSEBUTTONUP;
}

#endif

void iphone_controls::set_underwater(bool value)
{
	is_underwater = value;
}

void iphone_controls::set_can_interact(bool value)
{
	can_interact = value;
}

void iphone_controls::set_on_platform(bool value)
{
	on_platform = value;
}

void iphone_controls::set_standing(bool value)
{
	is_standing = value;
}

bool iphone_controls::water_dir(float* xvalue, float* yvalue)
{
	setup_rects();
	foreach(const Mouse& mouse, active_mice) {
		const int dx = mouse.starting_x - underwater_circle_x;
		const int dy = mouse.starting_y - underwater_circle_y;

		const int distance = sqrt(dx*dx + dy*dy);

		if(distance > 0 && distance < 2.3 * underwater_circle_rad) {
			const int dx = mouse.x - underwater_circle_x;
			const int dy = mouse.y - underwater_circle_y;

			const int distance = sqrt(dx*dx + dy*dy);

			*xvalue = float(dx)/float(distance);
			*yvalue = float(dy)/float(distance);
			return true;
		}
	}

	return false;
}
	
void iphone_controls::draw()
{
	if(!is_underwater) {
		if (preferences::show_iphone_controls())
		{
			graphics::draw_rect(left_arrow, graphics::color(255, 0, 0, 64));
			graphics::draw_rect(right_arrow, graphics::color(255, 0, 0, 64));
			graphics::draw_rect(up_arrow, graphics::color(0, 255, 0, 64));
			graphics::draw_rect(down_arrow, graphics::color(0, 0, 255, 64));
			graphics::draw_rect(a_button, graphics::color(255, 0, 0, 64));
			graphics::draw_rect(c_button, graphics::color(0, 255, 0, 64));
			graphics::draw_rect(interact_button, graphics::color(0, 0, 255, 64));
			graphics::draw_rect(jumpdown_button, graphics::color(255, 0, 255, 64));
		}
		return;
	}

	glColor4ub(128, 128, 128, 128);
	graphics::draw_circle(underwater_circle_x, underwater_circle_y, underwater_circle_rad);

	GLfloat x, y;
	if(water_dir(&x, &y)) {
		GLfloat varray[] = {
		  underwater_circle_x, underwater_circle_y,
		  underwater_circle_x + x*underwater_circle_rad, underwater_circle_y + y*underwater_circle_rad
		};

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4ub(255, 0, 0, 255);
		glVertexPointer(2, GL_FLOAT, 0, varray);
		glDrawArrays(GL_LINES, 0, (sizeof(varray)/sizeof(*varray))/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
	}

	glColor4ub(255, 255, 255, 255);
}

bool iphone_controls::hittest_button(const rect& area)
{
	setup_rects();
	//static graphics::texture tex(graphics::texture::get("gui/iphone_controls.png"));
	foreach(const Mouse& mouse, active_mice) {
		const point mouse_pos(mouse.x, mouse.y);
		if(point_in_rect(mouse_pos, area)) {
			return true;
		}
	}
	return false;
}

bool iphone_controls::up()
{
	if(is_underwater) {
		return false;
	}

	if(can_interact && hittest_button(interact_button)) {
		return true;
	}

	return hittest_button(up_arrow);
}

bool iphone_controls::down()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(down_arrow) ||
		(on_platform && hittest_button(jumpdown_button)) ||
		(!is_standing && hittest_button(spin_button));
}

bool iphone_controls::left()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(left_arrow);
}

bool iphone_controls::right()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(right_arrow);
}

bool iphone_controls::attack()
{
	return false; //hittest_button(b_button);
}

bool iphone_controls::jump()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(preferences::reverse_ab() ? c_button : a_button) || (on_platform && hittest_button(jumpdown_button));
}

bool iphone_controls::tongue()
{
	return hittest_button(preferences::reverse_ab() ? a_button : c_button);
}

#else // dummy functions for non-iPhone

void translate_mouse_coords (int *x, int *y) {}

void iphone_controls::draw() {}

void iphone_controls::set_underwater(bool value) {}
void iphone_controls::set_can_interact(bool value) {}
void iphone_controls::set_on_platform(bool value) {}
void iphone_controls::set_standing(bool value) {}

bool iphone_controls::water_dir(float* xvalue, float* yvalue) { return false; }

bool iphone_controls::up() {return false;}

bool iphone_controls::down() {return false;}

bool iphone_controls::left() {return false;}

bool iphone_controls::right() {return false;}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump() {return false;}

bool iphone_controls::tongue() {return false;}

void iphone_controls::read_controls() {}

#endif
