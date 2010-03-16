#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include <iostream>
#include <cmath>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "border_widget.hpp"
#include "button.hpp"
#include "character_editor_dialog.hpp"
#include "collision_utils.hpp"
#include "draw_tile.hpp"
#include "debug_console.hpp"
#include "editor.hpp"
#include "editor_dialogs.hpp"
#include "editor_formula_functions.hpp"
#include "editor_layers_dialog.hpp"
#include "editor_level_properties_dialog.hpp"
#include "editor_stats_dialog.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "frame.hpp"
#include "grid_widget.hpp"
#include "group_property_editor_dialog.hpp"
#include "image_widget.hpp"
#include "key.hpp"
#include "label.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "load_level.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "property_editor_dialog.hpp"
#include "raster.hpp"
#include "stats.hpp"
#include "texture.hpp"
#include "text_entry_widget.hpp"
#include "tile_map.hpp"
#include "tileset_editor_dialog.hpp"
#include "tooltip.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

#include "IMG_savepng.h"

namespace {
//keep a map of editors so that when we edit a level and then later
//come back to it we'll save all the state we had previously
std::map<std::string, editor*> all_editors;

//the last level we edited
std::string& g_last_edited_level() {
	static std::string str;
	return str;
}

bool g_draw_stats = false;

void toggle_draw_stats() {
	g_draw_stats = !g_draw_stats;
}
}

class editor_menu_dialog : public gui::dialog
{
	struct menu_item {
		std::string description;
		std::string hotkey;
		boost::function<void()> action;
	};

	void execute_menu_item(const std::vector<menu_item>& items, int n) {
		if(n >= 0 && n < items.size()) {
			items[n].action();
		}

		remove_widget(context_menu_);
		context_menu_.reset();
	}

	void show_file_menu() {
		menu_item items[] = {
			"New...", "", boost::bind(&editor_menu_dialog::new_level, this),
			"Open...", "ctrl+o", boost::bind(&editor_menu_dialog::open_level, this),
			"Save", "ctrl+s", boost::bind(&editor::save_level, &editor_),
			"Save As...", "", boost::bind(&editor_menu_dialog::save_level_as, this),
			"Exit", "<esc>", boost::bind(&editor::quit, &editor_),
		};

		std::vector<menu_item> res;
		foreach(const menu_item& m, items) {
			res.push_back(m);
		}
		show_menu(res);
	}

	void show_edit_menu() {
		menu_item items[] = {
			"Level Properties", "", boost::bind(&editor::edit_level_properties, &editor_),
			"Undo", "u", boost::bind(&editor::undo_command, &editor_),
			"Redo", "r", boost::bind(&editor::redo_command, &editor_),
		};

		std::vector<menu_item> res;
		foreach(const menu_item& m, items) {
			res.push_back(m);
		}
		show_menu(res);
	}

	void show_view_menu() {
		menu_item items[] = {
			"Zoom Out", "x", boost::bind(&editor::zoom_out, &editor_),
			"Zoom In", "z", boost::bind(&editor::zoom_in, &editor_),
			editor_.get_level().show_foreground() ? "Hide Foreground" : "Show Foreground", "f", boost::bind(&level::set_show_foreground, &editor_.get_level(), !editor_.get_level().show_foreground()),
			editor_.get_level().show_background() ? "Hide Background" : "Show Background", "b", boost::bind(&level::set_show_background, &editor_.get_level(), !editor_.get_level().show_background()),
			g_draw_stats ? "Hide Stats" : "Show Stats", "", toggle_draw_stats
		};

		std::vector<menu_item> res;
		foreach(const menu_item& m, items) {
			res.push_back(m);
		}
		show_menu(res);
	}

	void show_stats_menu() {
		menu_item items[] = {
		        "Details...", "", boost::bind(&editor::show_stats, &editor_),
		        "Refresh stats", "", boost::bind(&editor::download_stats, &editor_),
		};
		std::vector<menu_item> res;
		foreach(const menu_item& m, items) {
			res.push_back(m);
		}
		show_menu(res);
	}

	void show_scripts_menu() {
		std::vector<menu_item> res;
		foreach(const editor_script::info& script, editor_script::all_scripts()) {
			menu_item item = { script.name, "", boost::bind(&editor::run_script, &editor_, script.name) };
			res.push_back(item);
		}
		
		show_menu(res);
	}

	void show_window_menu() {
		std::vector<menu_item> res;
		for(std::map<std::string, editor*>::const_iterator i = all_editors.begin(); i != all_editors.end(); ++i) {
			std::string name = i->first;
			if(name == g_last_edited_level()) {
				name += " *";
			}
			menu_item item = { name, "", boost::bind(&editor_menu_dialog::open_level_in_editor, this, i->first) };
			res.push_back(item);
		}
		show_menu(res);
	}

	void show_menu(const std::vector<menu_item>& items) {
		using namespace gui;
		gui::grid* grid = new gui::grid(2);
		grid->set_hpad(40);
		grid->set_show_background(true);
		grid->allow_selection();
		grid->swallow_clicks();
		grid->register_selection_callback(boost::bind(&editor_menu_dialog::execute_menu_item, this, items, _1));
		foreach(const menu_item& item, items) {
			grid->add_col(widget_ptr(new label(item.description, graphics::color_white()))).
			      add_col(widget_ptr(new label(item.hotkey, graphics::color_white())));
		}

		int mousex, mousey;
		SDL_GetMouseState(&mousex, &mousey);

		mousex -= x();
		mousey -= y();

		remove_widget(context_menu_);
		context_menu_.reset(grid);
		add_widget(context_menu_, mousex, mousey);
	}

	editor& editor_;
	gui::widget_ptr context_menu_;
public:
	explicit editor_menu_dialog(editor& e)
	  : gui::dialog(0, 0, graphics::screen_width() - 160, 40), editor_(e)
	{
		init();
	}

	void init() {
		clear();

		using namespace gui;
		gui::grid* grid = new gui::grid(6);
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("File", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_file_menu, this))));
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("Edit", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_edit_menu, this))));
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("View", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_view_menu, this))));
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("Window", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_window_menu, this))));
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("Statistics", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_stats_menu, this))));
		add_widget(widget_ptr(grid));

		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("Scripts", graphics::color_white())),
		             boost::bind(&editor_menu_dialog::show_scripts_menu, this))));
		add_widget(widget_ptr(grid));
	}

	void new_level() {
		using namespace gui;
		dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
		d.add_widget(widget_ptr(new label("New Level", graphics::color_white(), 48)));
		text_entry_widget* entry = new text_entry_widget;
		d.add_widget(widget_ptr(new label("Name:", graphics::color_white())))
		 .add_widget(widget_ptr(entry));
		d.show_modal();
		
		std::string name = entry->text();
		if(name.empty() == false) {
			sys::write_file("data/level/" + name, "[level]\n[/level]\n");
			editor_.close();
			g_last_edited_level() = name;
		}
	}

	void save_level_as() {
		using namespace gui;
		dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
		d.add_widget(widget_ptr(new label("Save As", graphics::color_white(), 48)));
		text_entry_widget* entry = new text_entry_widget;
		d.add_widget(widget_ptr(new label("Name:", graphics::color_white())))
		 .add_widget(widget_ptr(entry));
		d.show_modal();
		
		if(!d.cancelled() && entry->text().empty() == false) {
			editor_.save_level_as(entry->text());
		}
	}

	void open_level() {
		open_level_in_editor(show_choose_level_dialog("Open Level"));
	}

	void open_level_in_editor(const std::string& lvl) {
		if(lvl.empty() == false && lvl != g_last_edited_level()) {
			remove_widget(context_menu_);
			context_menu_.reset();
			editor_.close();
			g_last_edited_level() = lvl;
		}
	}
};

namespace {
const char* ModeStrings[] = {"Tiles", "Objects", "Properties",};

const char* ToolStrings[] = {
  "Add tiles by drawing rectangles",
  "Select Tiles",
  "Select connected regions of tiles",
  "Add tiles by drawing pencil strokes",
  "Pick tiles or objects",
  "Add Objects",
  "Select Objects",
};

const char* ToolIcons[] = {"editor_draw_rect", "editor_rect_select", "editor_wand", "editor_pencil", "editor_eyedropper", "editor_add_object", "editor_select_object",};
}

class editor_mode_dialog : public gui::dialog
{
	editor& editor_;
	gui::widget_ptr context_menu_;

	void select_tool(int tool)
	{
		if(tool >= 0 && tool < editor::NUM_TOOLS) {
			editor_.change_tool(static_cast<editor::EDIT_TOOL>(tool));
		}
	}

