#include <list>

#include "font.hpp"
#include "debug_console.hpp"
#include "foreach.hpp"
#include "preferences.hpp"
#include "raster.hpp"

namespace debug_console
{

namespace {
std::list<graphics::texture>& messages() {
	static std::list<graphics::texture> message_queue;
	return message_queue;
}
}

void add_message(const std::string& msg)
{
	if(!preferences::debug()) {
		return;
	}

	const SDL_Color col = {255, 255, 255, 255};
	try {
		messages().push_back(font::render_text(msg, col, 14));
	} catch(font::error& e) {
		std::cerr << "FAILED TO ADD MESSAGE DUE TO FONT RENDERING FAILURE\n";
		return;
	}
	if(messages().size() > 8) {
		messages().pop_front();
	}
}

void draw()
{
	if(messages().empty()) {
		return;
	}

	int ypos = 80;
	foreach(const graphics::texture& t, messages()) {
		const SDL_Rect area = {0, ypos-2, t.width() + 10, t.height() + 5};
		graphics::draw_rect(area, graphics::color_black(), 128);
		graphics::blit_texture(t, 5, ypos);
		ypos += t.height() + 5;
	}
}

}
