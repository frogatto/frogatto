#ifndef EDITOR_STATS_DIALOG_HPP_INCLUDED
#define EDITOR_STATS_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR


#include <string>
#include <vector>

#include "dialog.hpp"

class editor;

namespace editor_dialogs
{

class editor_stats_dialog : public gui::dialog
{
public:
	explicit editor_stats_dialog(editor& e);
	void init();
private:
	editor& editor_;
};

typedef boost::intrusive_ptr<editor_stats_dialog> editor_stats_dialog_ptr;

}

#endif // !NO_EDITOR
#endif