	bool handle_event(const SDL_Event& event, bool claimed)
	{
		if(!claimed) {
			const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
			if(ctrl_pressed) {
				return false;
			}

			switch(event.type) {
			case SDL_KEYDOWN: {
				switch(event.key.keysym.sym) {
					//TODO: add short cuts for tools here.

				}

				break;
			}
			}
		}

		return claimed || dialog::handle_event(event, claimed);
	}

public:
	explicit editor_mode_dialog(editor& e)
	  : gui::dialog(graphics::screen_width() - 160, 0, 160, 160), editor_(e)
	{
		init();
	}

	void init()
	{
		clear();
		using namespace gui;

		grid_ptr grid(new gui::grid(3));
		for(int n = 0; n != editor::NUM_TOOLS; ++n) {
			button_ptr tool_button(
			  new button(widget_ptr(new gui_section_widget(ToolIcons[n], 26, 26)),
			             boost::bind(&editor_mode_dialog::select_tool, this, n)));
			tool_button->set_tooltip(ToolStrings[n]);
			grid->add_col(widget_ptr(new border_widget(tool_button, n == editor_.tool() ? graphics::color(255,255,255,255) : graphics::color(0,0,0,0))));
		}

		grid->finish_row();
		add_widget(grid, 5, 5);
	}
};

namespace {

const int RectEdgeSelectThreshold = 6;

void execute_functions(const std::vector<boost::function<void()> >& v) {
	foreach(const boost::function<void()>& f, v) {
		f();
	}
}

//the tiles that we've drawn in the current action.
std::vector<point> g_current_draw_tiles;

const editor_variable_info* g_variable_editing = NULL;
int g_variable_editing_original_value = 0;
const editor_variable_info* variable_info_selected(const_entity_ptr e, int xpos, int ypos, int zoom)
{
	if(!e || !e->editor_info()) {
		return NULL;
	}

	foreach(const editor_variable_info& var, e->editor_info()->vars()) {
		const variant value = e->query_value(var.variable_name());
		switch(var.type()) {
			case editor_variable_info::XPOSITION: {
				if(!value.is_int()) {
					break;
				}

				if(xpos >= value.as_int() - zoom*RectEdgeSelectThreshold && xpos <= value.as_int() + zoom*RectEdgeSelectThreshold) {
					return &var;
				}
				break;
			}
			case editor_variable_info::YPOSITION: {
				if(!value.is_int()) {
					break;
				}

				if(ypos >= value.as_int() - zoom*RectEdgeSelectThreshold && ypos <= value.as_int() + zoom*RectEdgeSelectThreshold) {
					return &var;
				}
				break;
			}
			default:
				break;
		}
	}

	return NULL;
}

int round_tile_size(int n)
{
	if(n >= 0) {
		return n - n%TileSize;
	} else {
		n = -n + 32;
		return -(n - n%TileSize);
	}
}

bool resizing_left_level_edge = false,
     resizing_right_level_edge = false,
     resizing_top_level_edge = false,
     resizing_bottom_level_edge = false;

rect modify_selected_rect(rect boundaries, int xpos, int ypos) {

	const int x = round_tile_size(xpos);
	const int y = round_tile_size(ypos);

	if(resizing_left_level_edge) {
		boundaries = rect::from_coordinates(x, boundaries.y(), boundaries.x2(), boundaries.y2());
	}

	if(resizing_right_level_edge) {
		boundaries = rect::from_coordinates(boundaries.x(), boundaries.y(), x, boundaries.y2());
	}

	if(resizing_top_level_edge) {
		boundaries = rect::from_coordinates(boundaries.x(), y, boundaries.x2(), boundaries.y2());
	}

	if(resizing_bottom_level_edge) {
		boundaries = rect::from_coordinates(boundaries.x(), boundaries.y(), boundaries.x2(), y);
	}

	return boundaries;
}

//find if an edge of a rectangle is selected
bool rect_left_edge_selected(const rect& r, int x, int y, int zoom) {
	return y >= r.y() - RectEdgeSelectThreshold*zoom &&
	       y <= r.y2() + RectEdgeSelectThreshold*zoom &&
	       x >= r.x() - RectEdgeSelectThreshold*zoom &&
	       x <= r.x() + RectEdgeSelectThreshold*zoom;
}

bool rect_right_edge_selected(const rect& r, int x, int y, int zoom) {
	return y >= r.y() - RectEdgeSelectThreshold*zoom &&
	       y <= r.y2() + RectEdgeSelectThreshold*zoom &&
	       x >= r.x2() - RectEdgeSelectThreshold*zoom &&
	       x <= r.x2() + RectEdgeSelectThreshold*zoom;
}

bool rect_top_edge_selected(const rect& r, int x, int y, int zoom) {
	return x >= r.x() - RectEdgeSelectThreshold*zoom &&
	       x <= r.x2() + RectEdgeSelectThreshold*zoom &&
	       y >= r.y() - RectEdgeSelectThreshold*zoom &&
	       y <= r.y() + RectEdgeSelectThreshold*zoom;
}

bool rect_bottom_edge_selected(const rect& r, int x, int y, int zoom) {
	return x >= r.x() - RectEdgeSelectThreshold*zoom &&
	       x <= r.x2() + RectEdgeSelectThreshold*zoom &&
	       y >= r.y2() - RectEdgeSelectThreshold*zoom &&
	       y <= r.y2() + RectEdgeSelectThreshold*zoom;
}

std::vector<editor::tileset> tilesets;

std::vector<editor::enemy_type> enemy_types;

int selected_property = 0;

}

void editor::enemy_type::init(wml::const_node_ptr node)
{
	const std::vector<const_custom_object_type_ptr> types = custom_object_type::get_all();
	foreach(const_custom_object_type_ptr t, types) {
		if(t->editor_info()) {
			enemy_types.push_back(editor::enemy_type(*t));
		}
	}
}

editor::enemy_type::enemy_type(const custom_object_type& type)
  : category(type.editor_info()->category())
{
	wml::node_ptr new_node(new wml::node("character"));
	new_node->set_attr("type", type.id());
	new_node->set_attr("custom", "yes");
	new_node->set_attr("face_right", "false");
	new_node->set_attr("x", "1500");
	new_node->set_attr("y", "0");

	if(type.is_human()) {
		new_node->set_attr("is_human", "true");
	}

	node = new_node;
	preview_object = entity::build(node);
	preview_frame = &preview_object->current_frame();
}

void editor::tileset::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("tileset");
	wml::node::const_child_iterator i2 = node->end_child("tileset");
	for(; i1 != i2; ++i1) {
		tilesets.push_back(editor::tileset(i1->second));
	}
}

editor::tileset::tileset(wml::const_node_ptr node)
  : category(node->attr("category")), type(node->attr("type")),
    zorder(wml::get_int(node, "zorder")),
	x_speed(wml::get_int(node, "x_speed", 100)),
	y_speed(wml::get_int(node, "y_speed", 100)),
	sloped(wml::get_bool(node, "sloped"))
{
	if(node->get_child("preview")) {
		preview.reset(new tile_map(node->get_child("preview")));
	}
}

void editor::edit(const char* level_cfg, int xpos, int ypos)
{
	editor*& e = all_editors[level_cfg];
	if(!e) {
		e = new editor(level_cfg);
	}

	if(xpos != -1) {
		e->xpos_ = xpos;
		e->ypos_ = ypos;
	}

	e->edit_level();
	if(g_last_edited_level() != level_cfg) {
		//a new level was set, so start editing it now.
		edit(g_last_edited_level().c_str());
	}
}

std::string editor::last_edited_level()
{
	return g_last_edited_level();
}

editor::editor(const char* level_cfg)
  : zoom_(1), xpos_(0), ypos_(0), anchorx_(0), anchory_(0),
    selected_entity_startx_(0), selected_entity_starty_(0),
    filename_(level_cfg), tool_(TOOL_ADD_RECT),
    done_(false), face_right_(true),
	cur_tileset_(0), cur_object_(0),
    current_dialog_(NULL),
	drawing_rect_(false), dragging_(false), level_changed_(0)
{
	editor_menu_dialog_.reset(new editor_menu_dialog(*this));
	editor_mode_dialog_.reset(new editor_mode_dialog(*this));

	static bool first_time = true;
	if(first_time) {
		wml::const_node_ptr editor_cfg = wml::parse_wml(sys::read_file("data/editor.cfg"));
		tile_map::load_all();
		tileset::init(editor_cfg);
		enemy_type::init(editor_cfg);
		first_time = false;
	}

	assert(!tilesets.empty());
	lvl_.reset(new level(level_cfg));
	lvl_->finish_loading();
	lvl_->set_editor();
	lvl_->set_as_current_level();

	group_property_dialog_.reset(new editor_dialogs::group_property_editor_dialog(*this));
	property_dialog_.reset(new editor_dialogs::property_editor_dialog(*this));
}

editor::~editor()
{
}

