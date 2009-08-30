#ifndef EDITOR_HPP_INCLUDED
#define EDITOR_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

#include "geometry.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "prop.hpp"
#include "stats.hpp"

namespace gui {
class dialog;
}

namespace editor_dialogs {
class character_editor_dialog;
class editor_layers_dialog;
class group_property_editor_dialog;
class property_editor_dialog;
class tileset_editor_dialog;
}

class editor_menu_dialog;
class editor_mode_dialog;

class editor
{
public:
	static void edit(const char* level_cfg, int xpos=-1, int ypos=-1);
	static std::string last_edited_level();

	editor(const char* level_cfg);
	~editor();
	void edit_level();

	void load_stats();
	void download_stats();

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

	struct tile_selection {
		bool empty() const { return tiles.empty(); }
		std::vector<point> tiles;
	};

	const std::vector<tileset>& all_tilesets() const;
	int get_tileset() const { return cur_tileset_; }
	void set_tileset(int index);

	const std::vector<enemy_type>& all_characters() const;

	int get_object() const { return cur_object_; }
	void set_object(int index);

	const std::vector<const_prop_ptr>& get_props() const;

	enum EDIT_TOOL { TOOL_ADD_RECT, TOOL_SELECT_RECT, TOOL_MAGIC_WAND, TOOL_PENCIL, TOOL_PICKER, TOOL_ADD_OBJECT, TOOL_SELECT_OBJECT, NUM_TOOLS };
	EDIT_TOOL tool() const { return tool_; }
	void change_tool(EDIT_TOOL tool);

	level& get_level() { return *lvl_; }

	void save_level();
	void save_level_as(const std::string& filename);
	void quit() { done_ = true; }
	void zoom_in();
	void zoom_out();

	void undo_command();
	void redo_command();

	void close() { done_ = true; }

	//make the selected objects part of a group
	void group_selection();

	//function to execute a command which will go into the undo/redo list.
	void execute_command(boost::function<void()> command, boost::function<void()> undo);

private:
	void handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	void handle_mouse_button_up(const SDL_MouseButtonEvent& event);
	void handle_key_press(const SDL_KeyboardEvent& key);
	void handle_scrolling();

	void handle_object_dragging(int mousex, int mousey);

	void process_ghost_objects();
	void remove_ghost_objects();
	void draw() const;
	void draw_selection(int xoffset, int yoffset) const;

	void add_tile_rect(int x1, int y1, int x2, int y2);
	void remove_tile_rect(int x1, int y1, int x2, int y2);
	void select_tile_rect(int x1, int y1, int x2, int y2);
	void select_magic_wand(int xpos, int ypos);

	void set_selection(const tile_selection& s);

	bool editing_objects() const { return tool_ == TOOL_ADD_OBJECT || tool_ == TOOL_SELECT_OBJECT; }
	bool editing_tiles() const { return !editing_objects(); }

	CKey key_;

	boost::scoped_ptr<level> lvl_;
	int zoom_;
	int xpos_, ypos_;
	int anchorx_, anchory_;

	//if we are dragging an entity around, this marks the position from
	//which the entity started the drag.
	int selected_entity_startx_, selected_entity_starty_;
	std::string filename_;

	EDIT_TOOL tool_;
	mutable bool select_previous_level_, select_next_level_;
	bool done_;
	bool face_right_;
	int cur_tileset_;

	int cur_object_;

	tile_selection tile_selection_;

	boost::scoped_ptr<editor_menu_dialog> editor_menu_dialog_;
	boost::scoped_ptr<editor_mode_dialog> editor_mode_dialog_;
	boost::scoped_ptr<editor_dialogs::character_editor_dialog> character_dialog_;
	boost::scoped_ptr<editor_dialogs::editor_layers_dialog> layers_dialog_;
	boost::scoped_ptr<editor_dialogs::group_property_editor_dialog> group_property_dialog_;
	boost::scoped_ptr<editor_dialogs::property_editor_dialog> property_dialog_;
	boost::scoped_ptr<editor_dialogs::tileset_editor_dialog> tileset_dialog_;

	gui::dialog* current_dialog_;

	//if the mouse is currently down, drawing a rect.
	bool drawing_rect_, dragging_;

	struct executable_command {
		boost::function<void()> redo_command;
		boost::function<void()> undo_command;
	};

	std::vector<executable_command> undo_, redo_;

	std::vector<entity_ptr> ghost_objects_;

	std::vector<stats::record_ptr> stats_;
};

#endif
