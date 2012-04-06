#include <boost/bind.hpp>

#include "code_editor_dialog.hpp"
#include "code_editor_widget.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "label.hpp"
#include "module.hpp"
#include "text_editor_widget.hpp"

code_editor_dialog::code_editor_dialog(const rect& r)
  : dialog(r.x(), r.y(), r.w(), r.h()), invalidated_(0), modified_(false)
{
	init();
}

void code_editor_dialog::init()
{
	using namespace gui;

	editor_ = new code_editor_widget(width() - 10, height() - 60);
	search_ = new text_editor_widget(120);
	replace_ = new text_editor_widget(120);
	const SDL_Color col = {255,255,255,255};
	widget_ptr find_label(label::create("Find: ", col));
	status_label_ = label::create("Ok", col);
	error_label_ = label::create("", col);
	add_widget(find_label, 12, 12, MOVE_RIGHT);
	add_widget(widget_ptr(search_), MOVE_RIGHT);
	add_widget(widget_ptr(replace_), MOVE_DOWN);
	add_widget(widget_ptr(editor_), find_label->x(), find_label->y() + find_label->height() + 2);
	add_widget(status_label_);
	add_widget(error_label_, status_label_->x() + 480, status_label_->y());

	replace_->set_visible(false);

	search_->set_on_change_handler(boost::bind(&code_editor_dialog::on_search_changed, this));
	search_->set_on_enter_handler(boost::bind(&code_editor_dialog::on_search_enter, this));

	editor_->set_on_change_handler(boost::bind(&code_editor_dialog::on_code_changed, this));
	editor_->set_on_move_cursor_handler(boost::bind(&code_editor_dialog::on_move_cursor, this));
}

void code_editor_dialog::load_file(const std::string& fname)
{
	if(fname_ == fname) {
		return;
	}

	fname_ = fname;
	std::string text = json::get_file_contents(fname);
	editor_->set_text(json::get_file_contents(fname));
	modified_ = text != sys::read_file(module::map_file(fname));
	on_move_cursor();
}

bool code_editor_dialog::has_keyboard_focus() const
{
	return editor_->has_focus() || search_->has_focus() || replace_->has_focus();
}

bool code_editor_dialog::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = claimed || dialog::handle_event(event, claimed);
	if(claimed) {
		return claimed;
	}

	if(has_keyboard_focus()) {
		switch(event.type) {
		case SDL_KEYDOWN: {
			if(event.key.keysym.sym == SDLK_f && (event.key.keysym.mod&KMOD_CTRL)) {
				search_->set_focus(true);
				replace_->set_focus(false);
				editor_->set_focus(false);
				return true;
			} else if(event.key.keysym.sym == SDLK_s && (event.key.keysym.mod&KMOD_CTRL)) {
				sys::write_file(module::map_file(fname_), editor_->text());
				status_label_->set_text(formatter() << "Saved " << fname_);
				modified_ = false;
				return true;
			}
			break;
		}
		}
	}

	return claimed;
}

void code_editor_dialog::process()
{
	if(invalidated_ && SDL_GetTicks() > invalidated_ + 200) {
		const bool result = custom_object_type::set_file_contents(fname_, editor_->text());
		error_label_->set_text(result ? "Ok" : "Error");
		invalidated_ = 0;
	}
}

void code_editor_dialog::on_search_changed()
{
	editor_->set_search(search_->text());
}

void code_editor_dialog::on_search_enter()
{
	editor_->next_search_match();
}

void code_editor_dialog::on_code_changed()
{
	if(!modified_) {
		modified_ = true;
		on_move_cursor();
	}

	if(!invalidated_) {
		invalidated_ = SDL_GetTicks();
		error_label_->set_text("Processing...");
	}
}

void code_editor_dialog::on_move_cursor()
{
	status_label_->set_text(formatter() << "Row " << (editor_->cursor_row()+1) << " Col " << (editor_->cursor_col()+1) << (modified_ ? " (Modified)" : ""));
}
