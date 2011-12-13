#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA)
#include <GLES/gl.h>
#ifdef TARGET_PANDORA
#include <GLES/glues.h>
#endif
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "asserts.hpp"
#include "controls.hpp"
#include "debug_console.hpp"
#include "draw_number.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "globals.h"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "i18n.hpp"
#include "level.hpp"
#include "message_dialog.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "speech_dialog.hpp"
#include "texture.hpp"
#include "texture_frame_buffer.hpp"

namespace {

std::vector<rect> current_debug_rects;

std::string& scene_title() {
	static std::string title;
	return title;
}

achievement_ptr current_achievement;
int current_achievement_duration = 0;

	
struct screen_flash {
	graphics::color_transform color, delta;
	int duration;
};

std::vector<screen_flash>& flashes() {
	static std::vector<screen_flash> obj;
	return obj;
}

int scene_title_duration_;

screen_position last_position;
}

bool is_achievement_displayed() {
	return current_achievement && current_achievement_duration > 0;
}

screen_position& last_draw_position()
{
	return last_position;
}

void screen_color_flash(const graphics::color_transform& color, const graphics::color_transform& color_delta, int duration)
{
	screen_flash f = { color, color_delta, duration };
	flashes().push_back(f);
}

void set_scene_title(const std::string& msg, int duration) {
	//explicitly translate all level titles
	scene_title() = (msg.size() > 0) ? _(msg) : msg;
	scene_title_duration_ = duration;
}

void set_displayed_achievement(achievement_ptr a)
{
	current_achievement = a;
	current_achievement_duration = 250;
}

GLfloat hp_ratio = -1.0;
void draw_scene(const level& lvl, screen_position& pos, const entity* focus, bool do_draw) {
	const bool draw_ready = update_camera_position(lvl, pos, focus, do_draw);
	if(draw_ready) {
		render_scene(lvl, pos, focus, do_draw);
	}
}

