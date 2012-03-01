#include <iostream>
#include <stack>
#include <limits.h>

#include <boost/tokenizer.hpp>

#include "controls.hpp"
#include "color_utils.hpp"
#include "draw_scene.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "framed_gui_element.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "joystick.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "speech_dialog.hpp"
#include "iphone_controls.hpp"

namespace {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	const int OptionHeight = 70;
	const int OptionMinWidth = 200;
	const int OptionXPad = 20;
#else
	const int OptionHeight = 50;
	const int OptionMinWidth = 150;
	const int OptionXPad = 10;
#endif
	const int OptionsBorder = 20; // size of the border around the options window
	const int OptionsX = 135; // these denote the bottom right corner
	const int OptionsY = 115;
}

speech_dialog::speech_dialog()
  : cycle_(0), left_side_speaking_(false), horizontal_position_(0), text_char_(0), option_selected_(0),
    joystick_button_pressed_(true),
    joystick_up_pressed_(true),
    joystick_down_pressed_(true),
	expiration_(-1)
{
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	option_selected_ = -1;
#endif
}

speech_dialog::~speech_dialog()
{
}

bool speech_dialog::handle_mouse_move(int x, int y)
{
	translate_mouse_coords(&x, &y);
	rect box(
		preferences::virtual_screen_width() - OptionsX - option_width_ - OptionsBorder,
		preferences::virtual_screen_height() - OptionsY - OptionHeight*options_.size() - OptionsBorder,
		option_width_ + OptionsBorder*2, OptionHeight*options_.size() + OptionsBorder*2
	);
	//std::cerr << "Options box: " << box << " : " << x << " : " << y << "\n";
	if (point_in_rect(point(x, y), box))
	{
		option_selected_ = (y-box.y())/OptionHeight;
		return true;
	} else {
		option_selected_ = -1;
		return false;
	}
}

void speech_dialog::move_up()
{
	--option_selected_;
	if(option_selected_ < 0) {
		option_selected_ = options_.size() - 1;
	}
}

void speech_dialog::move_down()
{
	++option_selected_;
	if(option_selected_ == options_.size()) {
		option_selected_ = 0;
	}
}

bool speech_dialog::key_press(const SDL_Event& event)
{
	static int last_mouse = 0;
	if(text_char_ == num_chars() && options_.empty() == false) {
		if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_UP)) {
				move_up();
			} else if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_DOWN)) {
				move_down();
			} else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE ||
			   event.key.keysym.sym == get_sdlkey(controls::CONTROL_JUMP) ||
			   event.key.keysym.sym == get_sdlkey(controls::CONTROL_TONGUE)) {
				return true;
			}
		}
	
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE || TARGET_BLACKBERRY
		if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			last_mouse = event.button.which;
			handle_mouse_move(event.button.x, event.button.y);
		}
		if (event.type == SDL_MOUSEMOTION)
		{
			if (event.motion.which == last_mouse)
				handle_mouse_move(event.motion.x, event.motion.y);
		}
		if (event.type == SDL_MOUSEBUTTONUP)
		{
			if (event.motion.which == last_mouse)
			{
				last_mouse = -1;
				return handle_mouse_move(event.motion.x, event.motion.y);
			}
		}
#endif

		return false;
	} else if (event.type != SDL_KEYDOWN && event.type != SDL_MOUSEBUTTONDOWN) {
		return false; // only keydown and mousebuttondown should be handled by the rest of the function
	}

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

bool is_skipping_game();

bool speech_dialog::process()
{
	if(is_skipping_game()) {
		return true;
	}

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

	if(expiration_ <= 0) {
		joystick::update();

		if(!joystick_up_pressed_ && joystick::up()) {
			move_up();
		}

		if(!joystick_down_pressed_ && joystick::down()) {
			move_down();
		}

		if(!joystick_button_pressed_ && (joystick::button(0) || joystick::button(10))) {
			return true;
		}
	}

	joystick_up_pressed_ = joystick::up();
	joystick_down_pressed_ = joystick::down();
	joystick_button_pressed_ = joystick::button(0) || joystick::button(1);

	return cycle_ == expiration_;
}

