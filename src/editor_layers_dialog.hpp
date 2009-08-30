#ifndef EDITOR_LAYERS_DIALOG_HPP_INCLUDED
#define EDITOR_LAYERS_DIALOG_HPP_INCLUDED

#include <vector>

#include "dialog.hpp"
#include "editor.hpp"
#include "image_widget.hpp"

namespace editor_dialogs
{

class editor_layers_dialog : public gui::dialog
{
public:
	explicit editor_layers_dialog(editor& e);
	void init();
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
};

}

#endif
