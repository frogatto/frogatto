#include <ctype.h>
#include <iostream>

#include "font.hpp"
#include "foreach.hpp"
#include "message_dialog.hpp"
#include "raster.hpp"
#include "string_utils.hpp"

namespace {
const int FontSize = 22;

std::string::const_iterator get_end_of_word(std::string::const_iterator i1,
                                            std::string::const_iterator i2)
{
	while(i1 != i2 && util::c_isspace(*i1)) {
		++i1;
	}

	while(i1 != i2 && !util::c_isspace(*i1)) {
		++i1;
	}

	return i1;
}


std::string::const_iterator get_line(std::string::const_iterator i1,
                                     std::string::const_iterator i2,
									 int max_chars)
{
	std::string::const_iterator begin = i1;
	i1 = get_end_of_word(i1, i2);
	if(i1 == i2) {
		return i1;
	}

	while(i1 != i2 && *i1 != '\n' &&  get_end_of_word(i1, i2) - begin < max_chars) {
		i1 = get_end_of_word(i1, i2);
	}

	return i1;
}

message_dialog* current_dialog = NULL;

}

void message_dialog::show_modal(const std::string& text, const std::vector<std::string>* options)
{
	if(current_dialog) {
		clear_modal();
	}

	const int Width = 650;
	const int Height = font::char_height(FontSize)*3;
	current_dialog = new message_dialog(text, rect(graphics::screen_width()/2 - Width/2, graphics::screen_height()/2 - Height/2, Width, Height), options);
}

void message_dialog::clear_modal()
{
	delete current_dialog;
	current_dialog = NULL;
}

message_dialog* message_dialog::get()
{
	return current_dialog;
}

message_dialog::message_dialog(const std::string& text, const rect& pos,
                               const std::vector<std::string>* options)
  : text_(text), pos_(pos), line_height_(0),
    cur_row_(0), cur_char_(0), cur_wait_(0),
	selected_option_(0)
{
	line_height_ = font::char_height(FontSize);
	viewable_lines_ = pos.h()/line_height_;
	if(viewable_lines_ < 1) {
		viewable_lines_ = 1;
	}

	const int max_chars_on_line = std::max<int>(1, pos.w()/font::char_width(FontSize));
	std::string::const_iterator i1 = text.begin();
	std::string::const_iterator i2 = i1;
	while(i2 != text.end()) {
		i2 = get_line(i2, text.end(), max_chars_on_line);
		if(i2 == i1) {
			break;
		}

		while(i1 != i2 && util::c_isspace(*i1)) {
			++i1;
		}

		lines_.push_back(font::render_text(std::string(i1, i2), graphics::color_black(), FontSize));
		i1 = i2;
	}

	if(options != NULL) {
		foreach(const std::string& option, *options) {
			options_.push_back(font::render_text(option, graphics::color_black(), FontSize));
		}
	}
}

namespace {

void draw_frame(const rect& r)
{
	const SDL_Color border = { 0xa2, 0x64, 0x76, 0xff };
	const SDL_Color bg = { 0xbe, 0xa2, 0x8f, 0xff };

	const int Border = 4;
	const int Padding = 10;
	rect border_rect(r.x() - Padding - Border, r.y() - Padding - Border, r.w() + + Padding*2 + Border*2, r.h() + Padding*2 + Border*2);
	graphics::draw_rect(border_rect.sdl_rect(), border);
	rect back_rect(r.x() - Padding, r.y() - Padding, r.w() + Padding*2, r.h() + Padding*2);
	graphics::draw_rect(back_rect.sdl_rect(), bg);
}

}

void message_dialog::draw() const
{
	draw_frame(pos_);

	for(int n = 0; n <= cur_row_ && n < lines_.size(); ++n) {
		if(n != cur_row_) {
			graphics::blit_texture(lines_[n], pos_.x(), pos_.y() + n*line_height_);
		} else {
			const int width = cur_char_*font::char_width(FontSize);
			const int height = lines_[n].height();

			const double x2 = double(width)/double(lines_[n].width());
			graphics::blit_texture(lines_[n], pos_.x(), pos_.y() + n*line_height_,
			                       width, height, 0.0, 0.0, 0.0, x2, 1.0);
		}
	}

	if(cur_row_ >= lines_.size() && !options_.empty()) {
		const int CursorWidth = 8;

		int width = 0;
		int height = 0;
		foreach(const graphics::texture& t, options_) {
			if(t.width() > width) {
				width = t.width();
			}

			height += t.height();
		}

		rect r(pos_.x2() - 100, pos_.y2(), width + CursorWidth, height);
		draw_frame(r);

		for(int n = 0; n != options_.size(); ++n) {
			graphics::blit_texture(options_[n], r.x() + CursorWidth, r.y() + n*line_height_);

			if(n == selected_option_) {
				int xpos = r.x() + CursorWidth;
				const int ypos = r.y() + n*line_height_ + line_height_/2;
				int height = 1;
				while(xpos > r.x()) {
					graphics::draw_rect(rect(xpos, ypos - height, 1, height*2).sdl_rect(), graphics::color_black());
					--xpos;
					++height;
				}
			}
		}
	}
}

void message_dialog::process()
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				if(options_.empty() == false) {
					if(event.key.keysym.sym == SDLK_DOWN) {
						selected_option_++;
						if(selected_option_ == options_.size()) {
							selected_option_ = 0;
						}
						break;
					}
					if(event.key.keysym.sym == SDLK_UP) {
						selected_option_--;
						if(selected_option_ == -1) {
							selected_option_ = options_.size() - 1;
						}
						break;
					}
				}
				if(cur_row_ >= lines_.size()) {
					clear_modal();
					return;
				}

				break;
		}
	}

	if(cur_row_ < lines_.size()) {
		int num_keys;
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 3
		const Uint8* keys = SDL_GetKeyboardState(&num_keys);
#else
		const Uint8* keys = SDL_GetKeyState(&num_keys);
#endif
		const int WaitTime = std::count(keys, keys + num_keys, 0)  == num_keys ? 3 : 1;

		const int nchars = lines_[cur_row_].width()/font::char_width(FontSize);

		++cur_wait_;
		if(cur_wait_ >= WaitTime) {
			cur_wait_ = 0;
			++cur_char_;
			if(cur_char_ >= nchars) {
				cur_char_ = 0;
				++cur_row_;
			}
		}
	}
}
