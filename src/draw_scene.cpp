#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

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
		pos.x += lvl.auto_move_camera_x()*100;
		pos.y += lvl.auto_move_camera_y()*100;
		std::cerr << "auto move: " << lvl.auto_move_camera_x() << "\n";
		const int max_vertical_look = (drawable_height()/3)*(drawable_height()/3);
		const int vertical_look_speed = 300;

		const int min_x = lvl.boundaries().x() + graphics::screen_width()/2;
		const int max_x = lvl.boundaries().x2() - graphics::screen_width()/2;
		const int min_y = lvl.boundaries().y() + drawable_height()/2;
		const int max_y = lvl.boundaries().y2() - drawable_height()/2;
		const int x = std::min(std::max(focus->feet_x(), min_x), max_x);
		const int vertical_look = std::sqrt(std::abs(pos.vertical_look)) * (pos.vertical_look > 0 ? 1 : -1);
		const int y = std::min(std::max(focus->feet_y() - drawable_height()/5 + vertical_look, min_y), max_y);
		const int target_xpos = 100*(x - graphics::screen_width()/2);

		const int target_ypos = (y - drawable_height()/2)*100;

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
			const int horizontal_move_speed = 30;
			const int vertical_move_speed = 10;
			int xdiff = (target_xpos - pos.x)/horizontal_move_speed;
			int ydiff = (target_ypos - pos.y)/vertical_move_speed;

			pos.x += xdiff;
			pos.y += ydiff;
		}
		lvl.draw_background(pos.x/100, pos.y/100, camera_rotation);

		glTranslatef(-pos.x/100, -pos.y/100, 0);
	}
	lvl.draw(pos.x/100, pos.y/100, graphics::screen_width(), drawable_height());
	glPopMatrix();

	if(lvl.player()) {
		graphics::texture statusbar(graphics::texture::get("statusbar.png"));
		graphics::blit_texture(statusbar, 0, 600 - 124, 800, 124,
		                       0.0, 0.0, 0.38, 1.0, 1.0);
		graphics::blit_texture(statusbar, 16, 600 - 124 + 16, 47, 71,
		                       0.0, 0.0, 1.0/100.0, 24.0/400.0, 37.0/100.0);
		for(int hp = 0; hp < lvl.player()->max_hitpoints(); ++hp) {
			const GLfloat is_red = hp >= lvl.player()->hitpoints() ? 15.0 : 0.0;
			graphics::blit_texture(statusbar, 78 + 30*hp, 600 - 124 + 16, 28, 71,
			                       0.0, (69.0 + is_red)/400.0, 1.0/100.0, (83.0 + is_red)/400.0, 37.0/100.0);
		}

		variant coins = lvl.player()->query_value("coins");
		if(!coins.as_bool()) {
			coins = variant(0);
		}
		const_item_type_ptr coin_type = item_type::get("coin");
		if(coin_type) {
			coin_type->get_frame().draw(285, 510, true);
		}

		graphics::blit_texture(font::render_text("x " + coins.string_cast(), graphics::color_black(), 18), 320, 516);
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
