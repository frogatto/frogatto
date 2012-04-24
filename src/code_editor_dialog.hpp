#ifndef CODE_EDITOR_DIALOG_HPP_INCLUDED
#define CODE_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/shared_ptr.hpp>

#include <string>

#include "animation_preview_widget.hpp"
#include "asserts.hpp"
#include "dialog.hpp"
#include "geometry.hpp"
#include "grid_widget.hpp"
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

	void load_file(std::string fname);

	bool has_keyboard_focus() const;

	void process();

private:
	void init_files_grid();

	bool handle_event(const SDL_Event& event, bool claimed);
	void handle_draw_children() const;

	void set_animation_rect(rect r);
	void set_integer_attr(const char* attr, int value);

	void save();

	std::string fname_;

	int invalidated_;

	bool modified_;

	boost::shared_ptr<gui::code_editor_widget> editor_;
	gui::text_editor_widget* search_;
	gui::text_editor_widget* replace_;

	gui::label_ptr status_label_;

	gui::label_ptr error_label_;

	gui::grid_ptr files_grid_;

	void on_search_changed();
	void on_search_enter();

	void on_code_changed();
	void on_move_cursor();

	//As long as there is a code editor active, we are going to want to
	//recover from errors.
	assert_recover_scope assert_recovery_;

	boost::shared_ptr<gui::animation_preview_widget> animation_preview_;

	struct KnownFile {
		std::string fname;
		boost::shared_ptr<frame> anim;
		boost::shared_ptr<gui::code_editor_widget> editor;
	};

	std::vector<KnownFile> files_;
	void select_file(int index);
};

#endif // !NO_EDITOR
#endif
