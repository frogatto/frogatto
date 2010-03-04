#include <string>
#include <iostream>

#include "loading_screen.hpp"
#include "wml_node.hpp"
#include "custom_object_type.hpp"
#include "wml_utils.hpp"
#include "raster.hpp"
#include "graphical_font.hpp"
#include "geometry.hpp"
#include "texture.hpp"

loading_screen::loading_screen (int items) : items_(items), status_(0)
{
	background_ = graphics::texture::get("backgrounds/loading_screen.png");
}

void loading_screen::load (wml::const_node_ptr node)
{
	//custom_object_type::get("frogatto_playable");
	FOREACH_WML_CHILD(preload_node, node, "preload")
	{
		draw_and_increment(wml::get_str(preload_node, "message"));
		custom_object_type::get(wml::get_str(preload_node, "name"));
	}
}

void loading_screen::draw (const std::string& message)
{
	std::cerr << "*** Drawing loading screen with message: " << message << "\n";
	const int bar_width = 100;
	const int bar_height = 10;
	int screen_w = graphics::screen_width();
	int screen_h = graphics::screen_height();
	
	graphics::prepare_raster();
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	int bg_w = background_.width();
	int bg_h = background_.height();
	graphics::blit_texture(background_, screen_w/2-bg_w, screen_h/2-bg_h, bg_w*2, bg_h*2);
	
	int bar_origin_x = graphics::screen_width()/2 - bar_width/2;
	rect bg(screen_w/2 - bar_width/2, screen_h/2 - bar_height/2, bar_width, bar_height);
	graphics::draw_rect(bg, graphics::color(96, 96, 96, 255));
	float amount_done = (float)status_ / (float)items_;
	rect bar(screen_w/2 - bar_width/2, screen_h/2 - bar_height/2, bar_width*amount_done, bar_height);
	graphics::draw_rect(bar, graphics::color(220, 220, 220, 255));
	
	const_graphical_font_ptr font = graphical_font::get("door_label");
	rect text_size = font->dimensions(message);
	font->draw(screen_w/2 - text_size.w()/2, screen_h/2 + bar_height/2 + 5, message);
	
	SDL_GL_SwapBuffers();
	//SDL_Delay(500); //make it possible to see on fast computers; for debugging
}

void loading_screen::increment_status ()
{
	status_++;
}

void loading_screen::set_number_of_items (int items)
{
	items_ = items;
}
