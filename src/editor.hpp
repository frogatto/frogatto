#ifndef EDITOR_HPP_INCLUDED
#define EDITOR_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <stack>
#include <vector>

#include "external_text_editor.hpp"
#include "geometry.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "preferences.hpp"
#include "stats.hpp"

static const int EDITOR_MENUBAR_HEIGHT = 40;
static const int EDITOR_SIDEBAR_WIDTH = 220;

namespace gui {
class dialog;
}

namespace editor_dialogs {
class character_editor_dialog;
class editor_layers_dialog;
class property_editor_dialog;
class segment_editor_dialog;
class tileset_editor_dialog;
class custom_object_dialog;
}

class code_editor_dialog;

class editor_menu_dialog;
class editor_mode_dialog;

class editor
{
public:
	//A manager which should be scoped around creation of editors.
	struct manager {
		~manager();
	};

	static editor* get_editor(const char* level_cfg);
	static void edit(const char* level_cfg, int xpos=-1, int ypos=-1);
	static std::string last_edited_level();

	static int sidebar_width();
	static int codebar_height();

	editor(const char* level_cfg);
	~editor();

	void setup_for_editing();
	void edit_level();

	void process();
	bool handle_event(const SDL_Event& event, bool swallowed);
	void handle_scrolling();

	int xpos() const { return xpos_; }
	int ypos() const { return ypos_; }

	int xres() const { return xres_; }
	int yres() const { return yres_; }

	void set_pos(int x, int y);

	typedef boost::intrusive_ptr<level> level_ptr;
	void set_playing_level(level_ptr lvl);
	void toggle_active_level();

	void load_stats();
	void show_stats();
	void download_stats();

	struct tileset {
		static void init(variant node);
		explicit tileset(variant node);
		std::string category;
		std::string type;
		int zorder;
		int x_speed;
		int y_speed;
		boost::shared_ptr<tile_map> preview;
		bool sloped;
	};

	struct enemy_type {
		static void init(variant node);
		explicit enemy_type(const custom_object_type& type);
		variant node;
		std::string category;
		entity_ptr preview_object;
		boost::shared_ptr<const frame> preview_frame;
	};

	struct tile_selection {
		bool empty() const { return tiles.empty(); }
		std::vector<point> tiles;
	};

	const tile_selection& selection() const { return tile_selection_; }

	const std::vector<tileset>& all_tilesets() const;
	int get_tileset() const { return cur_tileset_; }
	void set_tileset(int index);

	const std::vector<enemy_type>& all_characters() const;

	int get_object() const { return cur_object_; }
	void set_object(int index);

	enum EDIT_TOOL { TOOL_ADD_RECT, TOOL_SELECT_RECT, TOOL_MAGIC_WAND, TOOL_PENCIL, TOOL_PICKER, TOOL_ADD_OBJECT, TOOL_SELECT_OBJECT, TOOL_EDIT_SEGMENTS, NUM_TOOLS };
	EDIT_TOOL tool() const;
	void change_tool(EDIT_TOOL tool);

	level& get_level() { return *lvl_; }
	const level& get_level() const { return *lvl_; }

	std::vector<level_ptr> get_level_list() const { return levels_; }

	void save_level();
	void save_level_as(const std::string& filename);
	void quit();
	bool confirm_quit(bool allow_cancel=true);
	void zoom_in();
	void zoom_out();
	int zoom() const { return zoom_; }

	void undo_command();
	void redo_command();

	void close() { done_ = true; }

	void edit_level_properties();
	void create_new_module();
	void edit_module_properties();
	void create_new_object();

	//make the selected objects part of a group
	void group_selection();

	bool face_right() const { return face_right_; }

	//switch the current facing.
	void toggle_facing();

	void toggle_upside_down();

	void duplicate_selected_objects();

	void run_script(const std::string& id);

	//function which gets the expected layer at which a certain tile id appears.
	int get_tile_zorder(const std::string& tile_id) const;
	void add_tile_rect(int zorder, const std::string& tile_id, int x1, int y1, int x2, int y2);

	enum EXECUTABLE_COMMAND_TYPE { COMMAND_TYPE_DEFAULT, COMMAND_TYPE_DRAG_OBJECT };

	//function to execute a command which will go into the undo/redo list.
	//normally any time the editor mutates the level, it should be done
	//through this function
	void execute_command(boost::function<void()> command, boost::function<void()> undo, EXECUTABLE_COMMAND_TYPE type=COMMAND_TYPE_DEFAULT);

	//functions to begin and end a group of commands. This is used when we
	//are going to execute a bunch of commands, and from the point of view of
	//undoing, they should be viewed as a single operation.
	//When end_command_group() is called, all calls to execute_command since
	//the corresponding call to begin_command_group() will be rolled up
	//into a single command.
	//
	//These functions are re-entrant.
	void begin_command_group();
	void end_command_group();

	void draw_gui() const;

	//We are currently playing a level we are editing, and we want
	//to reset it to its initial state.
	void reset_playing_level(bool keep_player=true);

	void toggle_pause() const;
	void toggle_code();

