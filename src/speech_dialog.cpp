#include <iostream>

#include "frame.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "raster.hpp"
#include "speech_dialog.hpp"

speech_dialog::speech_dialog()
  : cycle_(0), left_side_speaking_(false), horizontal_position_(0), text_char_(0)
{}

bool speech_dialog::key_press()
{
	if(text_char_ < num_chars()) {
		text_char_ = num_chars();
		return false;
	}

	if(text_.size() > 2) {
		text_.erase(text_.begin());
		text_char_ = text_.front().size();
		return false;
	}

	return true;
}

void speech_dialog::process()
{
	++cycle_;

	if(text_char_ < num_chars()) {
		++text_char_;
	}

	const int ScrollSpeed = 20;
	if(left_side_speaking_) {
		if(horizontal_position_ > 0) {
			horizontal_position_ -= ScrollSpeed;
			if(horizontal_position_ < 0) {
				horizontal_position_ = 0;
			}
		}
	} else {
		const int width = gui_section::get("speech_portrait_pane")->width();
		if(horizontal_position_ < width) {
			horizontal_position_ += ScrollSpeed;
			if(horizontal_position_ > width) {
				horizontal_position_ = width;
			}
		}
	}
}

void speech_dialog::draw() const
{
	const_gui_section_ptr portrait_panel = gui_section::get("speech_portrait_pane");
	const_gui_section_ptr text_panel = gui_section::get("speech_text_pane");
	portrait_panel->blit(-horizontal_position_, graphics::screen_height() - portrait_panel->height());
	portrait_panel->blit(graphics::screen_width() - horizontal_position_, graphics::screen_height() - portrait_panel->height());
	text_panel->blit(portrait_panel->width() - horizontal_position_, graphics::screen_height() - text_panel->height());

	const_graphical_font_ptr font = graphical_font::get("default");

	const int TextBorder = 10;
	const rect text_area(portrait_panel->width() - horizontal_position_ + TextBorder, graphics::screen_height() - text_panel->height() + TextBorder, text_panel->width() - TextBorder*2, text_panel->height() - TextBorder*2);

	//if we have multiple lines, we always align on the very left, otherwise
	//we center it.
	int text_left_align = text_.size() > 1 ? text_area.x() + 16 : text_area.x2();
	int total_height = 0;
	for(int n = 0; n < text_.size(); ++n) {
		rect area = font->dimensions(text_[n]);

		if(n < 2) {
			total_height += area.h();
		}

		const int width = area.w();
		const int left = text_area.x() + text_area.w()/2 - width/2;
		if(left < text_left_align) {
			text_left_align = left;
		}
	}

	int ypos = text_area.y() + (text_area.h() - total_height)/2;
	int nchars = text_char_;
	for(int n = 0; n < 2 && n < text_.size() && nchars > 0; ++n) {
		std::string str(text_[n].begin(), text_[n].begin() +
		                  std::min<int>(nchars, text_[n].size()));
		//currently the color of speech is hard coded.
		glColor3ub(255, 187, 10);
		rect area = font->draw(text_left_align, ypos, str);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		ypos = area.y2();
		nchars -= text_[n].size();
	}

	if(text_.size() > 2 && text_char_ == num_chars() && (cycle_&16)) {
		const_gui_section_ptr down_arrow = gui_section::get("speech_text_down_arrow");
		down_arrow->blit(text_area.x2() - down_arrow->width() - 10, text_area.y2() - down_arrow->height() - 10);
		
	}

	if(left_) {
		const frame& f = left_->portrait_frame();
		const int xpos = -horizontal_position_ + portrait_panel->width()/2 - f.width()/2;
		const int ypos = graphics::screen_height() - f.height() - 18;
		f.draw(xpos, ypos);
	}

	if(right_) {
		const frame& f = right_->portrait_frame();
		const int xpos = graphics::screen_width() - horizontal_position_ + portrait_panel->width()/2 - f.width()/2;
		const int ypos = graphics::screen_height() - f.height() - 18;
		f.draw(xpos, ypos, false);
	}

}

void speech_dialog::set_speaker_and_flip_side(const_entity_ptr e)
{
	std::cerr << "set speaker\n";
	left_side_speaking_ = !left_side_speaking_;
	set_speaker(e, left_side_speaking_);
}

void speech_dialog::set_speaker(const_entity_ptr e, bool left_side)
{
	if(left_side) {
		left_ = e;
	} else {
		right_ = e;
	}
}

void speech_dialog::set_side(bool left_side)
{
	left_side_speaking_ = left_side;
}

void speech_dialog::set_text(const std::vector<std::string>& text)
{
	text_ = text;
	text_char_ = 0;
}

int speech_dialog::num_chars() const
{
	int res = 0;
	if(text_.size() >= 1) {
		res += text_[0].size();
	}

	if(text_.size() >= 2) {
		res += text_[1].size();
	}

	return res;
}