void editor::group_selection()
{
	const int group = lvl_->add_group();
	std::vector<boost::function<void()> > undo, redo;

	foreach(const entity_ptr& c, lvl_->editor_selection()) {
		undo.push_back(boost::bind(&level::set_character_group, lvl_.get(), c, c->group()));
		redo.push_back(boost::bind(&level::set_character_group, lvl_.get(), c, group));
	}

	execute_command(
	  boost::bind(execute_functions, redo),
	  boost::bind(execute_functions, undo));
}

void editor::toggle_facing()
{
	face_right_ = !face_right_;
	if(character_dialog_) {
		character_dialog_->init();
	}
}

void editor::process_ghost_objects()
{
	lvl_->swap_chars(ghost_objects_);

	const size_t num_chars_before = lvl_->get_chars().size();
	const std::vector<entity_ptr> chars = lvl_->get_chars();
	foreach(const entity_ptr& p, chars) {
		p->process(*lvl_);
	}

	lvl_->swap_chars(ghost_objects_);

	foreach(entity_ptr& p, ghost_objects_) {
		if(p->destroyed()) {
			lvl_->remove_character(p);
			p = entity_ptr();
		}
	}

	ghost_objects_.erase(std::remove(ghost_objects_.begin(), ghost_objects_.end(), entity_ptr()), ghost_objects_.end());
}

void editor::remove_ghost_objects()
{
	foreach(entity_ptr c, ghost_objects_) {
		lvl_->remove_character(c);
	}
}

namespace {
unsigned int get_mouse_state(int& mousex, int& mousey) {
	unsigned int res = SDL_GetMouseState(&mousex, &mousey);
	mousex = (mousex*graphics::screen_width())/preferences::virtual_screen_width();
	mousey = (mousey*graphics::screen_height())/preferences::virtual_screen_height();

	return res;
}
}

void editor::edit_level()
{
	stats::flush();
	try {
		load_stats();
	} catch(...) {
		debug_console::add_message("Error parsing stats");
		std::cerr << "ERROR LOADING STATS\n";
	}

	g_last_edited_level() = filename_;

	tileset_dialog_.reset(new editor_dialogs::tileset_editor_dialog(*this));
	layers_dialog_.reset(new editor_dialogs::editor_layers_dialog(*this));
	current_dialog_ = tileset_dialog_.get();

	glEnable(GL_BLEND);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int selected_tile = 0;

	//reset the tool status.
	change_tool(tool_);

	done_ = false;
	int prev_mousex = -1, prev_mousey = -1;
	while(!done_) {
		process_ghost_objects();
		handle_scrolling();

		int mousex, mousey;
		const unsigned int buttons = get_mouse_state(mousex, mousey);

		if(buttons == 0) {
			drawing_rect_ = false;
		}

		//make middle-clicking drag the screen around.
		if(prev_mousex != -1 && prev_mousey != -1 && (buttons&SDL_BUTTON_MIDDLE)) {
			const int diff_x = mousex - prev_mousex;
			const int diff_y = mousey - prev_mousey;
			xpos_ -= diff_x*zoom_;
			ypos_ -= diff_y*zoom_;
		}

		prev_mousex = mousex;
		prev_mousey = mousey;

		const int selectx = round_tile_size(xpos_ + mousex*zoom_);
		const int selecty = round_tile_size(ypos_ + mousey*zoom_);

		const bool object_mode = (tool() == TOOL_ADD_OBJECT || tool() == TOOL_SELECT_OBJECT);
		if(property_dialog_ && g_variable_editing) {
			int diff = 0;
			switch(g_variable_editing->type()) {
			case editor_variable_info::XPOSITION:
				diff = (xpos_ + mousex*zoom_) - anchorx_;
				break;
			case editor_variable_info::YPOSITION:
				diff = (ypos_ + mousey*zoom_) - anchory_;
				break;
			default:
				break;
			}

			if(property_dialog_ && property_dialog_->get_entity()) {
				property_dialog_->get_entity()->mutate_value(g_variable_editing->variable_name(), variant(g_variable_editing_original_value + diff));
			}
		} else if(object_mode && !buttons) {
			//remove ghost objects and re-add them. This guarantees ghost
			//objects always remain at the end of the level ordering.
			remove_ghost_objects();
			entity_ptr c = lvl_->get_character_at_point(xpos_ + mousex*zoom_, ypos_ + mousey*zoom_);
			foreach(const entity_ptr& ghost, ghost_objects_) {
				lvl_->add_character(ghost);
			}

			lvl_->set_editor_highlight(c);
			//See if we should add ghost objects. Human objects don't get
			//ghost (it doesn't make much sense for them to do so)
			if(ghost_objects_.empty() && c && !c->is_human()) {
				//we have an object but no ghost for it, make the
				//object's ghost and deploy it.
				entity_ptr clone = c->clone();
				if(clone) {
					ghost_objects_.push_back(clone);
					lvl_->add_character(clone);
				}
			} else if(ghost_objects_.empty() == false && !c) {
				//ghost objects are present but we are no longer moused-over
				//an object, so remove the ghosts.
				remove_ghost_objects();
				ghost_objects_.clear();
			}
		} else if(object_mode && lvl_->editor_highlight()) {
			//we're handling objects, and a button is down, and we have an
			//object under the mouse. This means we are dragging something.
			handle_object_dragging(mousex, mousey);
		}

		if(!object_mode) {
			//not in object mode, so make sure there are no ghost objects
			//or objects highlighted.
			lvl_->set_editor_highlight(entity_ptr());
			remove_ghost_objects();
			ghost_objects_.clear();
		}

		//if we're drawing with a pencil see if we add a new tile
		if(tool() == TOOL_PENCIL && buttons && mousey > editor_menu_dialog_->height() && mousex < editor_mode_dialog_->x()) {
			const int xpos = xpos_ + mousex*zoom_;
			const int ypos = ypos_ + mousey*zoom_;
			point p(round_tile_size(xpos), round_tile_size(ypos));
			if(std::find(g_current_draw_tiles.begin(), g_current_draw_tiles.end(), p) == g_current_draw_tiles.end()) {
				g_current_draw_tiles.push_back(p);

				if(buttons&SDL_BUTTON_LEFT) {
					add_tile_rect(p.x, p.y, p.x, p.y);
				} else {
					remove_tile_rect(p.x, p.y, p.x, p.y);
				}
			}
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {

			if(editor_menu_dialog_->process_event(event, false)) {
				continue;
			}

			if(editor_mode_dialog_->process_event(event, false)) {
				continue;
			}

			if(current_dialog_ && current_dialog_->process_event(event, false)) {
				continue;
			}

			if(layers_dialog_ && layers_dialog_->process_event(event, false)) {
				continue;
			}
			
			switch(event.type) {
			case SDL_QUIT:
				done_ = true;
				break;
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE) {
					if(confirm_quit()) {
						return;
					}
				}

				handle_key_press(event.key);
				break;

				break;
			case SDL_MOUSEBUTTONDOWN:
				handle_mouse_button_down(event.button);
				break;

			case SDL_MOUSEBUTTONUP:
				handle_mouse_button_up(event.button);
				break;
			default:
				break;
			}
		}

		draw();

		lvl_->complete_rebuild_tiles_in_background();
	}
}

