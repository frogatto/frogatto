#ifndef MESSAGE_DIALOG_HPP_INCLUDED
#define MESSAGE_DIALOG_HPP_INCLUDED

#include <string>
#include <vector>

#include "geometry.hpp"
#include "texture.hpp"

class message_dialog
{
public:
	static void show_modal(const std::string& text, const std::vector<std::string>* options=NULL);
	static void clear_modal();
	static message_dialog* get();
	void draw() const;
	void process();

	int selected_option() const { return selected_option_; }
private:
	message_dialog(const std::string& text, const rect& pos,
	               const std::vector<std::string>* options=NULL);
	std::string text_;
	rect pos_;
	int viewable_lines_;
	int line_height_;

	int cur_row_, cur_char_, cur_wait_;

	std::vector<graphics::texture> lines_;
	std::vector<graphics::texture> options_;
	int selected_option_;
};

typedef boost::intrusive_ptr<message_dialog> message_dialog_ptr;

#endif
