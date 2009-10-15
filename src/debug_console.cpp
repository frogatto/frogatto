#include <deque>

#include "font.hpp"
#include "debug_console.hpp"
#include "foreach.hpp"
#include "raster.hpp"

namespace debug_console
{

namespace {
std::deque<graphics::texture> messages;
}

void add_message(const std::string& msg)
{
	const SDL_Color col = {255, 255, 255, 255};
	messages.push_back(font::render_text(msg, col, 14));
	if(messages.size() > 8) {
		messages.pop_front();
	}
}

void draw()
{
	if(messages.empty()) {
		return;
	}

	int ypos = 32;
	foreach(const graphics::texture& t, messages) {
		const SDL_Rect area = {0, ypos-2, t.width() + 10, t.height() + 5};
		graphics::draw_rect(area, graphics::color_black(), 128);
		graphics::blit_texture(t, 5, ypos);
		ypos += t.height() + 5;
	}
}

}
