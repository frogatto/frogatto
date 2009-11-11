#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "controls.hpp"
#include "debug_console.hpp"
#include "draw_number.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "level.hpp"
#include "message_dialog.hpp"
#include "player_info.hpp"
#include "raster.hpp"
#include "speech_dialog.hpp"
#include "texture.hpp"

namespace {

int drawable_height() {
	const int statusbar_height = 0;
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

void set_scene_title(const std::string& msg, int duration) {
	title_texture_fg = font::render_text(msg, graphics::color_white(), 60);
	title_texture_bg = font::render_text(msg, graphics::color_black(), 60);
	title_display_remaining = duration;
}

GLfloat hp_ratio = -1.0;
void draw_scene(const level& lvl, screen_position& pos, const entity* focus) {
	static int frame_num = 0;
	++frame_num;
	if(focus == NULL && lvl.player()) {
		focus = &lvl.player()->get_entity();
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

	if(focus) {
		// If the camera is automatically moved along by the level (e.g. a 
		// hurtling through the sky level) do that here.
		pos.x += lvl.auto_move_camera_x()*100;
		pos.y += lvl.auto_move_camera_y()*100;

		//find how much padding will have to be on the edge of the screen due
		//to the level being wider than the screen. This value will be 0
		//if the level is larger than the screen (i.e. most cases)
		const int x_screen_pad = std::max<int>(0, graphics::screen_width() - lvl.boundaries().w());

		//find the boundary values for the camera position based on the size
		//of the level. These boundaries keep the camera from ever going out
		//of the bounds of the level.
		const int min_x = lvl.boundaries().x() + graphics::screen_width()/2 - x_screen_pad/2;
		const int max_x = lvl.boundaries().x2() - graphics::screen_width()/2 + x_screen_pad/2;
		const int min_y = lvl.boundaries().y() + drawable_height()/2;
		const int max_y = lvl.boundaries().y2() - drawable_height()/2;

		//we look a certain number of frames ahead -- assuming the focus
		//keeps moving at the current velocity, we converge toward the point
		//they will be at in x frames.
		const int PredictiveFramesHorz = 20;
		const int PredictiveFramesVert = 5;

		int displacement_x = 0, displacement_y = 0;
		if(pos.focus_x || pos.focus_y) {
			displacement_x = focus->feet_x() - pos.focus_x;
			displacement_y = focus->feet_y() - pos.focus_y;
		}

		pos.focus_x = focus->feet_x();
		pos.focus_y = focus->feet_y();

		//find the point we want the camera to converge toward. It will be the
		//feet of the player, but inside the boundaries we calculated above.
		int x = std::min(std::max(focus->feet_x() + displacement_x*PredictiveFramesHorz, min_x), max_x);

		//calculate the adjustment to the camera's target position based on
		//our vertical look. This is calculated as the square root of the
		//vertical look, to make the movement slowly converge.
		const int vertical_look = focus->vertical_look();

		//find the y point for the camera to converge toward
		int y = std::min(std::max(focus->feet_y() - drawable_height()/5 + displacement_y*PredictiveFramesVert + vertical_look, min_y), max_y);

		if(lvl.lock_screen()) {
			x = lvl.lock_screen()->x;
			y = lvl.lock_screen()->y;
		}

		//find the target x,y position of the camera in centi-pixels. Note that
		//(x,y) represents the position the camera should center on, while
		//now we're calculating the top-left point.
		//
		//the actual camera position will converge toward this point
		const int target_xpos = 100*(x - graphics::screen_width()/2);
		const int target_ypos = 100*(y - drawable_height()/2);

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

	lvl.draw_status();
}

void draw_fps(const level& lvl, const performance_data& data)
{
	std::ostringstream s;
	s << data.fps << "fps; " << (data.draw/10) << "% draw; " << (data.process/10) << "% process; " << (data.delay/10) << "% idle; " << lvl.num_active_chars() << " objects; " << 50*(data.cycle/50) << " cycles";
	graphics::texture t(font::render_text(s.str(), graphics::color_white(), 14));
	graphics::blit_texture(t, 10, 60);

	if(controls::num_players() > 1) {
		//draw networking stats
		std::ostringstream s;
		s << controls::packets_received() << " packets received; " << controls::num_errors() << " errors; " << controls::cycles_behind() << " behind; " << controls::their_highest_confirmed() << " remote cycles " << controls::last_packet_size() << " packet";
		graphics::texture t(font::render_text(s.str(), graphics::color_white(), 14));
		graphics::blit_texture(t, 10, 24);
	}
}
