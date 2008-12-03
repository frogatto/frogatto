#ifndef TILESET_EDITOR_DIALOG_HPP_INCLUDED
#define TILESET_EDITOR_DIALOG_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "dialog.hpp"
#include "tile_map.hpp"

class editor;

namespace editor_dialogs
{

class tileset_editor_dialog : public gui::dialog
{
public:
	explicit tileset_editor_dialog(editor& e);
	void init();
private:
	void set_tileset(int index);

	bool handle_event(const SDL_Event& event, bool claimed);
	editor& editor_;
};

}

#endif
