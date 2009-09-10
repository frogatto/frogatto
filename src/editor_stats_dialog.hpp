#ifndef EDITOR_STATS_DIALOG_HPP_INCLUDED
#define EDITOR_STATS_DIALOG_HPP_INCLUDED

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
	void add_stats(const std::vector<stats::record_ptr>& stats);
	editor& editor_;
};

}

#endif
