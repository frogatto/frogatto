#ifndef SPEECH_DIALOG_HPP_INCLUDED
#define SPEECH_DIALOG_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "graphics.hpp"

#include <string>
#include <vector>

#include "color_utils.hpp"
#include "entity.hpp"
#include "gui_section.hpp"

class speech_dialog
{
public:

	speech_dialog();
	~speech_dialog();

	bool detect_joystick_press();
	bool key_press(const SDL_Event& e);
	bool process();
	void draw() const;
	void set_speaker_and_flip_side(const_entity_ptr e);
	void set_speaker(const_entity_ptr e, bool left_side=false);
	void set_side(bool left_side);
	void set_text(const std::vector<std::string>& text);
	void set_options(const std::vector<std::string>& options);
	void set_expiration(int time) { expiration_ = time; }

	int option_selected() const { return option_selected_; }
	void set_option_selected(int n) { option_selected_ = n; }
private:
	bool handle_mouse_move(int x, int y);
	void move_up();
	void move_down();

	bool scroll_text();

	int cycle_;
	const_entity_ptr left_, right_;
	bool left_side_speaking_;
	int horizontal_position_;

	struct TextMarkup {
		int begin;
		boost::shared_ptr<graphics::color> color;
	};

	std::vector<std::vector<TextMarkup> > markup_;

	std::vector<std::string> text_;
	int text_char_;

	std::vector<std::string> options_;
	int option_selected_;
	int option_width_;

	bool joystick_button_pressed_, joystick_up_pressed_, joystick_down_pressed_;

	int expiration_;

	mutable rect pane_area_;

	int num_chars() const;

	speech_dialog(const speech_dialog&);
	void operator=(const speech_dialog&);
};

#endif