void editor::handle_key_press(const SDL_KeyboardEvent& key)
{
	if(key.keysym.sym == SDLK_s && (key.keysym.mod&KMOD_ALT)) {
		IMG_SaveFrameBuffer("screenshot.png", 5);
	}

	if(key.keysym.sym == SDLK_u) {
		undo_command();
	}

	if(key.keysym.sym == SDLK_r &&
	   !(key.keysym.mod&KMOD_CTRL)) {
		redo_command();
	}

	if(key.keysym.sym == SDLK_z) {
		zoom_in();
	}

	if(key.keysym.sym == SDLK_x) {
		zoom_out();
	}

	if(key.keysym.sym == SDLK_f) {
		lvl_->set_show_foreground(!lvl_->show_foreground());
	}

	if(key.keysym.sym == SDLK_b) {
		lvl_->set_show_background(!lvl_->show_background());
	}

	if(editing_objects() && (key.keysym.sym == SDLK_DELETE || key.keysym.sym == SDLK_BACKSPACE) && lvl_->editor_selection().empty() == false) {
		//deleting objects. We clear the selection as well as
		//deleting. To undo, the previous selection will be cleared,
		//and then the deleted objects re-selected.
		std::vector<boost::function<void()> > redo, undo;
		undo.push_back(boost::bind(&level::editor_clear_selection, lvl_.get()));

		//if we undo, return the objects to the property dialog
		undo.push_back(boost::bind(&editor_dialogs::group_property_editor_dialog::set_group, group_property_dialog_.get(), lvl_->editor_selection()));
		redo.push_back(boost::bind(&level::editor_clear_selection, lvl_.get()));
		//we want to clear the objects in the property dialog
		redo.push_back(boost::bind(&editor_dialogs::group_property_editor_dialog::set_group, group_property_dialog_.get(), std::vector<entity_ptr>()));
		foreach(const entity_ptr& e, lvl_->editor_selection()) {
			redo.push_back(boost::bind(&level::remove_character, lvl_.get(), e));
			undo.push_back(boost::bind(&level::add_character, lvl_.get(), e));
			undo.push_back(boost::bind(&level::editor_select_object, lvl_.get(), e));
		}
		execute_command(
		  boost::bind(execute_functions, redo),
		  boost::bind(execute_functions, undo));
	}

	if(!tile_selection_.empty() && (key.keysym.sym == SDLK_DELETE || key.keysym.sym == SDLK_BACKSPACE)) {
		int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
		std::vector<boost::function<void()> > redo, undo;
		foreach(const point& p, tile_selection_.tiles) {
			const int x = p.x*TileSize;
			const int y = p.y*TileSize;

			min_x = std::min(x, min_x);
			max_x = std::max(x, max_x);
			min_y = std::min(y, min_y);
			max_y = std::max(y, max_y);

			redo.push_back(boost::bind(&level::clear_tile_rect, lvl_.get(), x, y, x, y));
			std::map<int, std::vector<std::string> > old_tiles;
			lvl_->get_all_tiles_rect(x, y, x, y, old_tiles);
			for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
				undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x, y, x, y, i->second));
			}
		}

		if(!tile_selection_.tiles.empty()) {
			undo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), min_x, min_y, max_x, max_y));
			redo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), min_x, min_y, max_x, max_y));
			undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));
			redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));
		}

		execute_command(
		  boost::bind(execute_functions, redo),
		  boost::bind(execute_functions, undo));
	}

	if(key.keysym.sym == SDLK_o) {
		editor_menu_dialog_->open_level();
	}

	if(key.keysym.sym == SDLK_s) {
		save_level();
	}

	if(key.keysym.sym == SDLK_f) {
		toggle_facing();
	}

	if(key.keysym.sym == SDLK_r &&
	   (key.keysym.mod&KMOD_CTRL)) {
		lvl_->rebuild_tiles();
	}

	if(key.keysym.sym == SDLK_c) {
		foreach(const entity_ptr& obj, lvl_->get_chars()) {
			if(entity_collides_with_level(*lvl_, *obj, MOVE_NONE)) {
				xpos_ = obj->x() - graphics::screen_width()/2;
				ypos_ = obj->y() - graphics::screen_height()/2;
				break;
			}
		}
	}
}

void editor::handle_scrolling()
{
	const int ScrollSpeed = 24*zoom_;
	const int FastScrollSpeed = 48*zoom_;

	if(key_[SDLK_LEFT]) {
		xpos_ -= ScrollSpeed;
		if(key_[SDLK_KP0]) {
			xpos_ -= FastScrollSpeed;
		}
	}

	if(key_[SDLK_RIGHT]) {
		xpos_ += ScrollSpeed;
		if(key_[SDLK_KP0]) {
			xpos_ += FastScrollSpeed;
		}
	}

	if(key_[SDLK_UP]) {
		ypos_ -= ScrollSpeed;
		if(key_[SDLK_KP0]) {
			ypos_ -= FastScrollSpeed;
		}
	}

	if(key_[SDLK_DOWN]) {
		ypos_ += ScrollSpeed;
		if(key_[SDLK_KP0]) {
			ypos_ += FastScrollSpeed;
		}
	}
}

void editor::handle_object_dragging(int mousex, int mousey)
{
	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	const int dx = xpos_ + mousex*zoom_ - anchorx_;
	const int dy = ypos_ + mousey*zoom_ - anchory_;
	const int xpos = selected_entity_startx_ + dx;
	const int ypos = selected_entity_starty_ + dy;

	const int new_x = xpos - (ctrl_pressed ? 0 : (xpos%TileSize));
	const int new_y = ypos - (ctrl_pressed ? 0 : (ypos%TileSize));

	const int delta_x = new_x - lvl_->editor_highlight()->x();
	const int delta_y = new_y - lvl_->editor_highlight()->y();

	//don't move the object from its starting position until the
	//delta in movement is large enough.
	const bool in_starting_position =
	  lvl_->editor_highlight()->x() == selected_entity_startx_ &&
	  lvl_->editor_highlight()->y() == selected_entity_starty_;
	const bool too_small_to_move = in_starting_position &&
	         abs(dx) < 5 && abs(dy) < 5;

	if(!too_small_to_move && (new_x != lvl_->editor_highlight()->x() || new_y != lvl_->editor_highlight()->y())) {
		std::vector<boost::function<void()> > redo, undo;

		foreach(const entity_ptr& e, lvl_->editor_selection()) {
			redo.push_back(boost::bind(&editor::move_object, this, e, delta_x, delta_y));
			undo.push_back(boost::bind(&editor::move_object, this, e, -delta_x, -delta_y));
			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));
		}

		remove_ghost_objects();
		ghost_objects_.clear();
	}
}

void editor::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	const bool shift_pressed = (SDL_GetModState()&(KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
	int mousex, mousey;
	const unsigned int buttons = get_mouse_state(mousex, mousey);

	anchorx_ = xpos_ + mousex*zoom_;
	anchory_ = ypos_ + mousey*zoom_;
	if(event.button == SDL_BUTTON_MIDDLE) {
		return;
	}

	resizing_left_level_edge = rect_left_edge_selected(lvl_->boundaries(), anchorx_, anchory_, zoom_);
	resizing_right_level_edge = rect_right_edge_selected(lvl_->boundaries(), anchorx_, anchory_, zoom_);
	resizing_top_level_edge = rect_top_edge_selected(lvl_->boundaries(), anchorx_, anchory_, zoom_);
	resizing_bottom_level_edge = rect_bottom_edge_selected(lvl_->boundaries(), anchorx_, anchory_, zoom_);

	if(resizing_left_level_edge || resizing_right_level_edge || resizing_top_level_edge || resizing_bottom_level_edge) {
		return;
	}

	dragging_ = drawing_rect_ = false;

	if(tool() == TOOL_PICKER) {
		std::cerr << "pick tile...\n";
		//pick the top most tile at this point.
		std::map<int, std::vector<std::string> > tiles;
		lvl_->get_all_tiles_rect(anchorx_, anchory_, anchorx_, anchory_, tiles);
		std::string tile;
		for(std::map<int, std::vector<std::string> >::reverse_iterator i = tiles.rbegin(); i != tiles.rend(); ++i) {
			if(i->second.empty() == false) {
				tile = i->second.back();
				std::cerr << "picking tile: '" << tile << "'\n";
				break;
			}
		}

		if(!tile.empty()) {
			for(int n = 0; n != all_tilesets().size(); ++n) {
				if(all_tilesets()[n].type == tile) {
					tileset_dialog_->select_category(all_tilesets()[n].category);
					tileset_dialog_->set_tileset(n);
					std::cerr << "pick tile " << n << "\n";
					return;
				}
			}
		}
	} else if(editing_tiles() && !tile_selection_.empty() &&
	   std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(round_tile_size(anchorx_)/TileSize, round_tile_size(anchory_)/TileSize))) {
		//we are beginning to drag our selection
		dragging_ = true;
	} else if(tool() == TOOL_ADD_RECT || tool() == TOOL_SELECT_RECT) {
		drawing_rect_ = true;
	} else if(tool() == TOOL_MAGIC_WAND) {
		drawing_rect_ = false;
	} else if(tool() == TOOL_PENCIL) {
		drawing_rect_ = false;
		point p(round_tile_size(anchorx_), round_tile_size(anchory_));
		if(buttons&SDL_BUTTON_LEFT) {
			add_tile_rect(p.x, p.y, p.x, p.y);
		} else {
			remove_tile_rect(p.x, p.y, p.x, p.y);
		}
		g_current_draw_tiles.clear();
		g_current_draw_tiles.push_back(p);
	} else if(property_dialog_ && variable_info_selected(property_dialog_->get_entity(), anchorx_, anchory_, zoom_)) {
		g_variable_editing = variable_info_selected(property_dialog_->get_entity(), anchorx_, anchory_, zoom_);
		g_variable_editing_original_value = property_dialog_->get_entity()->query_value(g_variable_editing->variable_name()).as_int();
		
	} else if(tool() == TOOL_SELECT_OBJECT && !lvl_->editor_highlight()) {
		//dragging a rectangle to select objects
		drawing_rect_ = true;
	} else if(property_dialog_) {
		property_dialog_->set_entity(lvl_->editor_highlight());
	}

	if(lvl_->editor_highlight()) {
		if(std::count(lvl_->editor_selection().begin(),
		              lvl_->editor_selection().end(), lvl_->editor_highlight()) == 0) {
			//set the object as selected in the editor.
			if(!shift_pressed) {
				lvl_->editor_clear_selection();
			}

			lvl_->editor_select_object(lvl_->editor_highlight());
			group_property_dialog_->set_group(lvl_->editor_selection());

			if(!lvl_->editor_selection().empty() && tool() != TOOL_SELECT_OBJECT) {
				//we are in add objects mode and we clicked on an object,
				//so change to select mode.
				change_tool(TOOL_SELECT_OBJECT);
			}

			if(lvl_->editor_selection().size() > 1) {
				current_dialog_ = group_property_dialog_.get();
			} else if(lvl_->editor_selection().size() == 1) {
				current_dialog_ = property_dialog_.get();
			}
		}

		//start dragging the object
		selected_entity_startx_ = lvl_->editor_highlight()->x();
		selected_entity_starty_ = lvl_->editor_highlight()->y();

		if(tool() == TOOL_PICKER) {
			wml::const_node_ptr node = lvl_->editor_highlight()->write();
			const std::string type = node->attr("type");
			for(int n = 0; n != all_characters().size(); ++n) {
				const enemy_type& c = all_characters()[n];
				if(c.node->attr("type").str() == type) {
					character_dialog_->select_category(c.category);
					character_dialog_->set_character(n);
					return;
				}
			}
		}
	} else {
		//clear any selection in the editor
		lvl_->editor_clear_selection();
	}

	if(tool() == TOOL_ADD_OBJECT && event.button == SDL_BUTTON_LEFT && !lvl_->editor_highlight()) {
		wml::node_ptr node(wml::deep_copy(enemy_types[cur_object_].node));
		node->set_attr("x", formatter() << (ctrl_pressed ? anchorx_ : round_tile_size(anchorx_)));
		node->set_attr("y", formatter() << (ctrl_pressed ? anchory_ : round_tile_size(anchory_)));
		node->set_attr("face_right", face_right_ ? "yes" : "no");

		entity_ptr c(entity::build(node));

		//any vars that require formula initialization are calculated here.
		std::map<std::string, variant> vars;
		foreach(const editor_variable_info& info, c->editor_info()->vars()) {
			if(info.formula()) {
				vars[info.variable_name()] = info.formula()->execute(*c);
			}
		}

		//we only want to actually set the vars once we've calculated all of
		//them, to avoid any ordering issues etc. So set them all here.
		for(std::map<std::string, variant>::const_iterator i = vars.begin();
		    i != vars.end(); ++i) {
			game_logic::formula_callable* obj_vars = c->query_value("vars").mutable_callable();
			obj_vars->mutate_value(i->first, i->second);
		}

		if(!place_entity_in_level(*lvl_, *c)) {
			//could not place entity. Not really an error; the user just
			//clicked in an illegal position to place an object.

		} else if(c->is_human() && lvl_->player()) {
			execute_command(
			  boost::bind(&level::add_character, lvl_.get(), c),
			  boost::bind(&level::add_character, lvl_.get(), &lvl_->player()->get_entity()));

		} else {
			execute_command(
			  boost::bind(&level::add_character, lvl_.get(), c),
			  boost::bind(&level::remove_character, lvl_.get(), c));
		}
	}
}

