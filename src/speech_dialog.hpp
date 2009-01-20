#ifndef SPEECH_DIALOG_HPP_INCLUDED
#define SPEECH_DIALOG_HPP_INCLUDED

#include <string>
#include <vector>

#include "entity.hpp"
#include "gui_section.hpp"

class speech_dialog
{
public:
	speech_dialog();

	bool key_press();
	void process();
	void draw() const;
	void set_speaker_and_flip_side(const_entity_ptr e);
	void set_speaker(const_entity_ptr e, bool left_side);
	void set_side(bool left_side);
	void set_text(const std::vector<std::string>& text);
private:
	int cycle_;
	const_entity_ptr left_, right_;
	bool left_side_speaking_;
	int horizontal_position_;

	std::vector<std::string> text_;
	int text_char_;

	int num_chars() const;
};

#endif
