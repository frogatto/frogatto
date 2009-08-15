#ifndef SPEECH_DIALOG_HPP_INCLUDED
#define SPEECH_DIALOG_HPP_INCLUDED

#include "SDL.h"

#include <string>
#include <vector>

#include "entity.hpp"
#include "gui_section.hpp"

class speech_dialog
{
public:
	static speech_dialog* get();

	speech_dialog();
	~speech_dialog();

	bool key_press(const SDL_Event& e);
	bool process();
	void draw() const;
	void set_speaker_and_flip_side(const_entity_ptr e);
	void set_speaker(const_entity_ptr e, bool left_side);
	void set_side(bool left_side);
	void set_text(const std::vector<std::string>& text);
	void set_options(const std::vector<std::string>& options);

	int option_selected() const { return option_selected_; }
private:
	void move_up();
	void move_down();

	int cycle_;
	const_entity_ptr left_, right_;
	bool left_side_speaking_;
	int horizontal_position_;

	std::vector<std::string> text_;
	int text_char_;

	std::vector<std::string> options_;
	int option_selected_;

	bool joystick_button_pressed_, joystick_up_pressed_, joystick_down_pressed_;

	int num_chars() const;

	speech_dialog(const speech_dialog&);
	void operator=(const speech_dialog&);
};

#endif