void editor::handle_mouse_button_up(const SDL_MouseButtonEvent& event)
{
	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	const bool shift_pressed = (SDL_GetModState()&(KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
	int mousex, mousey;
	const unsigned int buttons = get_mouse_state(mousex, mousey);
			
	const int xpos = xpos_ + mousex*zoom_;
	const int ypos = ypos_ + mousey*zoom_;

	if(g_variable_editing) {
		if(property_dialog_ && property_dialog_->get_entity()) {
			entity_ptr e = property_dialog_->get_entity();
			const std::string& var = g_variable_editing->variable_name();
			execute_command(
			  boost::bind(&entity::mutate_value, e.get(), var, e->query_value(var)),
			  boost::bind(&entity::mutate_value, e.get(), var, variant(g_variable_editing_original_value)));
			property_dialog_->init();
		}
		g_variable_editing = NULL;
		return;
	}

	if(resizing_left_level_edge || resizing_right_level_edge ||resizing_top_level_edge || resizing_bottom_level_edge) {
		rect boundaries = modify_selected_rect(lvl_->boundaries(), xpos, ypos);

		resizing_left_level_edge = resizing_right_level_edge = resizing_top_level_edge = resizing_bottom_level_edge = false;

		if(boundaries != lvl_->boundaries()) {
			execute_command(
			  boost::bind(&level::set_boundaries, lvl_.get(), boundaries),
			  boost::bind(&level::set_boundaries, lvl_.get(), lvl_->boundaries()));
		}
		return;
	}


	if(editing_tiles()) {
		if(dragging_) {
			const int selectx = xpos_ + mousex*zoom_;
			const int selecty = ypos_ + mousey*zoom_;

			//dragging selection
			int diffx = (selectx - anchorx_)/TileSize;
			int diffy = (selecty - anchory_)/TileSize;

			std::cerr << "MAKE DIFF: " << diffx << "," << diffy << "\n";
			std::vector<boost::function<void()> > redo, undo;

			foreach(const point& p, tile_selection_.tiles) {
				const int x = (p.x+diffx)*TileSize;
				const int y = (p.y+diffy)*TileSize;
				undo.push_back(boost::bind(&level::clear_tile_rect,lvl_.get(), x, y, x, y));
			}

			int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;

			//backup both the contents of the old and new regions, so we can restore them both
			foreach(const point& p, tile_selection_.tiles) {
				int x = p.x*TileSize;
				int y = p.y*TileSize;

				min_x = std::min(x, min_x);
				max_x = std::max(x, max_x);
				min_y = std::min(y, min_y);
				max_y = std::max(y, max_y);

				std::map<int, std::vector<std::string> > old_tiles;
				lvl_->get_all_tiles_rect(x, y, x, y, old_tiles);
				for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
					undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x, y, x, y, i->second));
					redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x, y, x, y, std::vector<std::string>(1,"")));
				}

				old_tiles.clear();
	
				x += diffx*TileSize;
				y += diffy*TileSize;

				min_x = std::min(x, min_x);
				max_x = std::max(x, max_x);
				min_y = std::min(y, min_y);
				max_y = std::max(y, max_y);

				lvl_->get_all_tiles_rect(x, y, x, y, old_tiles);
				for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
					undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x, y, x, y, i->second));
					redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x, y, x, y, std::vector<std::string>(1,"")));
				}
			}

			
			foreach(const point& p, tile_selection_.tiles) {
				const int x = p.x*TileSize;
				const int y = p.y*TileSize;

				min_x = std::min(x + diffx*TileSize, min_x);
				max_x = std::max(x + diffx*TileSize, max_x);
				min_y = std::min(y + diffy*TileSize, min_y);
				max_y = std::max(y + diffy*TileSize, max_y);

				std::map<int, std::vector<std::string> > old_tiles;
				lvl_->get_all_tiles_rect(x, y, x, y, old_tiles);
				for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
					redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x + diffx*TileSize, y + diffy*TileSize, x + diffx*TileSize, y + diffy*TileSize, i->second));
				}
			}

			if(!tile_selection_.tiles.empty()) {
				undo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), min_x, min_y, max_x, max_y));
				redo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), min_x, min_y, max_x, max_y));
				undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));
				redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));
			}

			tile_selection new_selection = tile_selection_;
			foreach(point& p, new_selection.tiles) {
				p.x += diffx;
				p.y += diffy;
			}
			
			redo.push_back(boost::bind(&editor::set_selection, this, new_selection));
			undo.push_back(boost::bind(&editor::set_selection, this, tile_selection_));

			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));
			
		} else if(!drawing_rect_) {
			//wasn't drawing a rect.
			if(event.button == SDL_BUTTON_LEFT && tool() == TOOL_MAGIC_WAND) {
				select_magic_wand(anchorx_, anchory_);
			}
		} else if(event.button == SDL_BUTTON_LEFT) {

			if(tool() == TOOL_ADD_RECT) {
				add_tile_rect(anchorx_, anchory_, xpos, ypos);
			} else if(tool() == TOOL_SELECT_RECT) {
				select_tile_rect(anchorx_, anchory_, xpos, ypos);
			}
			  
		} else if(event.button == SDL_BUTTON_RIGHT) {
			remove_tile_rect(anchorx_, anchory_, xpos, ypos);
		}
	} else {
		//some kind of object editing
		if(event.button == SDL_BUTTON_RIGHT) {
			std::vector<boost::function<void()> > undo, redo;
			std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect::from_coordinates(anchorx_, anchory_, xpos, ypos));

			foreach(const entity_ptr& c, chars) {
				redo.push_back(boost::bind(&level::remove_character, lvl_.get(), c));
				undo.push_back(boost::bind(&level::add_character, lvl_.get(), c));
			}
			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));
		} else if(tool() == TOOL_SELECT_OBJECT && drawing_rect_) {
			std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect::from_coordinates(anchorx_, anchory_, xpos, ypos));
			foreach(const entity_ptr& c, chars) {
				lvl_->editor_select_object(c);
			}

			group_property_dialog_->set_group(lvl_->editor_selection());

			if(lvl_->editor_selection().size() == 1) {
				current_dialog_ = property_dialog_.get();
				property_dialog_->set_entity(lvl_->editor_selection().front());
			} else {
				current_dialog_ = group_property_dialog_.get();
			}

			if(lvl_->editor_selection().empty()) {
				//if we choose an empty selection we revert to add
				//objects mode.
				change_tool(TOOL_ADD_OBJECT);
			}
		}
	}

	drawing_rect_ = dragging_ = false;
}


