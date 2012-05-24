#pragma once
#ifndef CUSTOM_OBJECT_DIALOG_HPP_INCLUDED

#include "custom_object.hpp"
#include "custom_object_type.hpp"
#include "dialog.hpp"
#include "editor.hpp"
#include "variant.hpp"

namespace editor_dialogs {

class custom_object_dialog : public gui::dialog
{
public:
	explicit custom_object_dialog(editor& e, int x, int y, int w, int h);
	void init();
protected:
	void change_template();
	void execute_change_template(const std::vector<std::string>& choices, size_t index);
private:
	std::string template_file_;
	variant object_template_;
	custom_object_type_ptr object_;

	gui::widget_ptr context_menu_;
};

}

#endif // CUSTOM_OBJECT_DIALOG_HPP_INCLUDED