void speech_dialog::draw() const
{
	static const const_gui_section_ptr top_corner = gui_section::get("speech_dialog_top_corner");
	static const const_gui_section_ptr bottom_corner = gui_section::get("speech_dialog_bottom_corner");
	static const const_gui_section_ptr top_edge = gui_section::get("speech_dialog_top_edge");
	static const const_gui_section_ptr bottom_edge = gui_section::get("speech_dialog_bottom_edge");
	static const const_gui_section_ptr side_edge = gui_section::get("speech_dialog_side_edge");
	static const const_gui_section_ptr arrow = gui_section::get("speech_dialog_arrow");

	const_graphical_font_ptr font = graphical_font::get("default");

	const int TextAreaHeight = 80;

	const int TextBorder = 10;

	int speaker_xpos = INT_MAX;
	int speaker_ypos = INT_MAX;

	const_entity_ptr speaker = left_side_speaking_ ? left_ : right_;
	if(speaker) {
		const screen_position& pos = last_draw_position();
		const int screen_x = pos.x/100 + (graphics::screen_width()/2)*(-1.0/pos.zoom + 1.0);
		const int screen_y = pos.y/100 + (graphics::screen_height()/2)*(-1.0/pos.zoom + 1.0);

		speaker_xpos = (speaker->feet_x() - screen_x)*pos.zoom - 36;
		speaker_ypos = (speaker->feet_y() - screen_y)*pos.zoom - 10;
	}

	if(pane_area_.w() == 0) {
		pane_area_ = rect(
		  top_corner->width(),
		  preferences::virtual_screen_height() - TextAreaHeight + TextBorder,
		  preferences::virtual_screen_width() - top_corner->width()*2,
		  TextAreaHeight - bottom_corner->height());
		if(speaker_ypos < 100) {
			pane_area_ = rect(pane_area_.x(), top_corner->height() + 50, pane_area_.w(), pane_area_.h());
		}
	}

	const rect text_area(pane_area_.x()-30, pane_area_.y()-30, pane_area_.w()+60, pane_area_.h()+60);

	graphics::draw_rect(pane_area_, graphics::color(85, 53, 53, 255));
	top_corner->blit(pane_area_.x() - top_corner->width(), pane_area_.y() - top_corner->height());
	top_corner->blit(pane_area_.x2()-1, pane_area_.y() - top_corner->height(), -top_corner->width(), top_corner->height());

	top_edge->blit(pane_area_.x(), pane_area_.y() - top_edge->height(), pane_area_.w(), top_edge->height());

	bottom_corner->blit(pane_area_.x() - bottom_corner->width(), pane_area_.y2());
	bottom_corner->blit(pane_area_.x2()-1, pane_area_.y2(), -bottom_corner->width(), bottom_corner->height());

	bottom_edge->blit(pane_area_.x(), pane_area_.y2(), pane_area_.w(), bottom_edge->height());

	side_edge->blit(pane_area_.x() - side_edge->width(), pane_area_.y(), side_edge->width(), pane_area_.h());
	side_edge->blit(pane_area_.x2()-1, pane_area_.y(), -side_edge->width(), pane_area_.h());

	if(speaker) {

		//if the arrow to the speaker is within reasonable limits, then
		//blit it.
		if(speaker_xpos > top_corner->width() && speaker_xpos < graphics::screen_width() - top_corner->width() - arrow->width()) {
			arrow->blit(speaker_xpos, pane_area_.y() - arrow->height() - 32);
		}
	}


	//we center our text. Create a vector of the left edge of the text.
	std::vector<int> text_left_align;

	int total_height = 0;
	for(int n = 0; n < text_.size(); ++n) {
		rect area = font->dimensions(text_[n]);

		if(n < 2) {
			total_height += area.h();
		}

		const int width = area.w();
		const int left = text_area.x() + text_area.w()/2 - width/2;
		text_left_align.push_back(left);
	}

	int ypos = text_area.y() + (text_area.h() - total_height)/2;
	int nchars = text_char_;
	for(int n = 0; n < 2 && n < text_.size() && nchars > 0; ++n) {
		std::string str(text_[n].begin(), text_[n].begin() +
		                  std::min<int>(nchars, text_[n].size()));
		//currently the color of speech is hard coded.
		glColor4ub(255, 187, 10, 255);
		//move the first line slightly up so that accents don't mess up centering
		rect area = font->draw(text_left_align[n], ypos - 2, str);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		//add some space between the lines
		ypos = area.y2() + 4;
		nchars -= text_[n].size();
	}

	if(text_.size() > 2 && text_char_ == num_chars() && (cycle_&16)) {
		const_gui_section_ptr down_arrow = gui_section::get("speech_text_down_arrow");
		down_arrow->blit(text_area.x2() - down_arrow->width() - 10, text_area.y2() - down_arrow->height() - 10);
		
	}

	if(text_char_ == num_chars() && options_.empty() == false) {
		//const_gui_section_ptr options_panel = gui_section::get("speech_portrait_pane");
		const_framed_gui_element_ptr options_panel = framed_gui_element::get("regular_window");
		int xpos = graphics::screen_width() - OptionsX - option_width_ - OptionsBorder*2;
		int ypos = graphics::screen_height() - OptionsY - OptionHeight*options_.size() - OptionsBorder*2;
		options_panel->blit(xpos, ypos, OptionsBorder*2 + option_width_, OptionsBorder*2 + OptionHeight*options_.size(), true);

		xpos += OptionsBorder + OptionXPad;
		ypos += OptionsBorder;

		glColor4ub(255, 187, 10, 255);
		int index = 0;
		foreach(const std::string& option, options_) {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
			if(index == option_selected_) {
				graphics::draw_rect(rect(xpos-OptionXPad, ypos, option_width_, OptionHeight), graphics::color(0xC74545FF));
				glColor4ub(255, 187, 10, 255); //reset color to what it was, since draw_rect changes it
			}
#endif
			rect area = font->dimensions(option);
			area = font->draw(xpos, ypos+(OptionHeight/3-area.h()/4), option);

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
			if(index == option_selected_) {
				const_gui_section_ptr cursor = gui_section::get("cursor");
				cursor->blit(area.x2(), area.y());
			}
#endif

			ypos += OptionHeight;
			++index;
		}
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}
}

void speech_dialog::set_speaker_and_flip_side(const_entity_ptr e)
{
	//std::cerr << "set speaker\n";
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

	pane_area_ = rect();
}

void speech_dialog::set_side(bool left_side)
{
	left_side_speaking_ = left_side;
}

void speech_dialog::set_text(const std::vector<std::string>& text)
{
	text_.clear();
	for (int i = 0; i < text.size(); i++) {
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		//split text[i] at newline characters, add each line separately
		tokenizer lines(text[i], boost::char_separator<char> ("\n"));
		for (tokenizer::iterator iter = lines.begin(); iter != lines.end(); ++iter) {
			text_.push_back(*iter);
		}
	}
	text_char_ = 0;
}

void speech_dialog::set_options(const std::vector<std::string>& options)
{
	options_ = options;
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	option_selected_ = -1;
#else
	option_selected_ = 0;
#endif
	option_width_ = OptionMinWidth;
	const_graphical_font_ptr font = graphical_font::get("default");
	foreach(const std::string& option, options_) {
		rect area = font->dimensions(option);
		if (area.w()+OptionXPad*2 > option_width_)
			option_width_ = area.w()+OptionXPad*2;
	}
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