void editor::load_stats()
{
	stats_.clear();

	const std::string fname = "data/stats/" + lvl_->id();
	if(!sys::file_exists(fname)) {
		return;
	}

	std::string doc = "[stats]\n" + sys::read_file(fname) + "[/stats]\n";
	wml::const_node_ptr node(wml::parse_wml(doc));
	for(wml::node::const_all_child_iterator i = node->begin_children(); i != node->end_children(); ++i) {
		stats_.push_back(stats::record::read(*i));
		if(!stats_.back()) {
			stats_.pop_back();
		}
	}
}

void editor::show_stats()
{
	editor_dialogs::editor_stats_dialog stats_dialog(*this);
	stats_dialog.show_modal();
}

void editor::download_stats()
{
	const bool result = stats::download(lvl_->id());
	if(result) {
		debug_console::add_message("Got latest stats from the server");
		try {
			load_stats();
		} catch(...) {
			debug_console::add_message("Error parsing stats");
			std::cerr << "ERROR LOADING STATS\n";
		}
	} else {
		debug_console::add_message("Download of stats failed");
	}
}

int editor::get_tile_zorder(const std::string& tile_id) const
{
	foreach(const editor::tileset& tile, tilesets) {
		if(tile.type == tile_id) {
			return tile.zorder;
		}
	}

	return 0;
}

void editor::add_tile_rect(int zorder, const std::string& tile_id, int x1, int y1, int x2, int y2)
{
	if(x2 < x1) {
		std::swap(x1, x2);
	}

	if(y2 < y1) {
		std::swap(y1, y2);
	}

	std::vector<std::string> old_rect;
	lvl_->get_tile_rect(zorder, x1, y1, x2, y2, old_rect);

	std::vector<boost::function<void()> > undo, redo;

	redo.push_back(boost::bind(&level::add_tile_rect, lvl_.get(), zorder, x1, y1, x2, y2, tile_id));
	undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), zorder, x1, y1, x2, y2, old_rect));

	redo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), x1, y1, x2, y2));
	undo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), x1, y1, x2, y2));

	std::vector<int> layers;
	layers.push_back(zorder);
	undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), layers));
	redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), layers));

	execute_command(
	  boost::bind(execute_functions, redo),
	  boost::bind(execute_functions, undo));

	if(layers_dialog_) {
		layers_dialog_->init();
	}
}

void editor::add_tile_rect(int x1, int y1, int x2, int y2)
{
	x1 += ((100 - tilesets[cur_tileset_].x_speed)*xpos_)/100;
	x2 += ((100 - tilesets[cur_tileset_].x_speed)*xpos_)/100;
	y1 += ((100 - tilesets[cur_tileset_].y_speed)*ypos_)/100;
	y2 += ((100 - tilesets[cur_tileset_].y_speed)*ypos_)/100;

	add_tile_rect(tilesets[cur_tileset_].zorder, tilesets[cur_tileset_].type, x1, y1, x2, y2);
	lvl_->set_tile_layer_speed(tilesets[cur_tileset_].zorder,
	                           tilesets[cur_tileset_].x_speed,
							   tilesets[cur_tileset_].y_speed);
}

void editor::remove_tile_rect(int x1, int y1, int x2, int y2)
{
	if(x2 < x1) {
		std::swap(x1, x2);
	}

	if(y2 < y1) {
		std::swap(y1, y2);
	}

	std::map<int, std::vector<std::string> > old_tiles;
	lvl_->get_all_tiles_rect(x1, y1, x2, y2, old_tiles);
	std::vector<boost::function<void()> > redo, undo;
	for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
		undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl_.get(), i->first, x1, y1, x2, y2, i->second));
	}

	redo.push_back(boost::bind(&level::clear_tile_rect, lvl_.get(), x1, y1, x2, y2));
	redo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), x1, y1, x2, y2));
	undo.push_back(boost::bind(&level::refresh_tile_rect, lvl_.get(), x1, y1, x2, y2));
	undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));
	redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl_.get(), std::vector<int>()));

	execute_command(
	  boost::bind(execute_functions, redo),
	  boost::bind(execute_functions, undo));
}

void editor::select_tile_rect(int x1, int y1, int x2, int y2)
{
	tile_selection new_selection;

	const bool shift_pressed = (SDL_GetModState()&KMOD_SHIFT) != 0;
	if(shift_pressed) {
		//adding to the selection
		new_selection = tile_selection_;
	}

	if(x2 < x1) {
		std::swap(x1, x2);
	}

	if(y2 < y1) {
		std::swap(y1, y2);
	}

	if(x2 - x1 > TileSize/4 || y2 - y1 > TileSize/4) {
		x2 += TileSize;
		y2 += TileSize;

		x1 = round_tile_size(x1)/TileSize;
		y1 = round_tile_size(y1)/TileSize;
		x2 = round_tile_size(x2)/TileSize;
		y2 = round_tile_size(y2)/TileSize;

		for(int x = x1; x != x2; ++x) {
			for(int y = y1; y != y2; ++y) {
				const point p(x, y);
				new_selection.tiles.push_back(p);
			}
		}

		std::sort(new_selection.tiles.begin(), new_selection.tiles.end());

		const bool alt_pressed = (SDL_GetModState()&(KMOD_LALT|KMOD_RALT)) != 0;
		if(alt_pressed) {
			//diff from selection
			tile_selection diff;
			foreach(const point& p, tile_selection_.tiles) {
				if(std::binary_search(new_selection.tiles.begin(), new_selection.tiles.end(), p) == false) {
					diff.tiles.push_back(p);
				}
			}

			new_selection.tiles.swap(diff.tiles);
		}
	}

	execute_command(
	  boost::bind(&editor::set_selection, this, new_selection),	
	  boost::bind(&editor::set_selection, this, tile_selection_));
}

void editor::select_magic_wand(int xpos, int ypos)
{
	tile_selection new_selection;

	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	if(ctrl_pressed) {
		//adding to the selection
		new_selection = tile_selection_;
	}

	std::vector<point> tiles = lvl_->get_solid_contiguous_region(xpos, ypos);
	new_selection.tiles.insert(new_selection.tiles.end(), tiles.begin(), tiles.end());
	execute_command(
	  boost::bind(&editor::set_selection, this, new_selection),	
	  boost::bind(&editor::set_selection, this, tile_selection_));
}

void editor::set_selection(const tile_selection& s)
{
	tile_selection_ = s;
}

void editor::move_object(entity_ptr e, int delta_x, int delta_y)
{
	e->set_pos(e->x() + delta_x, e->y() + delta_y);

	//update any x/y co-ordinates to be the same relative to the object's
	//new position.
	if(e->editor_info()) {
		foreach(const editor_variable_info& var, e->editor_info()->vars()) {
			const variant value = e->query_value(var.variable_name());
			switch(var.type()) {
			case editor_variable_info::XPOSITION:
				if(value.is_int()) {
					e->mutate_value(var.variable_name(), variant(value.as_int() + delta_x));
				}
				break;
			case editor_variable_info::YPOSITION:
				if(value.is_int()) {
					e->mutate_value(var.variable_name(), variant(value.as_int() + delta_y));
				}
				break;
			default:
				break;
			}
		}
	}
}

const std::vector<editor::tileset>& editor::all_tilesets() const
{
	return tilesets;
}

const std::vector<editor::enemy_type>& editor::all_characters() const
{
	return enemy_types;
}

void editor::set_tileset(int index)
{
	cur_tileset_ = index;
	if(cur_tileset_ < 0) {
		cur_tileset_ = tilesets.size()-1;
	} else if(cur_tileset_ >= tilesets.size()) {
		cur_tileset_ = 0;
	}

	lvl_->set_tile_layer_speed(tilesets[cur_tileset_].zorder,
	                           tilesets[cur_tileset_].x_speed,
							   tilesets[cur_tileset_].y_speed);
}

void editor::set_object(int index)
{
	int max = enemy_types.size();

	if(index < 0) {
		index = max - 1;
	} else if(index >= max) {
		index = 0;
	}

	cur_object_ = index;
}

editor::EDIT_TOOL editor::tool() const
{
	const bool alt_pressed = (SDL_GetModState()&KMOD_ALT) != 0;
	if(alt_pressed) {
		switch(tool_) {
		case TOOL_ADD_RECT:
		case TOOL_SELECT_RECT:
		case TOOL_MAGIC_WAND:
		case TOOL_PENCIL:
		case TOOL_PICKER:
			return TOOL_PICKER;
		default:
			break;
		}
	}

	return tool_;
}

