#ifndef PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#define PROPERTY_EDITOR_DIALOG_HPP_INCLUDED

#include <string>

#include "dialog.hpp"
#include "editor.hpp"
#include "entity.hpp"

namespace editor_dialogs
{

class property_editor_dialog : public gui::dialog
{
public:
	explicit property_editor_dialog(editor& e);
	void init();

	void set_entity(entity_ptr e);
private:
	void change_property(const std::string& id, int change);

	editor& editor_;
	entity_ptr entity_;
};

}

#endif
