#include <string>
#include <iostream>

#include "loading_screen.hpp"
#include "wml_node.hpp"
#include "custom_object_type.hpp"
#include "wml_utils.hpp"
#include "raster.hpp"
#include "graphical_font.hpp"
#include "geometry.hpp"

loading_screen::loading_screen (int items) : items_(items), status_(0)
{
	std::cerr << "*** Init loading screen\n";
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
	const int bar_height = 20;
	
	graphics::prepare_raster();
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	int bar_origin_x = graphics::screen_width()/2 - bar_width/2;
	rect bg(graphics::screen_width()/2 - bar_width/2 - 1, graphics::screen_height()/2 - bar_height/2 - 1, bar_width+2, bar_height+2);
	graphics::draw_rect(bg, graphics::color(96, 96, 96, 255));
	float amount_done = (float)status_ / (float)items_;
	rect bar(graphics::screen_width()/2 - bar_width/2, graphics::screen_height()/2 - bar_height/2, bar_width*amount_done, bar_height);
	std::cerr << "Drawing loader bar with " << status_ << " of " << items_ << " done - " << bar << "\n";
	graphics::draw_rect(bar, graphics::color(220, 220, 220, 255));
	SDL_GL_SwapBuffers();
}

void loading_screen::increment_status ()
{
	status_++;
}

void loading_screen::set_number_of_items (int items)
{
	items_ = items;
}