void editor::change_tool(EDIT_TOOL tool)
{
	tool_ = tool;

	switch(tool_) {
	case TOOL_ADD_RECT:
	case TOOL_SELECT_RECT:
	case TOOL_MAGIC_WAND:
	case TOOL_PENCIL:
	case TOOL_PICKER: {
		if(!tileset_dialog_) {
			tileset_dialog_.reset(new editor_dialogs::tileset_editor_dialog(*this));
		}
		current_dialog_ = tileset_dialog_.get();
		lvl_->editor_clear_selection();
		break;
	}
	case TOOL_ADD_OBJECT: {
		if(!character_dialog_) {
			character_dialog_.reset(new editor_dialogs::character_editor_dialog(*this));
		}
		current_dialog_ = character_dialog_.get();
		character_dialog_->set_character(cur_object_);
		break;
	}
	case TOOL_SELECT_OBJECT: {
		current_dialog_ = property_dialog_.get();
		break;
	}
	}

	if(editor_mode_dialog_) {
		editor_mode_dialog_->init();
	}
}

void editor::save_level_as(const std::string& fname)
{
	all_editors.erase(filename_);
	all_editors[fname] = this;
	filename_ = fname;
	save_level();
	g_last_edited_level() = filename_;
}

void editor::quit()
{
	if(confirm_quit()) {
		done_ = true;
	}
}

namespace {
void quit_editor_result(gui::dialog* d, int* result_ptr, int result) {
	d->close();
	*result_ptr = result;
}
}

bool editor::confirm_quit()
{
	if(!level_changed_) {
		return true;
	}

	const int center_x = graphics::screen_width()/2;
	const int center_y = graphics::screen_height()/2;
	using namespace gui;
	dialog d(center_x - 140, center_y - 100, center_x + 140, center_y + 100);

	d.add_widget(widget_ptr(new label("Do you want to save the level?", graphics::color_white())), dialog::MOVE_DOWN);

	gui::grid* grid = new gui::grid(3);

	int result = 0;
	grid->add_col(widget_ptr(
	  new button(widget_ptr(new label("Yes", graphics::color_white())),
	             boost::bind(quit_editor_result, &d, &result, 0))));
	grid->add_col(widget_ptr(
	  new button(widget_ptr(new label("No", graphics::color_white())),
	             boost::bind(quit_editor_result, &d, &result, 1))));
	grid->add_col(widget_ptr(
	  new button(widget_ptr(new label("Cancel", graphics::color_white())),
	             boost::bind(quit_editor_result, &d, &result, 2))));
	d.add_widget(widget_ptr(grid));
	d.show_modal();

	if(result == 2) {
		return false;
	}

	if(result == 0 && !d.cancelled()) {
		save_level();
	}

	return true;
}

void editor::save_level()
{
	level_changed_ = 0;

	remove_ghost_objects();
	ghost_objects_.clear();

	const std::string path = "data/level/";
	std::string data;
	wml::write(lvl_->write(), data);
	sys::write_file(path + filename_, data);

	//see if we should write the next/previous levels also
	//based on them having changed.
	if(lvl_->previous_level().empty() == false) {
		try {
			level prev(lvl_->previous_level());
			if(prev.next_level() != lvl_->id()) {
				prev.set_next_level(lvl_->id());
				std::string data;
				wml::write(prev.write(), data);
				sys::write_file(path + prev.id(), data);
			}
		} catch(...) {
		}
	}

	if(lvl_->next_level().empty() == false) {
		try {
			level next(lvl_->next_level());
			if(next.previous_level() != lvl_->id()) {
				next.set_previous_level(lvl_->id());
				std::string data;
				wml::write(next.write(), data);
				sys::write_file(path + next.id(), data);
			}
		} catch(...) {
		}
	}
}

void editor::zoom_in()
{
	if(zoom_ > 1) {
		zoom_ /= 2;
	}
}

void editor::zoom_out()
{
	if(zoom_ < 8) {
		zoom_ *= 2;
	}
}

