#ifndef EDITOR_LAYERS_DIALOG_HPP_INCLUDED
#define EDITOR_LAYERS_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <vector>

#include "dialog.hpp"
#include "editor.hpp"
#include "image_widget.hpp"

namespace editor_dialogs
{

static const int LAYERS_DIALOG_WIDTH = 40;

class editor_layers_dialog : public gui::dialog
{
public:
	explicit editor_layers_dialog(editor& e);
	void init();
	void process();
private:
	void row_selected(int nrow);
	void row_mouseover(int nrow);
	editor& editor_;

	struct row_data {
		gui::gui_section_widget* checkbox;
		int layer;
		bool hidden;
	};

	std::vector<row_data> rows_;

	bool locked_;

	std::set<int> before_locked_state_;
};

}

#endif // !NO_EDITOR
#endif
