#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "debug_console.hpp"
#include "draw_number.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "level.hpp"
#include "message_dialog.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace {

int drawable_height() {
	const int statusbar_height = 124;
	return graphics::screen_height() - statusbar_height;
}

graphics::texture title_texture_fg, title_texture_bg;
int title_display_remaining = 0;

screen_position last_position;
}

screen_position& last_draw_position()
{
	return last_position;
}

void set_scene_title(const std::string& msg) {
	title_texture_fg = font::render_text(msg, graphics::color_white(), 60);
	title_texture_bg = font::render_text(msg, graphics::color_black(), 60);
	title_display_remaining = 50;
}

GLfloat hp_ratio = -1.0;
void draw_scene(const level& lvl, screen_position& pos, const entity* focus) {
	static int frame_num = 0;
	++frame_num;
	if(focus == NULL) {
		focus = lvl.player().get();
	}

	last_position = pos;

	const int start_time = SDL_GetTicks();
	graphics::prepare_raster();
	glPushMatrix();

	const int camera_rotation = lvl.camera_rotation();
	if(camera_rotation) {
		GLfloat rotate = GLfloat(camera_rotation)/1000.0;
		glRotatef(rotate, 0.0, 0.0, 1.0);
	}

	if(pos.flip_rotate) {
		const SDL_Surface* fb = SDL_GetVideoSurface();
		const double angle = sin(0.5*3.141592653589*GLfloat(pos.flip_rotate)/1000.0);
		const int pixels = (fb->w/2)*angle;

		glViewport(pixels, 0, fb->w - pixels*2, fb->h);
	}

	if(lvl.player()) {
		// If the camera is automatically moved along by the level (e.g. a 
		// hurtling through the sky level) do that here.
		pos.x += lvl.auto_move_camera_x()*100;
		pos.y += lvl.auto_move_camera_y()*100;

		//constants for the maximum that looking up or down can adjust the
		//screen by, and the speed which the target position moves when
		//the player looks up or down.
		const int max_vertical_look = (drawable_height()/3)*(drawable_height()/3);
		const int vertical_look_speed = 300;

		//find the boundary values for the camera position based on the size
		//of the level. These boundaries keep the camera from ever going out
		//of the bounds of the level.
		const int min_x = lvl.boundaries().x() + graphics::screen_width()/2;
		const int max_x = lvl.boundaries().x2() - graphics::screen_width()/2;
		const int min_y = lvl.boundaries().y() + drawable_height()/2;
		const int max_y = lvl.boundaries().y2() - drawable_height()/2;

		//find the point we want the camera to converge toward. It will be the
		//feet of the player, but inside the boundaries we calculated above.
		const int x = std::min(std::max(focus->feet_x(), min_x), max_x);

		//calculate the adjustment to the camera's target position based on
		//our vertical look. This is calculated as the square root of the
		//vertical look, to make the movement slowly converge.
		const int vertical_look = std::sqrt(std::abs(pos.vertical_look)) * (pos.vertical_look > 0 ? 1 : -1);

		//find the y point for the camera to converge toward
		const int y = std::min(std::max(focus->feet_y() - drawable_height()/5 + vertical_look, min_y), max_y);

		//find the target x,y position of the camera in centi-pixels. Note that
		//(x,y) represents the position the camera should center on, while
		//now we're calculating the top-left point.
		//
		//the actual camera position will converge toward this point
		const int target_xpos = 100*(x - graphics::screen_width()/2);
		const int target_ypos = 100*(y - drawable_height()/2);

		//adjust the vertical look according to if the focus is looking up/down
		if(message_dialog::get() == NULL && focus->look_up()) {
			pos.vertical_look -= vertical_look_speed;
			if(pos.vertical_look < -max_vertical_look) {
				pos.vertical_look = -max_vertical_look;
			}
		} else if(message_dialog::get() == NULL && focus->look_down()) {
			pos.vertical_look += vertical_look_speed;
			if(pos.vertical_look > max_vertical_look) {
				pos.vertical_look = max_vertical_look;
			}
		} else {
			if(pos.vertical_look > 0) {
				pos.vertical_look -= vertical_look_speed;
				if(pos.vertical_look < 0) {
					pos.vertical_look = 0;
				}
			} else if(pos.vertical_look < 0) {
				pos.vertical_look += vertical_look_speed;
				if(pos.vertical_look > 0) {
					pos.vertical_look = 0;
				}
			}
		}
		
		
		

		if(pos.init == false) {
			pos.x = target_xpos;
			pos.y = target_ypos;
			pos.init = true;
		} else {
			//Make (pos.x, pos.y) converge toward (target_xpos,target_ypos).
			//We do this by moving asymptotically toward the target, which
			//makes the camera have a nice acceleration/decceleration effect
			//as the target position moves.
			const int horizontal_move_speed = 30;
			const int vertical_move_speed = 10;
			int xdiff = (target_xpos - pos.x)/horizontal_move_speed;
			int ydiff = (target_ypos - pos.y)/vertical_move_speed;

			pos.x += xdiff;
			pos.y += ydiff;
		}
		
		
		//shake decay is handled automatically; just by giving it an offset and velocity,
		//it will automatically return to equilibrium
		
		//shake speed		
		pos.x += (pos.shake_x_offset);
		pos.y += (pos.shake_y_offset);
		
		//shake velocity
		pos.shake_x_offset += pos.shake_x_vel;
		pos.shake_y_offset += pos.shake_y_vel;
			
		//shake acceleration
		if ((std::abs(pos.shake_x_vel) < 50) && (std::abs(pos.shake_x_offset) < 50)){
			//prematurely end the oscillation if it's in the asymptote
			pos.shake_x_offset = 0;
			pos.shake_x_vel = 0;
		}else{
			//extraneous signs kept for consistency with conventional spring physics, also
			//the value that "offset" is divided by, is (the inverse of) 'k', aka "spring stiffness"
			//the value that "velocity" is divided by, is (the inverse of) 'b', aka "oscillation damping",
			//which causes the spring to come to rest.
			//These values are very sensitive, and tweaking them wrongly will cause the spring to 'explode',
			//and increase its motion out of game-bounds. 
			if (pos.shake_x_offset > 0){
				pos.shake_x_vel -= (1 * pos.shake_x_offset/3 + pos.shake_x_vel/15);
			}else if(pos.shake_x_offset < 0) {
				pos.shake_x_vel += (-1 * pos.shake_x_offset/3 - pos.shake_x_vel/15);
			}
		}
		if ((std::abs(pos.shake_y_vel) < 50) && (std::abs(pos.shake_y_offset) < 50)){
			//prematurely end the oscillation if it's in the asymptote
			pos.shake_y_offset = 0;
			pos.shake_y_vel = 0;
		}else{
			if (pos.shake_y_offset > 0){
				pos.shake_y_vel -= (1 * pos.shake_y_offset/3 + pos.shake_y_vel/15);
			}else if(pos.shake_y_offset < 0) {
				pos.shake_y_vel += (-1 * pos.shake_y_offset/3 - pos.shake_y_vel/15);
			}
		}

		glTranslatef(-pos.x/100, -pos.y/100, 0);
		lvl.draw_background(pos.x/100, pos.y/100, camera_rotation);
	}
	lvl.draw(pos.x/100, pos.y/100, graphics::screen_width(), drawable_height());
	graphics::clear_raster_distortion();
	glPopMatrix();

	debug_console::draw();

	draw_statusbar(lvl, pos, focus);
}