void editor::draw() const
{
	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;

	int mousex, mousey;
	get_mouse_state(mousex, mousey);

	graphics::prepare_raster();
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	glScalef(1.0/zoom_, 1.0/zoom_, 0);
	glTranslatef(-xpos_,-ypos_,0);

	if(zoom_ == 1) {
		//backgrounds only draw nicely at the regular zoom level for now.
		lvl_->draw_background(xpos_, ypos_, 0);
	}

	lvl_->draw(xpos_, ypos_, graphics::screen_width()*zoom_, graphics::screen_height()*zoom_);

	const int selectx = xpos_ + mousex*zoom_;
	const int selecty = ypos_ + mousey*zoom_;

	{
	std::string next_level = "To " + lvl_->next_level();
	std::string previous_level = "To " + lvl_->previous_level();
	if(lvl_->next_level().empty()) {
		next_level = "(no next level)";
	}
	if(lvl_->previous_level().empty()) {
		previous_level = "(no previous level)";
	}
	graphics::texture t = font::render_text(previous_level, graphics::color_black(), 24);
	int x = lvl_->boundaries().x() - t.width();
	int y = ypos_ + graphics::screen_height()/2;

	graphics::blit_texture(t, x, y);
	t = font::render_text(next_level, graphics::color_black(), 24);
	x = lvl_->boundaries().x2();
	graphics::blit_texture(t, x, y);
	}

	if(tool() == TOOL_ADD_OBJECT && !lvl_->editor_highlight()) {
		int x = round_tile_size(xpos_ + mousex*zoom_);
		int y = round_tile_size(ypos_ + mousey*zoom_);
		if(ctrl_pressed) {
			x = xpos_ + mousex*zoom_;
			y = ypos_ + mousey*zoom_;
		}

		entity& e = *all_characters()[cur_object_].preview_object;
		e.set_pos(x, y);
		if(place_entity_in_level(*lvl_, e)) {
			glColor4f(1.0, 1.0, 1.0, 0.5);
			all_characters()[cur_object_].preview_frame->draw(e.x(), e.y(), face_right_);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}

	if(drawing_rect_) {
		int x1 = anchorx_;
		int x2 = xpos_ + mousex*zoom_;
		if(x1 > x2) {
			std::swap(x1,x2);
		}

		int y1 = anchory_;
		int y2 = ypos_ + mousey*zoom_;
		if(y1 > y2) {
			std::swap(y1,y2);
		}

		const SDL_Rect rect = {x1, y1, x2 - x1, y2 - y1};
		const SDL_Color color = {255,255,255,255};
		graphics::draw_hollow_rect(rect, color);
	}
	
	std::vector<GLfloat>& varray = graphics::global_vertex_array();
	if(property_dialog_ && property_dialog_.get() == current_dialog_ && property_dialog_->get_entity() && property_dialog_->get_entity()->editor_info()) {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		const editor_variable_info* selected_var = variable_info_selected(property_dialog_->get_entity(), xpos_ + mousex*zoom_, ypos_ + mousey*zoom_, zoom_);
		foreach(const editor_variable_info& var, property_dialog_->get_entity()->editor_info()->vars()) {
			const std::string& name = var.variable_name();
			const editor_variable_info::VARIABLE_TYPE type = var.type();
			variant value = property_dialog_->get_entity()->query_value(name);
			if(&var == selected_var) {
				glColor4ub(255, 255, 0, 255);
			} else {
				glColor4ub(255, 0, 0, 255);
			}

			varray.clear();
			switch(type) {
				case editor_variable_info::XPOSITION:
					if(value.is_int()) {
						varray.push_back(value.as_int()); varray.push_back(ypos_);
						varray.push_back(value.as_int()); varray.push_back(ypos_ + graphics::screen_height()*zoom_);
					}
					break;
				case editor_variable_info::YPOSITION:
					if(value.is_int()) {
						varray.push_back(xpos_); varray.push_back(value.as_int());
						varray.push_back(xpos_ + graphics::screen_width()*zoom_); varray.push_back(value.as_int());
					}
					break;
				default:
					break;
			}
			glVertexPointer(2, GL_FLOAT, 0, &varray.front());
			glDrawArrays(GL_LINES, 0, varray.size()/2);
		}
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
	}

	if(g_draw_stats) {
		foreach(const stats::const_record_ptr& record, stats_) {
			record->draw();
		}
	}

	glPopMatrix();

	//draw grid
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	varray.clear();
	glColor4ub(255, 255, 255, 64);
	for(int x = TileSize - (xpos_/zoom_)%TileSize; x < graphics::screen_width(); x += 32/zoom_) {
		varray.push_back(x); varray.push_back(0);
		varray.push_back(x); varray.push_back(graphics::screen_height());
	}

	for(int y = TileSize - (ypos_/zoom_)%TileSize; y < graphics::screen_height(); y += 32/zoom_) {
		varray.push_back(0); varray.push_back(y);
		varray.push_back(graphics::screen_width()); varray.push_back(y);
	}
	glVertexPointer(2, GL_FLOAT, 0, &varray.front());
	glDrawArrays(GL_LINES, 0, varray.size()/2);
	
	// draw level boundaries in clear white
	{
		varray.clear();
		std::vector<GLfloat>& carray = graphics::global_texcoords_array(); //reusing texcoords array for colors
		carray.clear();
		rect boundaries = modify_selected_rect(lvl_->boundaries(), selectx, selecty);
		const int x1 = boundaries.x()/zoom_;
		const int x2 = boundaries.x2()/zoom_;
		const int y1 = boundaries.y()/zoom_;
		const int y2 = boundaries.y2()/zoom_;
		
		graphics::color selected_color(255, 255, 0, 255);
		graphics::color normal_color(255, 255, 255, 255);

		if(resizing_top_level_edge || rect_top_edge_selected(lvl_->boundaries(), selectx, selecty, zoom_)) {
			selected_color.add_to_vector(&carray);
			selected_color.add_to_vector(&carray);
		} else {
			normal_color.add_to_vector(&carray);
			normal_color.add_to_vector(&carray);
		}
		
		varray.push_back(x1 - xpos_/zoom_); varray.push_back(y1 - ypos_/zoom_);
		varray.push_back(x2 - xpos_/zoom_); varray.push_back(y1 - ypos_/zoom_);

		if(resizing_left_level_edge || rect_left_edge_selected(lvl_->boundaries(), selectx, selecty, zoom_)) {
			selected_color.add_to_vector(&carray);
			selected_color.add_to_vector(&carray);
		} else {
			normal_color.add_to_vector(&carray);
			normal_color.add_to_vector(&carray);
		}

		varray.push_back(x1 - xpos_/zoom_); varray.push_back(y1 - ypos_/zoom_);
		varray.push_back(x1 - xpos_/zoom_); varray.push_back(y2 - ypos_/zoom_);

		if(resizing_right_level_edge || rect_right_edge_selected(lvl_->boundaries(), selectx, selecty, zoom_)) {
			selected_color.add_to_vector(&carray);
			selected_color.add_to_vector(&carray);
		} else {
			normal_color.add_to_vector(&carray);
			normal_color.add_to_vector(&carray);
		}
		
		varray.push_back(x2 - xpos_/zoom_); varray.push_back(y1 - ypos_/zoom_);
		varray.push_back(x2 - xpos_/zoom_); varray.push_back(y2 - ypos_/zoom_);

		if(resizing_bottom_level_edge || rect_bottom_edge_selected(lvl_->boundaries(), selectx, selecty, zoom_)) {
			selected_color.add_to_vector(&carray);
			selected_color.add_to_vector(&carray);
		} else {
			normal_color.add_to_vector(&carray);
			normal_color.add_to_vector(&carray);
		}
		
		varray.push_back(x1 - xpos_/zoom_); varray.push_back(y2 - ypos_/zoom_);
		varray.push_back(x2 - xpos_/zoom_); varray.push_back(y2 - ypos_/zoom_);
		
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glColorPointer(4, GL_FLOAT, 0, &carray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	draw_selection(0, 0);
	
	if(dragging_) {
		int diffx = (selectx - anchorx_)/TileSize;
		int diffy = (selecty - anchory_)/TileSize;

		if(diffx != 0 || diffy != 0) {
			std::cerr << "DRAW DIFF: " << diffx << "," << diffy << "\n";
			draw_selection(diffx*TileSize, diffy*TileSize);
		}
	}
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);

	ASSERT_INDEX_INTO_VECTOR(cur_object_, enemy_types);

	//the location of the mouse cursor in the map
	char loc_buf[256];
	sprintf(loc_buf, "%d,%d", xpos_ + mousex*zoom_, ypos_ + mousey*zoom_);
	graphics::blit_texture(font::render_text(loc_buf, graphics::color_yellow(), 14), 10, 60);

	if(current_dialog_) {
		current_dialog_->draw();
	}

	if(layers_dialog_) {
		layers_dialog_->draw();
	}

	editor_menu_dialog_->draw();
	editor_mode_dialog_->draw();
	gui::draw_tooltip();

	debug_console::draw();

	SDL_GL_SwapBuffers();
	SDL_Delay(20);
}

void editor::draw_selection(int xoffset, int yoffset) const
{
	if(tile_selection_.empty()) {
		return;
	}

	const int ticks = (SDL_GetTicks()/40)%16;
	uint32_t stipple_bits = 0xFF;
	stipple_bits <<= ticks;
	const uint16_t stipple_mask = (stipple_bits&0xFFFF) | ((stipple_bits&0xFFFF0000) >> 16);
	
	glColor4ub(255, 255, 255, 255);
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1, stipple_mask);
	std::vector<GLfloat>& varray = graphics::global_vertex_array();
	varray.clear();
	foreach(const point& p, tile_selection_.tiles) {
		const int size = TileSize/zoom_;
		const int xpos = xoffset/zoom_ + p.x*size - xpos_/zoom_;
		const int ypos = yoffset/zoom_ + p.y*size - ypos_/zoom_;

		if(std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(p.x, p.y - 1)) == false) {
			varray.push_back(xpos); varray.push_back(ypos);
			varray.push_back(xpos + size); varray.push_back(ypos);
		}

		if(std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(p.x, p.y + 1)) == false) {
			varray.push_back(xpos + size); varray.push_back(ypos + size);
			varray.push_back(xpos); varray.push_back(ypos + size);
		}

		if(std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(p.x - 1, p.y)) == false) {
			varray.push_back(xpos); varray.push_back(ypos + size);
			varray.push_back(xpos); varray.push_back(ypos);
		}

		if(std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(p.x + 1, p.y)) == false) {
			varray.push_back(xpos + size); varray.push_back(ypos);
			varray.push_back(xpos + size); varray.push_back(ypos + size);
		}
	}
	glVertexPointer(2, GL_FLOAT, 0, &varray.front());
	glDrawArrays(GL_LINES, 0, varray.size()/2);
	glDisable(GL_LINE_STIPPLE);
	glLineStipple(1, 0xFFFF);
}

void editor::run_script(const std::string& id)
{
	editor_script::execute(id, *this);
}

void editor::execute_command(boost::function<void()> command, boost::function<void()> undo)
{
	level_changed_++;

	command();

	executable_command cmd;
	cmd.redo_command = command;
	cmd.undo_command = undo;
	undo_.push_back(cmd);
	redo_.clear();
}

void editor::begin_command_group()
{
	undo_commands_groups_.push(undo_.size());

	lvl_->editor_freeze_tile_updates(true);
}

void editor::end_command_group()
{
	lvl_->editor_freeze_tile_updates(false);

	ASSERT_NE(undo_commands_groups_.empty(), true);

	const int index = undo_commands_groups_.top();
	undo_commands_groups_.pop();

	if(index >= undo_.size()) {
		return;
	}

	//group all of the commands since beginning into one command
	std::vector<boost::function<void()> > undo, redo;
	for(int n = index; n != undo_.size(); ++n) {
		undo.push_back(undo_[n].undo_command);
		redo.push_back(undo_[n].redo_command);
	}

	//reverse the undos, since we want them executed in reverse order.
	std::reverse(undo.begin(), undo.end());

	//make it so undoing and redoing will freeze tile updates during the
	//group command, and then do a full refresh of tiles once we're done.
	undo.insert(undo.begin(), boost::bind(&level::editor_freeze_tile_updates, lvl_.get(), true));
	undo.push_back(boost::bind(&level::editor_freeze_tile_updates, lvl_.get(), false));
	redo.insert(redo.begin(), boost::bind(&level::editor_freeze_tile_updates, lvl_.get(), true));
	redo.push_back(boost::bind(&level::editor_freeze_tile_updates, lvl_.get(), false));

	executable_command cmd;
	cmd.redo_command = boost::bind(execute_functions, redo);
	cmd.undo_command = boost::bind(execute_functions, undo);

	//replace all the individual commands with the one group command.
	undo_.erase(undo_.begin() + index, undo_.end());
	undo_.push_back(cmd);
}

void editor::undo_command()
{
	if(undo_.empty()) {
		return;
	}

	--level_changed_;

	undo_.back().undo_command();
	redo_.push_back(undo_.back());
	undo_.pop_back();

	if(layers_dialog_) {
		layers_dialog_->init();
	}
}

void editor::redo_command()
{
	if(redo_.empty()) {
		return;
	}

	++level_changed_;

	redo_.back().redo_command();
	undo_.push_back(redo_.back());
	redo_.pop_back();

	if(layers_dialog_) {
		layers_dialog_->init();
	}
}

void editor::edit_level_properties()
{
	editor_dialogs::editor_level_properties_dialog prop_dialog(*this);
	prop_dialog.show_modal();
}
