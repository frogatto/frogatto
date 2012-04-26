#ifndef PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#define PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <string>

#include "dialog.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "text_editor_widget.hpp"

namespace editor_dialogs
{

class property_editor_dialog : public gui::dialog
{
public:
	explicit property_editor_dialog(editor& e);
	void init();

	entity_ptr get_entity() const { return entity_.empty() ? entity_ptr() : entity_.front(); }
	const std::vector<entity_ptr>& get_entity_list() const { return entity_; }
	void set_entity(entity_ptr e);
	void set_entity_group(const std::vector<entity_ptr>& entities);
	void set_label_dialog();
private:
	entity_ptr get_static_entity() const;
	void change_min_difficulty(int amount);
	void change_max_difficulty(int amount);
	void toggle_property(const std::string& id);
	void change_property(const std::string& id, int change);
	void change_level_property(const std::string& id);

	void change_label_property(const std::string& id);
	void change_text_property(const std::string& id, const gui::text_editor_widget* w);
	void change_enum_property(const std::string& id);
	void set_enum_property(const std::string& id, const std::vector<std::string>& options, int index);

	void change_points_property(const std::string& id);

	void mutate_value(const std::string& key, variant value);

	void deselect_object_type(std::string type);

	editor& editor_;
	std::vector<entity_ptr> entity_;
	gui::widget_ptr context_menu_;
};

}

#endif
#endif // !NO_EDITOR
