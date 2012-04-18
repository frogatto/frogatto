#ifndef CODE_EDITOR_DIALOG_HPP_INCLUDED
#define CODE_EDITOR_DIALOG_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <string>

#include "animation_preview_widget.hpp"
#include "asserts.hpp"
#include "dialog.hpp"
#include "geometry.hpp"
#include "label.hpp"

namespace gui {
class code_editor_widget;
class text_editor_widget;
}

class code_editor_dialog : public gui::dialog
{
public:
	code_editor_dialog(const rect& r);
	void init();

	void load_file(const std::string& fname);

	bool has_keyboard_focus() const;

	void process();

private:
	bool handle_event(const SDL_Event& event, bool claimed);
	void handle_draw_children() const;

	void set_animation_rect(rect r);

	std::string fname_;

	int invalidated_;

	bool modified_;

	gui::code_editor_widget* editor_;
	gui::text_editor_widget* search_;
	gui::text_editor_widget* replace_;

	gui::label_ptr status_label_;

	gui::label_ptr error_label_;

	void on_search_changed();
	void on_search_enter();

	void on_code_changed();
	void on_move_cursor();

	//As long as there is a code editor active, we are going to want to
	//recover from errors.
	assert_recover_scope assert_recovery_;

	boost::shared_ptr<gui::animation_preview_widget> animation_preview_;
};

#endif
