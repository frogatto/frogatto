#include "graphics.hpp"

#include <ctype.h>

#include "raster.hpp"
#include "string_utils.hpp"
#include "text_entry_widget.hpp"

namespace gui {

text_entry_widget::text_entry_widget() : scroll_(0), cursor_(0), font_(graphical_font::get("default"))
{
	SDL_EnableUNICODE(1);
}

const std::string& text_entry_widget::text() const
{
	return text_;
}

void text_entry_widget::set_text(const std::string& value)
{
	text_ = value;
	cursor_ = text_.size();
}

void text_entry_widget::set_font(const std::string& name)
{
	font_ = graphical_font::get(name);
}

void text_entry_widget::handle_draw() const
{
	rect r = font_->draw(x(), y(), std::string(text_.begin(), text_.begin() + cursor_));
	if(cursor_ == 0) {
		r = rect(r.x() - 4, r.y(), 0, 0);
	}
	const SDL_Rect cursor_rect = {r.x2() + 2, y(), 2, 28};
	graphics::draw_rect(cursor_rect, graphics::color_white());
	font_->draw(r.x2() + 4, y(), std::string(text_.begin() + cursor_, text_.end()));
}

bool text_entry_widget::handle_event(const SDL_Event& event, bool claimed)
{
	if(claimed) {
		return claimed;
	}

	switch(event.type) {
	case SDL_KEYDOWN: {

		switch(event.key.keysym.sym) {
		case SDLK_LEFT:
			if(cursor_ > 0) {
				--cursor_;
			}
			break;
		case SDLK_RIGHT:
			if(cursor_ < text_.size()) {
				++cursor_;
			}
			break;
		case SDLK_BACKSPACE:
			if(cursor_ > 0) {
				--cursor_;
				text_.erase(text_.begin() + cursor_);
			}
			break;
		case SDLK_DELETE:
			if(cursor_ < text_.size()) {
				text_.erase(text_.begin() + cursor_);
			}
			break;
		}

		char c = event.key.keysym.unicode;
		if(util::c_isprint(c)) {
			text_.insert(text_.begin() + cursor_, c);
			++cursor_;
			claimed = true;
		}
		break;
	}
	}

	return claimed;
}

}