bool update_camera_position(const level& lvl, screen_position& pos, const entity* focus, bool do_draw) {
	if(focus == NULL && lvl.player()) {
		focus = &lvl.player()->get_entity();
	}

	//flag which gets set to false if we abort drawing, due to the
	//screen position being initialized now.
	const bool draw_level = do_draw && pos.init;

	if(focus) {
		// If the camera is automatically moved along by the level (e.g. a 
		// hurtling through the sky level) do that here.
		pos.x += lvl.auto_move_camera_x()*100;
		pos.y += lvl.auto_move_camera_y()*100;

		//find how much padding will have to be on the edge of the screen due
		//to the level being wider than the screen. This value will be 0
		//if the level is larger than the screen (i.e. most cases)
		const int x_screen_pad = std::max<int>(0, graphics::screen_width() - lvl.boundaries().w());

		const int y_screen_pad = std::max<int>(0, graphics::screen_height() - lvl.boundaries().h());

		//find the boundary values for the camera position based on the size
		//of the level. These boundaries keep the camera from ever going out
		//of the bounds of the level.
		const int min_x = lvl.boundaries().x() + graphics::screen_width()/2 - x_screen_pad/2;
		const int max_x = lvl.boundaries().x2() - graphics::screen_width()/2 + x_screen_pad/2;
		const int min_y = lvl.boundaries().y() + graphics::screen_height()/2 - y_screen_pad/2;
		const int max_y = lvl.boundaries().y2() - graphics::screen_height()/2 + y_screen_pad/2;

		//std::cerr << "BOUNDARIES: " << lvl.boundaries().x() << ", " << lvl.boundaries().x2() << " WIDTH: " << graphics::screen_width() << " PAD: " << x_screen_pad << "\n";

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
		int y = std::min(std::max(focus->feet_y() - graphics::screen_height()/(5*lvl.zoom_level()) + displacement_y*PredictiveFramesVert + vertical_look, min_y), max_y);

		//std::cerr << "POSITION: " << x << "," << y << " IN " << min_x << "," << min_y << "," << max_x << "," << max_y << "\n";

		if(lvl.focus_override().empty() == false) {
			std::vector<entity_ptr> v = lvl.focus_override();
			int left = 0, right = 0, top = 0, bottom = 0;
			while(v.empty() == false) {
				left = v[0]->feet_x();
				right = v[0]->feet_x();
				top = v[0]->feet_y();
				bottom = v[0]->feet_y();
				foreach(const entity_ptr& e, v) {
					left = std::min<int>(e->feet_x(), left);
					right = std::max<int>(e->feet_x(), right);
					top = std::min<int>(e->feet_y(), top);
					bottom = std::min<int>(e->feet_y(), bottom);
				}

				const int BorderSize = 20;
				if(v.size() == 1 || right - left < graphics::screen_width()/lvl.zoom_level() - BorderSize && bottom - top < graphics::screen_height()/lvl.zoom_level() - BorderSize) {
					break;
				}

				break;

				v.pop_back();
			}

			x = std::min(std::max((left + right)/2, min_x), max_x);
			y = std::min(std::max((top + bottom)/2 - graphics::screen_height()/(5*lvl.zoom_level()), min_y), max_y);
		}


		//std::cerr << "POSITION2: " << x << "," << y << " IN " << min_x << "," << min_y << "," << max_x << "," << max_y << "\n";
		if(lvl.lock_screen()) {
			x = lvl.lock_screen()->x;
			y = lvl.lock_screen()->y;
		}
		//std::cerr << "POSITION3: " << x << "," << y << " IN " << min_x << "," << min_y << "," << max_x << "," << max_y << "\n";

		//for small screens the speech dialog arrows cover the entities they are
		//pointing to. adjust to that by looking up a little bit.
		if (lvl.current_speech_dialog() && preferences::virtual_screen_height() < 600)
			y = std::min(y + (600 - graphics::screen_height())/(2*lvl.zoom_level()), max_y);

		//find the target x,y position of the camera in centi-pixels. Note that
		//(x,y) represents the position the camera should center on, while
		//now we're calculating the top-left point.
		//
		//the actual camera position will converge toward this point
		const int target_xpos = 100*(x - graphics::screen_width()/2);
		const int target_ypos = 100*(y - graphics::screen_height()/2);

		if(pos.init == false) {
			pos.x = target_xpos;
			pos.y = target_ypos;
			pos.init = true;
		} else {
			//Make (pos.x, pos.y) converge toward (target_xpos,target_ypos).
			//We do this by moving asymptotically toward the target, which
			//makes the camera have a nice acceleration/decceleration effect
			//as the target position moves.
			const int horizontal_move_speed = 30/lvl.zoom_level();
			const int vertical_move_speed = 10/lvl.zoom_level();
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

		const int target_zoom = lvl.zoom_level();
		const float ZoomSpeed = 0.03;
		if(std::abs(target_zoom - pos.zoom) < ZoomSpeed) {
			pos.zoom = target_zoom;
		} else if(pos.zoom > target_zoom) {
			pos.zoom -= ZoomSpeed;
		} else {
			pos.zoom += ZoomSpeed;
		}
	}

	last_position = pos;

	return draw_level;
}

void render_scene(const level& lvl, screen_position& pos, const entity* focus, bool do_draw) {
	graphics::prepare_raster();
	glPushMatrix();

	const int camera_rotation = lvl.camera_rotation();
	if(camera_rotation) {
		GLfloat rotate = GLfloat(camera_rotation)/1000.0;
		glRotatef(rotate, 0.0, 0.0, 1.0);
	}

	if(pos.flip_rotate) {
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		const SDL_Surface* fb = SDL_GetVideoSurface();
		const double angle = sin(0.5*3.141592653589*GLfloat(pos.flip_rotate)/1000.0);
		const int pixels = (preferences::actual_screen_width()/2)*angle;
		
		//then squish all future drawing inwards
		glViewport(pixels, 0, preferences::actual_screen_width() - pixels*2, preferences::actual_screen_height());
	}

	int xscroll = (pos.x/100)&preferences::xypos_draw_mask;
	int yscroll = (pos.y/100)&preferences::xypos_draw_mask;

	if(pos.zoom > 1.0) {
		glScalef(pos.zoom, pos.zoom, 0);
		xscroll += (graphics::screen_width()/2)*(-1.0/pos.zoom + 1.0);
		yscroll += (graphics::screen_height()/2)*(-1.0/pos.zoom + 1.0);
	}

	glTranslatef(-xscroll, -yscroll, 0);
	lvl.draw_background(xscroll, yscroll, camera_rotation);

	lvl.draw(xscroll, yscroll, graphics::screen_width(), graphics::screen_height());

	foreach(const rect& r, current_debug_rects) {
		graphics::draw_rect(r, graphics::color(0, 0, 255, 175));
	}

	current_debug_rects.clear();

	graphics::clear_raster_distortion();
	glPopMatrix();

	for(std::vector<screen_flash>::iterator i = flashes().begin();
	    i != flashes().end(); ) {
		const graphics::color& tint = i->color.to_color();
		if(tint.a() > 0) {
			graphics::draw_rect(rect(0, 0, graphics::screen_width(), graphics::screen_height()), tint);
		}

		i->color = graphics::color_transform(i->color.r() + i->delta.r(),
		                                     i->color.g() + i->delta.g(),
		                                     i->color.b() + i->delta.b(),
		                                     i->color.a() + i->delta.a());

		if(--i->duration <= 0) {
			i = flashes().erase(i);
		} else {
			++i;
		}
	}

	debug_console::draw();

	if (!pause_stack) lvl.draw_status();

	if(scene_title_duration_ > 0) {
		--scene_title_duration_;
		const const_graphical_font_ptr f = graphical_font::get("default");
		ASSERT_LOG(f.get() != NULL, "COULD NOT LOAD DEFAULT FONT");
		const rect r = f->dimensions(scene_title());
		const GLfloat alpha = scene_title_duration_ > 10 ? 1.0 : scene_title_duration_/10.0;
		{
			glColor4ub(0, 0, 0, 128*alpha);
			f->draw(graphics::screen_width()/2 - r.w()/2 + 2, graphics::screen_height()/2 - r.h()/2 + 2, scene_title());
			glColor4ub(255, 255, 255, 255*alpha);
		}

		{
			f->draw(graphics::screen_width()/2 - r.w()/2, graphics::screen_height()/2 - r.h()/2, scene_title());
			glColor4ub(255, 255, 255, 255);
		}
	}
	
	if(current_achievement && current_achievement_duration > 0) {
		--current_achievement_duration;

		const_gui_section_ptr left = gui_section::get("achievements_left");
		const_gui_section_ptr right = gui_section::get("achievements_right");
		const_gui_section_ptr main = gui_section::get("achievements_main");

		const const_graphical_font_ptr title_font = graphical_font::get("white_outline");
		const const_graphical_font_ptr main_font = graphical_font::get("door_label");

		const std::string title_text = _("Achievement Unlocked!");
		const std::string name = current_achievement->name();
		const std::string description = "(" + current_achievement->description() + ")";
		const int width = std::max<int>(std::max<int>(
		  title_font->dimensions(title_text).w(),
		  main_font->dimensions(name).w()),
		  main_font->dimensions(description).w()
		  ) + 8;
		
		const int xpos = graphics::screen_width() - 16 - left->width() - right->width() - width;
		const int ypos = 16;

		const GLfloat alpha = current_achievement_duration > 10 ? 1.0 : current_achievement_duration/10.0;

		glColor4f(1.0, 1.0, 1.0, alpha);

		left->blit(xpos, ypos);
		main->blit(xpos + left->width(), ypos, width, main->height());
		right->blit(xpos + left->width() + width, ypos);

		title_font->draw(xpos + left->width(), ypos - 10, title_text);
		main_font->draw(xpos + left->width(), ypos + 24, name);

		glColor4f(0.0, 1.0, 0.0, alpha);
		main_font->draw(xpos + left->width(), ypos + 48, description);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		
	}
	
	if(pos.flip_rotate) {
		const SDL_Surface* fb = SDL_GetVideoSurface();
		const double angle = sin(0.5*3.141592653589*GLfloat(pos.flip_rotate)/1000.0);
		//const int pixels = (fb->w/2)*angle;
		const int pixels = (preferences::actual_screen_width()/2)*angle;
		
		
		//first draw black over the sections of the screen which aren't to be drawn to
		//GLshort varray1[8] = {0,0,  pixels,0,  pixels,fb->h,   0,fb->h};
		//GLshort varray2[8] = {fb->w - pixels,0,  fb->w,0,   fb->w,fb->h,  fb->w - pixels,fb->h};
		GLshort varray1[8] = {0,0,  pixels,0,  pixels,preferences::actual_screen_height(),   0,preferences::actual_screen_height()};
		GLshort varray2[8] = {preferences::actual_screen_width() - pixels,0,  preferences::actual_screen_width(),0,   preferences::actual_screen_width(),preferences::actual_screen_height(),  preferences::actual_screen_width() - pixels,preferences::actual_screen_height()};
		glColor4ub(0, 0, 0, 255);
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);		
		
		//glViewport(0, 0, fb->w, fb->h);
		glViewport(0, 0, preferences::actual_screen_width(), preferences::actual_screen_height());
		glVertexPointer(2, GL_SHORT, 0, &varray1);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glVertexPointer(2, GL_SHORT, 0, &varray2);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4ub(255, 255, 255, 255);
	}
}

void draw_fps(const level& lvl, const performance_data& data)
{
	if(!preferences::debug()) {
		return;
	}

	const_graphical_font_ptr font(graphical_font::get("door_label"));
	if(!font) {
		return;
	}
	std::ostringstream s;
	s << data.fps << "/" << data.cycles_per_second << "fps; " << (data.draw/10) << "% draw; " << (data.flip/10) << "% flip; " << (data.process/10) << "% process; " << (data.delay/10) << "% idle; " << lvl.num_active_chars() << " objects; " << data.nevents << " events";

	rect area = font->draw(10, 60, s.str());

	if(controls::num_players() > 1) {
		//draw networking stats
		std::ostringstream s;
		s << controls::packets_received() << " packets received; " << controls::num_errors() << " errors; " << controls::cycles_behind() << " behind; " << controls::their_highest_confirmed() << " remote cycles " << controls::last_packet_size() << " packet";

		area = font->draw(10, area.y2() + 5, s.str());
	}

	if(!data.profiling_info.empty()) {
		font->draw(10, area.y2() + 5, data.profiling_info);
	}
}

void add_debug_rect(const rect& r)
{
	current_debug_rects.push_back(r);
}
