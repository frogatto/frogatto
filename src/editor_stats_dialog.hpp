/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