	bool has_keyboard_focus() const;

	void start_adding_points(const std::string& field_name);
	const std::string& adding_points() const { return adding_points_; }

	int level_state_id() const { return level_changed_; }

	void mutate_object_value(level_ptr lvl, entity_ptr e, const std::string& value, variant new_value);

private:
	editor(const editor&);
	void operator=(const editor&);

	//Are we editing a level that is actually being played and in motion?
	bool editing_level_being_played() const;

	void reset_dialog_positions();

	void handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	void handle_mouse_button_up(const SDL_MouseButtonEvent& event);
	void handle_key_press(const SDL_KeyboardEvent& key);

	void handle_object_dragging(int mousex, int mousey);
	void handle_drawing_rect(int mousex, int mousey);

	void process_ghost_objects();
	void remove_ghost_objects();
	void draw() const;
	void draw_selection(int xoffset, int yoffset) const;

	void add_tile_rect(int x1, int y1, int x2, int y2);
	void remove_tile_rect(int x1, int y1, int x2, int y2);
	void select_tile_rect(int x1, int y1, int x2, int y2);
	void select_magic_wand(int xpos, int ypos);

	void set_selection(const tile_selection& s);

	void execute_shift_object(entity_ptr e, int dx, int dy);

	void move_object(level_ptr lvl, entity_ptr e, int delta_x, int delta_y);

	bool editing_objects() const { return tool_ == TOOL_ADD_OBJECT || tool_ == TOOL_SELECT_OBJECT; }
	bool editing_tiles() const { return !editing_objects(); }

	//functions which add and remove an object from a level, as well as
	//sending the object appropriate events.
	void add_multi_object_to_level(level_ptr lvl, entity_ptr e);
	void add_object_to_level(level_ptr lvl, entity_ptr e);
	void remove_object_from_level(level_ptr lvl, entity_ptr e);

	void generate_mutate_commands(entity_ptr e, const std::string& attr, variant new_value,
	                              std::vector<boost::function<void()> >& undo,
	                              std::vector<boost::function<void()> >& redo);

	void generate_remove_commands(entity_ptr e, std::vector<boost::function<void()> >& undo, std::vector<boost::function<void()> >& redo);

	CKey key_;

	level_ptr lvl_;

	std::vector<level_ptr> levels_;
	int zoom_;
	int xpos_, ypos_;
	int anchorx_, anchory_;

	// X and Y resolution of the editor, 0 means use default.
	int xres_, yres_;

	//if we are dragging an entity around, this marks the position from
	//which the entity started the drag.
	int selected_entity_startx_, selected_entity_starty_;
	std::string filename_;

	//If we are currently adding points to an object, this is non-empty
	//and has the name of the field we're adding points to. The object
	//being edited will always be lvl.editor_highlight()
	std::string adding_points_;

	EDIT_TOOL tool_;
	bool done_;
	bool face_right_;
	bool upside_down_;
	int cur_tileset_;

	int cur_object_;

	tile_selection tile_selection_;

	boost::scoped_ptr<editor_menu_dialog> editor_menu_dialog_;
	boost::scoped_ptr<editor_mode_dialog> editor_mode_dialog_;
	boost::scoped_ptr<editor_dialogs::character_editor_dialog> character_dialog_;
	boost::scoped_ptr<editor_dialogs::editor_layers_dialog> layers_dialog_;
	boost::scoped_ptr<editor_dialogs::property_editor_dialog> property_dialog_;
	boost::scoped_ptr<editor_dialogs::tileset_editor_dialog> tileset_dialog_;

	boost::scoped_ptr<editor_dialogs::segment_editor_dialog> segment_dialog_;

	boost::scoped_ptr<code_editor_dialog> code_dialog_;

	external_text_editor_ptr external_code_editor_;

	void set_code_file();

	gui::dialog* current_dialog_;

	//if the mouse is currently down, drawing a rect.
	bool drawing_rect_, dragging_;

	struct executable_command {
		boost::function<void()> redo_command;
		boost::function<void()> undo_command;
		EXECUTABLE_COMMAND_TYPE type;
	};

	std::vector<executable_command> undo_, redo_;

	//a temporary undo which is used for when we execute commands on
	//a temporary basis -- e.g. for a preview -- so we can later undo them.
	boost::scoped_ptr<executable_command> tmp_undo_;

	//indexes into undo_ which records the beginning of the current 'group'
	//of commands. When begin_command_group() is called, a value is added
	//set to the size of undo_. When end_command_group() is called, all
	//commands with index > the top value are aggregated into a single command,
	//and the top value is popped.
	std::stack<int> undo_commands_groups_;

	std::vector<entity_ptr> ghost_objects_;

	int level_changed_;
	int selected_segment_;

	int prev_mousex_, prev_mousey_;
};

struct editor_resolution_manager : private preferences::editor_screen_size_scope
{
	static bool is_active();
	explicit editor_resolution_manager(int xres, int yres);
	~editor_resolution_manager();
	
	int original_width_, original_height_;
};

#endif // !NO_EDITOR
#endif
