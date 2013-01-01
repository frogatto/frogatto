#ifndef GROUP_PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#define GROUP_PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <string>
#include <vector>

#include "dialog.hpp"
#include "editor.hpp"
#include "entity.hpp"

namespace editor_dialogs
{

class group_property_editor_dialog : public gui::dialog
{
public:
	explicit group_property_editor_dialog(editor& e);
	void init();

	void set_group(const std::vector<entity_ptr>& group);
private:
	void group_objects();

	editor& editor_;
	std::vector<entity_ptr> group_;
};

typedef boost::intrusive_ptr<group_property_editor_dialog> group_property_editor_dialog_ptr;

}

#endif
#endif // !NO_EDITOR

