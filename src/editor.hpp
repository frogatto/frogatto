#ifndef EDITOR_HPP_INCLUDED
#define EDITOR_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"

namespace gui {
class dialog;
}

namespace editor_dialogs {
class tileset_editor_dialog;
}

class editor_mode_dialog;

class editor
{
public:
	editor(const char* level_cfg);
	~editor();
	void edit_level();

	struct tileset {
		static void init(wml::const_node_ptr node);
		explicit tileset(wml::const_node_ptr node);
		std::string category;
		std::string type;
		int zorder;
		boost::shared_ptr<tile_map> preview;
		bool sloped;
	};

	const std::vector<tileset>& all_tilesets() const;
	int get_tileset() const { return cur_tileset_; }
	void set_tileset(int index);

	enum EDIT_MODE { EDIT_TILES, EDIT_CHARS, EDIT_ITEMS, EDIT_GROUPS, EDIT_PROPERTIES, EDIT_VARIATIONS, EDIT_PROPS, NUM_MODES };
	EDIT_MODE mode() const { return mode_; }
	void change_mode(int nmode);
private:
	void draw() const;

	CKey key_;

	boost::scoped_ptr<level> lvl_;
	int xpos_, ypos_;
	int anchorx_, anchory_;
	std::string filename_;

	EDIT_MODE mode_;
	mutable bool select_previous_level_, select_next_level_;
	bool done_;
	bool face_right_;
	int cur_tileset_;

	boost::scoped_ptr<editor_mode_dialog> editor_mode_dialog_;
	boost::scoped_ptr<editor_dialogs::tileset_editor_dialog> tileset_dialog_;

	gui::dialog* current_dialog_;
};

#endif
