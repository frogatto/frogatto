#ifndef EDITOR_HPP_INCLUDED
#define EDITOR_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "prop.hpp"

namespace gui {
class dialog;
}

namespace editor_dialogs {
class character_editor_dialog;
class prop_editor_dialog;
class property_editor_dialog;
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

	struct enemy_type {
		static void init(wml::const_node_ptr node);
		explicit enemy_type(wml::const_node_ptr node);
		wml::const_node_ptr node;
		std::string category;
		const frame* preview_frame;
	};

	const std::vector<tileset>& all_tilesets() const;
	int get_tileset() const { return cur_tileset_; }
	void set_tileset(int index);

	const std::vector<enemy_type>& all_characters() const;

	int get_item() const { return cur_item_; }
	void set_item(int index);

	const std::vector<const_prop_ptr>& get_props() const;

	enum EDIT_MODE { EDIT_TILES, EDIT_CHARS, EDIT_ITEMS, EDIT_GROUPS, EDIT_PROPERTIES, EDIT_VARIATIONS, EDIT_PROPS, EDIT_PORTALS, NUM_MODES };
	EDIT_MODE mode() const { return mode_; }
	void change_mode(int nmode);
private:
	void draw() const;

	CKey key_;

	boost::scoped_ptr<level> lvl_;
	int xpos_, ypos_;
	int anchorx_, anchory_;

	//if we are dragging an entity around, this marks the position from
	//which the entity started the drag.
	int selected_entity_startx_, selected_entity_starty_;
	std::string filename_;

	EDIT_MODE mode_;
	mutable bool select_previous_level_, select_next_level_;
	bool done_;
	bool face_right_;
	int cur_tileset_;

	int cur_item_;

	boost::scoped_ptr<editor_mode_dialog> editor_mode_dialog_;
	boost::scoped_ptr<editor_dialogs::character_editor_dialog> character_dialog_;
	boost::scoped_ptr<editor_dialogs::prop_editor_dialog> prop_dialog_;
	boost::scoped_ptr<editor_dialogs::property_editor_dialog> property_dialog_;
	boost::scoped_ptr<editor_dialogs::tileset_editor_dialog> tileset_dialog_;

	gui::dialog* current_dialog_;

	//if the mouse is currently down, drawing a rect.
	bool drawing_rect_;
};

#endif