void draw_statusbar(const level& lvl, screen_position& pos, const entity* focus)
{
	static int frame_num = 0;
	frame_num++;

	graphics::texture statusbar(graphics::texture::get("statusbar.png"));

	const_pc_character_ptr player = lvl.player();
	if(player) {
		if(player->driver()) {
			player = player->driver();
		}

		graphics::blit_texture(statusbar, 0, 600 - 124, 800, 124,
		                       0.0, 0.0, 0.0, 1.0, 62.0/104.0);

		player->icon_frame().draw(210, 600 - 124 + 14, true);
		for(int hp = 0; hp < player->max_hitpoints(); ++hp) {
			GLfloat shift = 0.0; // default to green
			if(hp >= player->hitpoints()) {
				//missing hitpoints, display as red
				shift = 30.0;
			} else if(player->hitpoints() - hp <= player->num_powerups()) {
				//powerup, display as yellow
				shift = 15.0;

				//make the red component fade in and out in a wave pattern
				const GLfloat frame = frame_num/10.0;
				glColor4f(sin(frame), 1.0, 0.0, 1.0);
			}
			graphics::blit_texture(statusbar, 278 + 30*hp, 600 - 124 + 16, 30, 74,
			                       0.0, (99.0 + shift)/400.0, 62.0/103.0, (114.0 + shift)/400.0, 99.0/103.0);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}

		variant coins = player->query_value("coins");
		if(!coins.as_bool()) {
			coins = variant(0);
		}

		const int wanted_coins = coins.as_int()*100;
		if(pos.coins < 0 || pos.coins > wanted_coins) {
			pos.coins = wanted_coins;
		}

		if(pos.coins < wanted_coins) {
			const int delta = (wanted_coins - pos.coins)/20;
			pos.coins += delta;
			if(delta == 0) {
				++pos.coins;
			}
		}

		draw_number(pos.coins, 3, 267*2, 600 - 124 + 11*2);

		const int score = player->score();

		const int wanted_score = player->score()*100;
		if(pos.score < 0 || pos.score > wanted_score) {
			pos.score = wanted_score;
		}

		if(pos.score < wanted_score) {
			const int delta = (wanted_score - pos.score)/20;
			pos.score += delta;
			if(delta == 0) {
				++pos.score;
			}
		}

		draw_number(pos.score, 7, 232*2, 600 - 124 + 49*2);
	}

	if(lvl.player() && lvl.player()->driver()) {
		const_character_ptr vehicle = lvl.player();
		vehicle->icon_frame().draw(18, 600 - 124 + 22, true);
		for(int hp = 0; hp < vehicle->max_hitpoints(); ++hp) {
			const GLfloat is_red = hp >= vehicle->hitpoints() ? 11.0 : 0.0;
			graphics::blit_texture(statusbar, 90 + 22*hp, 600 - 124 + 22, 20, 28,
			                       0.0, (375.0 + is_red)/400.0, 68.0/103.0, (385.0 + is_red)/400.0, 82.0/103.0);
		}
		
		// draw the vehicle's level and experience.
		variant xp_num = vehicle->query_value("experience");
		if(!xp_num.as_bool()) {
			xp_num = variant(0);
		}

		const int xp_needed = 7;
		int xp = xp_num.as_int();
		const int xp_level = xp/xp_needed;
		xp %= xp_needed;

		graphics::blit_texture(statusbar, 73*2, 600 - 124 + 46*2, 14*2, 8*2,
		                       0.0, 9.0/400.0, (80.0 - 9.0*(xp_level))/103.0,
		                       23.0/400.0, (88.0 - 9.0*(xp_level))/103.0);

		for(int n = 0; n != xp_needed - 1; ++n) {
			const double has_xp = (n < xp) ? 0.0 : 9.0;
			graphics::blit_texture(statusbar, 14*2 + 10*2*n, 600 - 124 + 46*2 + 1, 8*2, 8*2, 0.0,
			                       (49.0 + has_xp)/400.0, 81.0/103.0,
								   (57.0 + has_xp)/400.0, 89.0/103.0);
		}
	}

	if(title_display_remaining > 0) {
		const int xpos = graphics::screen_width()/2 - title_texture_fg.width()/2;
		const int ypos = graphics::screen_height()/2 - title_texture_fg.height()/2;

		glColor4f(1, 1, 1, title_display_remaining > 25 ? 1.0 : title_display_remaining/25.0);
		for(int x = -1; x <= 1; ++x) {
			for(int y = -1; y <= 1; ++y) {
				graphics::blit_texture(title_texture_bg, xpos+x*2, ypos+y*2);
			}
		}
		graphics::blit_texture(title_texture_fg, xpos, ypos);
		if(--title_display_remaining == 0) {
			title_texture_fg = graphics::texture();
			title_texture_bg = graphics::texture();
		}

		glColor4f(1, 1, 1, 1);
	}

	if(message_dialog::get()) {
		message_dialog::get()->draw();
	}
}
