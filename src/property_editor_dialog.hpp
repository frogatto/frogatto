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

	entity_ptr get_entity() const { return entity_; }
	void set_entity(entity_ptr e);
	void set_label_dialog();
private:
	void change_min_difficulty(int amount);
	void change_max_difficulty(int amount);
	void toggle_property(const std::string& id);
	void change_property(const std::string& id, int change);
	void change_level_property(const std::string& id);

	void change_label_property(const std::string& id);
	void change_text_property(const std::string& id);
	void change_enum_property(const std::string& id);
	void set_enum_property(const std::string& id, const std::vector<std::string>& options, int index);

	void change_points_property(const std::string& id);

	void mutate_value(const std::string& key, variant value);

	editor& editor_;
	entity_ptr entity_;
	gui::widget_ptr context_menu_;
};

}

#endif
