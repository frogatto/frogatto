#ifndef NO_EDITOR
#include "graphics.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include "asserts.hpp"
#include "border_widget.hpp"
#include "button.hpp"
#include "character_editor_dialog.hpp"
#include "code_editor_dialog.hpp"
#include "collision_utils.hpp"
#include "color_utils.hpp"
#include "custom_object_dialog.hpp"
#include "draw_scene.hpp"
#include "draw_tile.hpp"
#include "debug_console.hpp"
#include "editor.hpp"
#include "editor_dialogs.hpp"
#include "editor_formula_functions.hpp"
#include "editor_layers_dialog.hpp"
#include "editor_level_properties_dialog.hpp"
#include "editor_module_properties_dialog.hpp"
#include "editor_stats_dialog.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "frame.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "json_parser.hpp"
#include "key.hpp"
#include "label.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "module.hpp"
#include "multiplayer.hpp"
#include "object_events.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "property_editor_dialog.hpp"
#include "raster.hpp"
#include "segment_editor_dialog.hpp"
#include "stats.hpp"
#include "texture.hpp"
#include "text_editor_widget.hpp"
#include "tile_map.hpp"
#include "tileset_editor_dialog.hpp"
#include "tooltip.hpp"
#include "variant.hpp"
#include "variant_utils.hpp"

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

bool g_draw_grid = true;

void toggle_draw_grid() {
	g_draw_grid = !g_draw_grid;
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
			"Create New Module...", "", boost::bind(&editor::create_new_module, &editor_),
			"Edit Module Properties...", "", boost::bind(&editor::edit_module_properties, &editor_),
			"Create New Object...", "", boost::bind(&editor::create_new_object, &editor_),
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
			"Restart Level", "ctrl+r", boost::bind(&editor::reset_playing_level, &editor_, true),
			"Restart Level (including player)", "ctrl+alt+r", boost::bind(&editor::reset_playing_level, &editor_, false),
			"Pause Game", "ctrl+p", boost::bind(&editor::toggle_pause, &editor_),
			"Code", "", boost::bind(&editor::toggle_code, &editor_),
		};

		menu_item duplicate_item = { "Duplicate Object(s)", "ctrl+1", boost::bind(&editor::duplicate_selected_objects, &editor_) };

		std::vector<menu_item> res;
		foreach(const menu_item& m, items) {
			res.push_back(m);
		}

		if(editor_.get_level().editor_selection().empty() == false) {
			res.push_back(duplicate_item);
		}

		show_menu(res);
	}

	void show_view_menu() {
		menu_item items[] = {
			"Zoom Out", "x", boost::bind(&editor::zoom_out, &editor_),
			"Zoom In", "z", boost::bind(&editor::zoom_in, &editor_),
			editor_.get_level().show_foreground() ? "Hide Foreground" : "Show Foreground", "f", boost::bind(&level::set_show_foreground, &editor_.get_level(), !editor_.get_level().show_foreground()),
			editor_.get_level().show_background() ? "Hide Background" : "Show Background", "b", boost::bind(&level::set_show_background, &editor_.get_level(), !editor_.get_level().show_background()),
			g_draw_stats ? "Hide Stats" : "Show Stats", "", toggle_draw_stats,
			g_draw_grid ? "Hide Grid" : "Show Grid", "", toggle_draw_grid,
			preferences::show_debug_hitboxes() ? "Hide Hit Boxes" : "Show Hit Boxes", "h", preferences::toogle_debug_hitboxes,
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
	gui::button_ptr code_button_;
	std::string code_button_text_;
public:
	explicit editor_menu_dialog(editor& e)
	  : gui::dialog(0, 0, e.xres() ? e.xres() : 1200, EDITOR_MENUBAR_HEIGHT), editor_(e)
	{
		set_clear_bg_amount(255);
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

		code_button_text_ = "";
		set_code_button_text("Code ->");
	}

	void set_code_button_text(const std::string& text)
	{
		using namespace gui;

		if(code_button_text_ == text) {
			return;
		}

		code_button_text_ = text;

		if(code_button_) {
			remove_widget(code_button_);
		}

		if(text.empty()) {
			return;
		}

		code_button_ = button_ptr(new button(text, boost::bind(&editor::toggle_code, &editor_)));

		add_widget(code_button_, (editor_.xres() ? editor_.xres() : 1200) - 612, 4);
	}


	void new_level() {
		using namespace gui;
		dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
		d.add_widget(widget_ptr(new label("New Level", graphics::color_white(), 48)));
		text_editor_widget* entry = new text_editor_widget(200);
		entry->set_on_enter_handler(boost::bind(&dialog::close, &d));
		d.add_widget(widget_ptr(new label("Filename:", graphics::color_white())))
		 .add_widget(widget_ptr(entry));
		d.show_modal();
		
		std::string name = entry->text();
		if(name.empty() == false) {
			variant empty_lvl = json::parse_from_file("data/level/empty.cfg");
			std::string id = module::make_module_id(name);
			empty_lvl.add_attr(variant("id"), variant(module::get_id(id)));
			if(preferences::is_level_path_set()) {
				sys::write_file(preferences::level_path() + name, empty_lvl.write_json());
			} else {
				std::string nn = module::get_id(name);
				std::string modname = module::get_module_id(name);
				sys::write_file(module::get_module_path(modname) + preferences::level_path() + nn, empty_lvl.write_json());
			}
			loadlevel::load_level_paths();
			editor_.close();
			g_last_edited_level() = id;
		}
	}

	void save_level_as() {
		using namespace gui;
		dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
		d.add_widget(widget_ptr(new label("Save As", graphics::color_white(), 48)));
		text_editor_widget* entry = new text_editor_widget(200);
		entry->set_on_enter_handler(boost::bind(&dialog::close, &d));
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
  "Edit Level Segments",
};

const char* ToolIcons[] = {"editor_draw_rect", "editor_rect_select", "editor_wand", "editor_pencil", "editor_eyedropper", "editor_add_object", "editor_select_object", "editor_rect_select", };
}

class editor_mode_dialog : public gui::dialog
{
	editor& editor_;
	gui::widget_ptr context_menu_;

	std::vector<gui::border_widget*> tool_borders_;

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
	  : gui::dialog(graphics::screen_width() - EDITOR_SIDEBAR_WIDTH, 0, EDITOR_SIDEBAR_WIDTH, 160), editor_(e)
	{
		set_clear_bg_amount(255);
		init();
	}

	void init()
	{
		using namespace gui;
		clear();

		tool_borders_.clear();

		grid_ptr grid(new gui::grid(3));
		for(int n = 0; n < editor::NUM_TOOLS; ++n) {
			if(n == editor::TOOL_EDIT_SEGMENTS && editor_.get_level().segment_width() == 0 && editor_.get_level().segment_height() == 0) {
				continue;
			}
			std::cerr << "editor add tool: " << n << std::endl;
			button_ptr tool_button(
			  new button(widget_ptr(new gui_section_widget(ToolIcons[n], 26, 26)),
			             boost::bind(&editor_mode_dialog::select_tool, this, n)));
			tool_button->set_tooltip(ToolStrings[n]);
			tool_borders_.push_back(new border_widget(tool_button, graphics::color(0,0,0,0)));
			grid->add_col(widget_ptr(tool_borders_.back()));
		}

		grid->finish_row();
		add_widget(grid, 5, 5);

		refresh_selection();
	}

	void refresh_selection() {
		using namespace gui;
		for(int n = 0; n != tool_borders_.size(); ++n) {
			tool_borders_[n]->set_color(n == editor_.tool() ? graphics::color(255,255,255,255) : graphics::color(0,0,0,0));
		}
	}
};

namespace {

const int RectEdgeSelectThreshold = 6;

void execute_functions(const std::vector<boost::function<void()> >& v) {
	foreach(const boost::function<void()>& f, v) {
		f();
	}
}

bool g_started_dragging_object = false;

//the current state of the rectangle we're dragging
rect g_rect_drawing;

//the tiles that we've drawn in the current action.
std::vector<point> g_current_draw_tiles;

const editor_variable_info* g_variable_editing = NULL;
int g_variable_editing_index = -1;
variant g_variable_editing_original_value;
const editor_variable_info* variable_info_selected(const_entity_ptr e, int xpos, int ypos, int zoom, int* index_selected=NULL)
{
	if(index_selected) {
		*index_selected = -1;
	}

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
			case editor_variable_info::TYPE_POINTS: {
				if(!value.is_list()) {
					break;
				}

				int index = 0;
				foreach(variant p, value.as_list()) {
					point pt(p);
					if(point_in_rect(point(xpos, ypos), rect(pt.x-10, pt.y-10, 20, 20))) {
						if(index_selected) {
							*index_selected = index;
						}
						return &var;
					}

					++index;
				}
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

rect modify_selected_rect(const editor& e, rect boundaries, int xpos, int ypos) {

	const int x = round_tile_size(xpos);
	const int y = round_tile_size(ypos);

	if(resizing_left_level_edge) {
		boundaries = rect(x, boundaries.y(), boundaries.w() + (boundaries.x() - x), boundaries.h());
		if(e.get_level().segment_width() > 0) {
			while(boundaries.w()%e.get_level().segment_width() != 0) {
				boundaries = rect(boundaries.x()-1, boundaries.y(), boundaries.w()+1, boundaries.h());
			}
		}
	}

	if(resizing_right_level_edge) {
		boundaries = rect(boundaries.x(), boundaries.y(), x - boundaries.x(), boundaries.h());
		if(e.get_level().segment_width() > 0) {
			while(boundaries.w()%e.get_level().segment_width() != 0) {
				boundaries = rect(boundaries.x(), boundaries.y(), boundaries.w()+1, boundaries.h());
			}
		}
	}

	if(resizing_top_level_edge) {
		boundaries = rect(boundaries.x(), y, boundaries.w(), boundaries.h() + (boundaries.y() - y));
		if(e.get_level().segment_height() > 0) {
			while(boundaries.h()%e.get_level().segment_height() != 0) {
				boundaries = rect(boundaries.x(), boundaries.y()-1, boundaries.w(), boundaries.h()+1);
			}
		}
	}

	if(resizing_bottom_level_edge) {
		boundaries = rect(boundaries.x(), boundaries.y(), boundaries.w(), y - boundaries.y());
		if(e.get_level().segment_height() > 0) {
			while(boundaries.h()%e.get_level().segment_height() != 0) {
				boundaries = rect(boundaries.x(), boundaries.y(), boundaries.w(), boundaries.h()+1);
			}
		}
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

editor::manager::~manager() {
	enemy_types.clear();
}

void editor::enemy_type::init(variant node)
{
	enemy_types.clear();
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
	variant_builder new_node;
	new_node.add("type", type.id());
	new_node.add("custom", true);
	new_node.add("face_right", false);
	new_node.add("x", 1500);
	new_node.add("y", 0);

	if(type.is_human()) {
		new_node.add("is_human", true);
	}

	node = new_node.build();
	preview_object = entity::build(node);
	preview_frame.reset(new frame(preview_object->current_frame()));
}

void editor::tileset::init(variant node)
{
	foreach(variant tileset_node, node["tileset"].as_list()) {
		tilesets.push_back(editor::tileset(tileset_node));
	}
}

editor::tileset::tileset(variant node)
  : category(node["category"].as_string()), type(node["type"].as_string()),
    zorder(node["zorder"].as_int()),
	x_speed(node["x_speed"].as_int(100)),
	y_speed(node["y_speed"].as_int(100)),
	sloped(node["sloped"].as_bool()),
	node_info(node)
{
	if(node.has_key("preview")) {
		preview.reset(new tile_map(node["preview"]));
	}
}

editor* editor::get_editor(const char* level_cfg)
{
	editor*& e = all_editors[level_cfg];
	if(!e) {
		e = new editor(level_cfg);
	}
	e->done_ = false;
	return e;
}

void editor::edit(const char* level_cfg, int xpos, int ypos)
{
	editor* e = get_editor(level_cfg);

	if(xpos != -1) {
		e->xpos_ = xpos;
		e->ypos_ = ypos;
	}

	editor_resolution_manager resolution_manager(e->xres(), e->yres());

	e->setup_for_editing();
	e->edit_level();
	if(g_last_edited_level() != level_cfg) {
		//a new level was set, so start editing it now.
		edit(g_last_edited_level().c_str());
	}

	clear_level_wml();
}

std::string editor::last_edited_level()
{
	return g_last_edited_level();
}

namespace {
int g_codebar_width = 0;
}

int editor::sidebar_width()
{
	return g_codebar_width == 0 ? 180 : g_codebar_width;
}

int editor::codebar_height()
{
	return 0; //g_codebar_height;
}

editor::editor(const char* level_cfg)
  : zoom_(1), xpos_(0), ypos_(0), anchorx_(0), anchory_(0),
    selected_entity_startx_(0), selected_entity_starty_(0),
    filename_(level_cfg), tool_(TOOL_ADD_RECT),
    done_(false), face_right_(true), upside_down_(false),
	cur_tileset_(0), cur_object_(0),
    current_dialog_(NULL),
	drawing_rect_(false), dragging_(false), level_changed_(0),
	selected_segment_(-1), prev_mousex_(-1), prev_mousey_(-1),
	xres_(0), yres_(0)
{
	preferences::set_record_history(true);

	static bool first_time = true;
	if(first_time) {
		variant editor_cfg = json::parse_from_file("data/editor.cfg");
		tile_map::load_all();
		tileset::init(editor_cfg);
		enemy_type::init(editor_cfg);
		first_time = false;
		if(editor_cfg.is_map()) {
			if(editor_cfg["resolution"].is_null() == false) {
				std::vector<int> v = editor_cfg["resolution"].as_list_int();
				xres_ = v[0];
				yres_ = v[1];
			}
		}
	}

	assert(!tilesets.empty());
	lvl_.reset(new level(level_cfg));
	lvl_->set_editor();
	lvl_->finish_loading();
	lvl_->set_as_current_level();

	levels_.push_back(lvl_);

	editor_menu_dialog_.reset(new editor_menu_dialog(*this));
	editor_mode_dialog_.reset(new editor_mode_dialog(*this));

	property_dialog_.reset(new editor_dialogs::property_editor_dialog(*this));

	if(preferences::external_code_editor().is_null() == false && !external_code_editor_) {
		external_code_editor_ = external_text_editor::create(preferences::external_code_editor());
	}
}

editor::~editor()
{
}

void editor::group_selection()
{
	std::vector<boost::function<void()> > undo, redo;

	foreach(level_ptr lvl, levels_) {
		const int group = lvl->add_group();
		foreach(const entity_ptr& e, lvl_->editor_selection()) {
			entity_ptr c = lvl->get_entity_by_label(e->label());
			if(!c) {
				continue;
			}

			undo.push_back(boost::bind(&level::set_character_group, lvl.get(), c, c->group()));
			redo.push_back(boost::bind(&level::set_character_group, lvl.get(), c, group));
		}
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

void editor::toggle_upside_down()
{
	upside_down_ = !upside_down_;
	if(character_dialog_) {
		character_dialog_->init();
	}
}

void editor::duplicate_selected_objects()
{
	std::vector<boost::function<void()> > redo, undo;
	foreach(const entity_ptr& c, lvl_->editor_selection()) {
		entity_ptr duplicate_obj = c->clone();

		foreach(level_ptr lvl, levels_) {
			entity_ptr obj = duplicate_obj->backup();
			if(!place_entity_in_level_with_large_displacement(*lvl, *obj)) {
				continue;
			}
		
			redo.push_back(boost::bind(&editor::add_object_to_level, this, lvl, duplicate_obj));
			undo.push_back(boost::bind(&editor::remove_object_from_level, this, lvl, duplicate_obj));
		}
	}

	execute_command(
	  boost::bind(execute_functions, redo),
	  boost::bind(execute_functions, undo));
}

void editor::process_ghost_objects()
{
	if(editing_level_being_played()) {
		return;
	}

	lvl_->swap_chars(ghost_objects_);

	const size_t num_chars_before = lvl_->get_chars().size();
	const std::vector<entity_ptr> chars = lvl_->get_chars();
	foreach(const entity_ptr& p, chars) {
		p->process(*lvl_);
	}

	foreach(const entity_ptr& p, chars) {
		p->handle_event(OBJECT_EVENT_DRAW);
	}

	lvl_->swap_chars(ghost_objects_);

	foreach(entity_ptr& p, ghost_objects_) {
		if(p && p->destroyed()) {
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

int editor_resolution_manager_count = 0;

int editor_x_resolution = 0, editor_y_resolution = 0;

}

bool editor_resolution_manager::is_active()
{
	return editor_resolution_manager_count != 0;
}

editor_resolution_manager::editor_resolution_manager(int xres, int yres) :
	   original_width_(preferences::actual_screen_width()),
	   original_height_(preferences::actual_screen_height()) {
	std::cerr << "EDITOR RESOLUTION MANAGER: " << xres << ", " << yres << "\n";
	if(!editor_x_resolution) {
		if(xres != 0 && yres != 0) {
			editor_x_resolution = xres;
			editor_y_resolution = yres;
		} else {
			editor_x_resolution = 1200; //preferences::actual_screen_width() + EDITOR_SIDEBAR_WIDTH + editor_dialogs::LAYERS_DIALOG_WIDTH;
			editor_y_resolution = preferences::actual_screen_height() + EDITOR_MENUBAR_HEIGHT;
		}
	}

	if(++editor_resolution_manager_count == 1) {
		std::cerr << "EDITOR RESOLUTION: " << editor_x_resolution << "," << editor_y_resolution << "\n";
		SDL_Surface* result = graphics::set_video_mode(editor_x_resolution,editor_y_resolution,0,SDL_OPENGL|SDL_RESIZABLE|(preferences::fullscreen() ? SDL_FULLSCREEN : 0));

		if(result) {
			preferences::set_actual_screen_width(editor_x_resolution);
			preferences::set_actual_screen_height(editor_y_resolution);
		} else {
			editor_x_resolution = preferences::actual_screen_width();
			editor_y_resolution = preferences::actual_screen_height();
		}
	}
}

editor_resolution_manager::~editor_resolution_manager() {
	if(--editor_resolution_manager_count == 0) {
		preferences::set_actual_screen_width(original_width_);
		preferences::set_actual_screen_height(original_height_);
		graphics::set_video_mode(original_width_,original_height_,0,SDL_OPENGL|(preferences::resizable() ? SDL_RESIZABLE : 0)|(preferences::fullscreen() ? SDL_FULLSCREEN : 0));
	}
}

void editor::setup_for_editing()
{
	stats::flush();
	try {
		load_stats();
	} catch(...) {
		debug_console::add_message("Error parsing stats");
		std::cerr << "ERROR LOADING STATS\n";
	}

	lvl_->set_as_current_level();

	foreach(level_ptr lvl, levels_) {
		foreach(entity_ptr c, lvl->get_chars()) {
			if(entity_collides_with_level(*lvl, *c, MOVE_NONE)) {
				const int x = c->x();
				const int y = c->y();
				if(place_entity_in_level_with_large_displacement(*lvl, *c)) {
					assert(c->allow_level_collisions() || !entity_collides_with_level(*lvl, *c, MOVE_NONE));
					if(lvl == lvl_) {
						debug_console::add_message(formatter() << "Adjusted position of " << c->debug_description() << " to fit: (" << x << "," << y << ") -> (" << c->x() << "," << c->y() << ")");
					}
				} else {
					debug_console::add_message(formatter() << c->debug_description() << " is in an illegal position and can't be auto-corrected");
				}
			}
		}
	}

	g_last_edited_level() = filename_;

	tileset_dialog_.reset(new editor_dialogs::tileset_editor_dialog(*this));
	layers_dialog_.reset(new editor_dialogs::editor_layers_dialog(*this));
	current_dialog_ = tileset_dialog_.get();

	//reset the tool status.
	change_tool(tool_);
}

void editor::edit_level()
{

	glEnable(GL_BLEND);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int selected_tile = 0;

	done_ = false;
	prev_mousex_ = prev_mousey_ = -1;
	while(!done_) {
		const int scheduled_frame_end_time = SDL_GetTicks() + 20;

		handle_scrolling();
		process();

		SDL_Event event;
		while(SDL_PollEvent(&event) && !done_) {
			handle_event(event, false);
		}

		draw();

		SDL_Delay(std::max<int>(1, scheduled_frame_end_time - SDL_GetTicks()));
	}
}

bool editor::handle_event(const SDL_Event& event, bool swallowed)
{
	const bool dialog_started_with_focus = code_dialog_ && code_dialog_->has_focus() || current_dialog_ && current_dialog_->has_focus();
	if(code_dialog_ && code_dialog_->process_event(event, swallowed)) {
		return true;
	}

	if(swallowed) {
		return true;
	}

//	if(done_) {
//		return false;
//	}

	if(editor_menu_dialog_->process_event(event, false)) {
		return true;
	}

	if(editor_mode_dialog_->process_event(event, false)) {
		return true;
	}

	if(current_dialog_ && current_dialog_->process_event(event, false)) {
		return true;
	}

	if(layers_dialog_ && layers_dialog_->process_event(event, false)) {
		return true;
	}
	
	switch(event.type) {
	case SDL_QUIT:
		done_ = true;
		break;
	case SDL_KEYDOWN:
		if(event.key.keysym.sym == SDLK_ESCAPE) {
			if(confirm_quit()) {
				done_ = true;
				return true;
			}
		}

		handle_key_press(event.key);
		break;
	case SDL_MOUSEBUTTONDOWN:
		if(!dialog_started_with_focus) {
			//if the code dialog started with focus, we ignore mouse
			//presses so that the first click just unfocuses it.
			handle_mouse_button_down(event.button);
		}
		break;

	case SDL_MOUSEBUTTONUP:
		if(!dialog_started_with_focus) {
			//if the code dialog started with focus, we ignore mouse
			//presses so that the first click just unfocuses it.
			handle_mouse_button_up(event.button);
		}
		break;
	case SDL_VIDEORESIZE: {
		const SDL_ResizeEvent* const resize = reinterpret_cast<const SDL_ResizeEvent*>(&event);

		SDL_Surface* result = graphics::set_video_mode(resize->w,resize->h,0,SDL_OPENGL|SDL_RESIZABLE|(preferences::fullscreen() ? SDL_FULLSCREEN : 0));
		
		if(result) {
			editor_x_resolution = resize->w;
			editor_y_resolution = resize->h;
			preferences::set_actual_screen_width(resize->w);
			preferences::set_actual_screen_height(resize->h);
		
			reset_dialog_positions();
		}

		return false;
	}

	default:
		break;
	}

	return false;
}

void editor::process()
{
	if(code_dialog_) {
		code_dialog_->process();
	}

	if(external_code_editor_) {
		external_code_editor_->process();
	}

	if(layers_dialog_) {
		layers_dialog_->process();
	}

	if(external_code_editor_ && external_code_editor_->replace_in_game_editor() && editor_menu_dialog_) {
		std::string type;
		if(lvl_->editor_selection().empty() == false) {
			type = lvl_->editor_selection().back()->query_value("type").as_string();
		}
		if(type.empty() == false) {
			editor_menu_dialog_->set_code_button_text("edit " + type);
		} else {
			editor_menu_dialog_->set_code_button_text("");
		}
	}

	if(editor_mode_dialog_) {
		editor_mode_dialog_->refresh_selection();
	}

	g_codebar_width = code_dialog_ ? code_dialog_->width() : 0;

	if(code_dialog_ && code_dialog_->has_keyboard_focus()) {
		return;
	}

	process_ghost_objects();

	int mousex, mousey;
	const unsigned int buttons = get_mouse_state(mousex, mousey);
#if defined(__ANDROID__) && SDL_VERSION_ATLEAST(1, 3, 0)
	const Uint8* keystate = SDL_GetKeyboardState(0);
#else
	const Uint8* keystate = SDL_GetKeyState(NULL); 
#endif

	if(buttons == 0) {
		drawing_rect_ = false;
	}

	//make middle-clicking drag the screen around.
	if(prev_mousex_ != -1 && prev_mousey_ != -1 && (buttons&SDL_BUTTON_MIDDLE)) {
		const int diff_x = mousex - prev_mousex_;
		const int diff_y = mousey - prev_mousey_;
		xpos_ -= diff_x*zoom_;
		ypos_ -= diff_y*zoom_;
	}

	prev_mousex_ = mousex;
	prev_mousey_ = mousey;

	
	const int selectx = round_tile_size(xpos_ + mousex*zoom_);
	const int selecty = round_tile_size(ypos_ + mousey*zoom_);

	const bool object_mode = (tool() == TOOL_ADD_OBJECT || tool() == TOOL_SELECT_OBJECT);
	if(property_dialog_ && g_variable_editing) {
		const int diffx = (xpos_ + mousex*zoom_) - anchorx_;
		const int diffy = (ypos_ + mousey*zoom_) - anchory_;
		int diff = 0;
		switch(g_variable_editing->type()) {
		case editor_variable_info::XPOSITION:
			diff = diffx;
			break;
		case editor_variable_info::YPOSITION:
			diff = diffy;
			break;
		default:
			break;
		}

		if(property_dialog_ && property_dialog_->get_entity()) {
			variant new_value;
			const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;

			if(g_variable_editing->type() == editor_variable_info::TYPE_POINTS) {
				std::vector<variant> items = g_variable_editing_original_value.as_list();
				ASSERT_LOG(g_variable_editing_index >= 0 && g_variable_editing_index < items.size(), "Variable editing points invalid: " << g_variable_editing_index << " / " << items.size());
				point orig_point(items[g_variable_editing_index]);
				point new_point(orig_point.x + diffx, orig_point.y + diffy);
				if(!ctrl_pressed) {
					new_point.x = new_point.x - new_point.x%(TileSize/2);
					new_point.y = new_point.y - new_point.y%(TileSize/2);
				}
				items[g_variable_editing_index] = new_point.write();
				new_value = variant(&items);

			} else {
				int new_value_int = g_variable_editing_original_value.as_int() + diff;
				if(!ctrl_pressed) {
					new_value_int = new_value_int - new_value_int%(TileSize/2);
				}

				new_value = variant(new_value_int);
			}

			if(!new_value.is_null()) {
				std::vector<boost::function<void()> > undo, redo;
				generate_mutate_commands(property_dialog_->get_entity(), g_variable_editing->variable_name(), new_value, undo, redo);
				execute_command(
				  boost::bind(execute_functions, redo),
				  boost::bind(execute_functions, undo));

				//We don't want this to actually be undoable, since the whole
				//drag operation will be undoable when we're done, so remove
				//from the undo stack.
				undo_.pop_back();
			}
		}
	} else if(object_mode && !buttons) {
		//remove ghost objects and re-add them. This guarantees ghost
		//objects always remain at the end of the level ordering.
		remove_ghost_objects();
		entity_ptr c = lvl_->get_next_character_at_point(xpos_ + mousex*zoom_, ypos_ + mousey*zoom_, xpos_, ypos_);
		foreach(const entity_ptr& ghost, ghost_objects_) {
			lvl_->add_character(ghost);
		}

		lvl_->set_editor_highlight(c);
		//See if we should add ghost objects. Human objects don't get
		//ghost (it doesn't make much sense for them to do so)
		if(ghost_objects_.empty() && c && !c->is_human() && !editing_level_being_played()) {
			//we have an object but no ghost for it, make the
			//object's ghost and deploy it.
			entity_ptr clone = c->clone();
			if(clone && !entity_collides_with_level(*lvl_, *clone, MOVE_NONE)) {
				ghost_objects_.push_back(clone);
				lvl_->add_character(clone);

				//fire the event to tell the ghost it's been added.
				lvl_->swap_chars(ghost_objects_);
				clone->handle_event(OBJECT_EVENT_START_LEVEL);
				lvl_->swap_chars(ghost_objects_);
			}
		} else if(ghost_objects_.empty() == false && !c) {
			//ghost objects are present but we are no longer moused-over
			//an object, so remove the ghosts.
			remove_ghost_objects();
			ghost_objects_.clear();
		}
	}else if(object_mode && lvl_->editor_highlight()) {
		foreach(level_ptr lvl, levels_) {
			lvl->set_editor_dragging_objects();
		}
		
		//we're handling objects, and a button is down, and we have an
		//object under the mouse. This means we are dragging something.

		// check if cursor is not in the sidebar!
		if (mousex < editor_mode_dialog_->x()) {
			handle_object_dragging(mousex, mousey);
		}
	} else if(drawing_rect_) {
		handle_drawing_rect(mousex, mousey);
	}

	if(!object_mode) {
		//not in object mode, the picker still highlights objects,
		//though it won't create ghosts, so remove all ghosts.
		if(tool() == TOOL_PICKER) {
			entity_ptr c = lvl_->get_next_character_at_point(xpos_ + mousex*zoom_, ypos_ + mousey*zoom_, xpos_, ypos_);
			lvl_->set_editor_highlight(c);
		} else {
			lvl_->set_editor_highlight(entity_ptr());
		}

		remove_ghost_objects();
		ghost_objects_.clear();
	}

	//if we're drawing with a pencil see if we add a new tile
	if(tool() == TOOL_PENCIL && dragging_ && buttons) {
		const int xpos = xpos_ + mousex*zoom_;
		const int ypos = ypos_ + mousey*zoom_;
		point p(xpos, ypos);
		if(std::find(g_current_draw_tiles.begin(), g_current_draw_tiles.end(), p) == g_current_draw_tiles.end()) {
			g_current_draw_tiles.push_back(p);

			if(buttons&SDL_BUTTON_LEFT) {
				add_tile_rect(p.x, p.y, p.x, p.y);
			} else {
				remove_tile_rect(p.x, p.y, p.x, p.y);
			}
		}
	}

	foreach(level_ptr lvl, levels_) {
		lvl->complete_rebuild_tiles_in_background();
	}
}

void editor::set_pos(int x, int y)
{
	xpos_ = x;
	ypos_ = y;
}

void editor::set_playing_level(level_ptr lvl)
{
	levels_.resize(1);
	levels_.push_back(lvl);
	lvl_ = lvl;
}

void editor::toggle_active_level()
{
	std::vector<level_ptr>::iterator i = std::find(levels_.begin(), levels_.end(), lvl_);
	if(i != levels_.end()) {
		++i;
		if(i == levels_.end()) {
			i = levels_.begin();
		}

		lvl_ = *i;
	}
	lvl_->set_as_current_level();
}

bool editor::editing_level_being_played() const
{
	return levels_.size() == 2 && std::find(levels_.begin(), levels_.end(), lvl_) != levels_.begin();
}

void editor::reset_dialog_positions()
{
	if(editor_mode_dialog_) {
		editor_mode_dialog_->set_loc(graphics::screen_width() - editor_mode_dialog_->width(), editor_mode_dialog_->y());
	}

#define SET_DIALOG_POS(d) if(d) { \
	d->set_loc(graphics::screen_width() - d->width(), d->y()); \
	\
	d->set_dim(d->width(), \
	 std::max<int>(10, preferences::actual_screen_height() - d->y())); \
}
	SET_DIALOG_POS(character_dialog_);
	SET_DIALOG_POS(property_dialog_);
	SET_DIALOG_POS(tileset_dialog_);
#undef SET_DIALOG_POS

	if(layers_dialog_ && editor_mode_dialog_) {
		layers_dialog_->set_loc(editor_mode_dialog_->x() - layers_dialog_->width(), EDITOR_MENUBAR_HEIGHT);
		layers_dialog_->set_dim(layers_dialog_->width(), preferences::actual_screen_height() - EDITOR_MENUBAR_HEIGHT);
	}

	if(editor_menu_dialog_ && editor_mode_dialog_) {
		editor_menu_dialog_->set_dim(preferences::actual_screen_width() - editor_mode_dialog_->width(), editor_menu_dialog_->height());
	}
}

namespace {
	bool sort_entity_zsub_orders(const entity_ptr& a, const entity_ptr& b) {
	return a->zsub_order() < b->zsub_order();
}
}

void editor::execute_shift_object(entity_ptr e, int dx, int dy)
{
	begin_command_group();
	foreach(level_ptr lvl, levels_) {
		entity_ptr obj = lvl->get_entity_by_label(e->label());
		if(obj) {
			execute_command(boost::bind(&editor::move_object, this, lvl, obj, obj->x()+dx,obj->y()+dy),
							boost::bind(&editor::move_object,this, lvl, obj,obj->x(),obj->y()));
		}
	}
	end_command_group();
}

void editor::handle_key_press(const SDL_KeyboardEvent& key)
{
	if(key.keysym.sym == SDLK_e && (key.keysym.mod&KMOD_ALT) && levels_.size() > 1) {
		done_ = true;
		return;
	}

	if(key.keysym.sym == SDLK_s && (key.keysym.mod&KMOD_ALT)) {
		IMG_SaveFrameBuffer((std::string(preferences::user_data_path()) + "screenshot.png").c_str(), 5);
	}

	if(key.keysym.sym == SDLK_1 && key.keysym.mod&KMOD_CTRL) {
		duplicate_selected_objects();
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

	if(key.keysym.sym == SDLK_h) {
		preferences::toogle_debug_hitboxes();
	}
	
	if(key.keysym.sym == SDLK_KP8) {
		begin_command_group();
		foreach(const entity_ptr& e, lvl_->editor_selection()){
			execute_shift_object(e, 0, -2);
		}
		end_command_group();
	}

	if(key.keysym.sym == SDLK_KP5) {
		begin_command_group();
		foreach(const entity_ptr& e, lvl_->editor_selection()){
			execute_shift_object(e, 0, 2);
		}
		end_command_group();
	}
	
	if(key.keysym.sym == SDLK_KP4) {
		begin_command_group();
		foreach(const entity_ptr& e, lvl_->editor_selection()){
			execute_shift_object(e, -2, 0);
		}
		end_command_group();
	}
	
	if(key.keysym.sym == SDLK_KP6) {
		begin_command_group();
		foreach(const entity_ptr& e, lvl_->editor_selection()){
			execute_shift_object(e, 2, 0);
		}
		end_command_group();
	}
	
	if(key.keysym.sym == SDLK_EQUALS || key.keysym.sym == SDLK_MINUS ) {
		if(lvl_->editor_selection().size() > 1){
			
			//store them in a new container
			std::vector <entity_ptr> v2;
			foreach(const entity_ptr& e, lvl_->editor_selection()){
				v2.push_back(e.get());
			}
			//sort this container in ascending zsub_order
			std::sort(v2.begin(),v2.end(), sort_entity_zsub_orders);
					
			//if it was +, then move the backmost object in front of the frontmost object.
			//if it was -, do vice versa (frontmost object goes behind backmost object)
			if(key.keysym.sym == SDLK_EQUALS){
				begin_command_group();
				foreach(level_ptr lvl, levels_) {
					entity_ptr obj = lvl->get_entity_by_label(v2.front()->label());
					if(obj) {
						execute_command(boost::bind(&entity::set_zsub_order, obj, v2.back()->zsub_order()+1),
										boost::bind(&entity::set_zsub_order, obj, v2.front()->zsub_order() ));
					}
				}
				end_command_group();
			}else if(key.keysym.sym == SDLK_MINUS){
				begin_command_group();
				foreach(level_ptr lvl, levels_) {
					entity_ptr obj = lvl->get_entity_by_label(v2.back()->label());
					if(obj) {
						execute_command(boost::bind(&entity::set_zsub_order, obj, v2.front()->zsub_order()-1),
										boost::bind(&entity::set_zsub_order, obj, v2.back()->zsub_order() ));
				
					}
				}
				end_command_group();
			}
		}
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
		undo.push_back(boost::bind(&editor_dialogs::property_editor_dialog::set_entity_group, property_dialog_.get(), lvl_->editor_selection()));
		redo.push_back(boost::bind(&level::editor_clear_selection, lvl_.get()));
		//we want to clear the objects in the property dialog
		redo.push_back(boost::bind(&editor_dialogs::property_editor_dialog::set_entity_group, property_dialog_.get(), std::vector<entity_ptr>()));
		foreach(const entity_ptr& e, lvl_->editor_selection()) {
			generate_remove_commands(e, undo, redo);
			undo.push_back(boost::bind(&level::editor_select_object, lvl_.get(), e));
		}
		execute_command(
		  boost::bind(execute_functions, redo),
		  boost::bind(execute_functions, undo));
	}

	if(!tile_selection_.empty() && (key.keysym.sym == SDLK_DELETE || key.keysym.sym == SDLK_BACKSPACE)) {
		int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
		std::vector<boost::function<void()> > redo, undo;

		foreach(level_ptr lvl, levels_) {
			foreach(const point& p, tile_selection_.tiles) {
				const int x = p.x*TileSize;
				const int y = p.y*TileSize;

				min_x = std::min(x, min_x);
				max_x = std::max(x, max_x);
				min_y = std::min(y, min_y);
				max_y = std::max(y, max_y);

				redo.push_back(boost::bind(&level::clear_tile_rect, lvl.get(), x, y, x, y));
				std::map<int, std::vector<std::string> > old_tiles;
				lvl->get_all_tiles_rect(x, y, x, y, old_tiles);
				for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
					undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x, y, x, y, i->second));
				}
			}

			if(!tile_selection_.tiles.empty()) {
				undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
				redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
			}
		}

		execute_command(
		  boost::bind(execute_functions, redo),
		  boost::bind(execute_functions, undo));
	}

	if(key.keysym.sym == SDLK_o) {
		editor_menu_dialog_->open_level();
	}

	if(key.keysym.sym == SDLK_s && (key.keysym.mod&KMOD_CTRL)) {
		save_level();
	}

	if(key.keysym.sym == SDLK_f) {
		toggle_facing();
	}

	if(key.keysym.sym == SDLK_i) {
		toggle_upside_down();
	}

	if(key.keysym.sym == SDLK_r &&
	   (key.keysym.mod&KMOD_CTRL) && levels_.size() == 2 &&
	   lvl_ == levels_.back()) {

		entity_ptr player;
		if(lvl_->player()) {
			player.reset(&lvl_->player()->get_entity());
		}

		levels_.front()->transfer_state_to(*levels_.back());

		if(player) {
			if(place_entity_in_level(*lvl_, *player)) {
				lvl_->add_player(player);
			}
		}

		controls::new_level(lvl_->cycle(), lvl_->players().empty() ? 1 : lvl_->players().size(), multiplayer::slot());

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

void editor::reset_playing_level(bool keep_player)
{
	if(levels_.size() == 2 && lvl_ == levels_.back()) {
		entity_ptr player;
		if(keep_player && lvl_->player()) {
			player.reset(&lvl_->player()->get_entity());
		}

		levels_.front()->transfer_state_to(*levels_.back());

		if(player) {
			if(place_entity_in_level(*lvl_, *player)) {
				lvl_->add_player(player);
			}
		}

		controls::new_level(lvl_->cycle(), lvl_->players().empty() ? 1 : lvl_->players().size(), multiplayer::slot());

	}
}

void editor::toggle_pause() const
{
	if(level_runner::get_current()) {
		level_runner::get_current()->toggle_pause();
	}
}

void editor::handle_scrolling()
{
	if(code_dialog_ && code_dialog_->has_keyboard_focus()) {
		return;
	}

	const int ScrollSpeed = 24*zoom_;
	const int FastScrollSpeed = 384*zoom_;

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
			foreach(level_ptr lvl, levels_) {
				entity_ptr obj = lvl->get_entity_by_label(e->label());
				if(obj) {
					redo.push_back(boost::bind(&editor::move_object, this, lvl, obj, e->x() + delta_x, e->y() + delta_y));
					undo.push_back(boost::bind(&editor::move_object, this, lvl, obj, obj->x(), obj->y()));
				}
			}

		}

		//all dragging that is done should be treated as one operation
		//from an undo/redo perspective. So, we see if we're already dragging
		//and have performed existing drag operations, and if so we
		//roll the previous undo command into this.
		boost::function<void()> undo_fn = boost::bind(execute_functions, undo);

		if(g_started_dragging_object && undo_.empty() == false && undo_.back().type == COMMAND_TYPE_DRAG_OBJECT) {
			undo_fn = undo_.back().undo_command;
			undo_command();
		}

		execute_command(boost::bind(execute_functions, redo), undo_fn, COMMAND_TYPE_DRAG_OBJECT);

		g_started_dragging_object = true;

		remove_ghost_objects();
		ghost_objects_.clear();
	}
}

void editor::handle_drawing_rect(int mousex, int mousey)
{
	const unsigned int buttons = get_mouse_state(mousex, mousey);

	const int xpos = xpos_ + mousex*zoom_;
	const int ypos = ypos_ + mousey*zoom_;

	int x1 = xpos;
	int x2 = anchorx_;
	int y1 = ypos;
	int y2 = anchory_;
	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	x1 = round_tile_size(x1);
	x2 = round_tile_size(x2 + TileSize);
	y1 = round_tile_size(y1);
	y2 = round_tile_size(y2 + TileSize);

	const rect new_rect = rect(x1, y1, x2 - x1, y2 - y1);
	if(g_rect_drawing == new_rect) {
		return;
	}

	if(tool() == TOOL_ADD_RECT) {
		lvl_->freeze_rebuild_tiles_in_background();
		if(tmp_undo_.get()) {
			tmp_undo_->undo_command();
		}

		if(buttons == SDL_BUTTON_LEFT) {
			add_tile_rect(anchorx_, anchory_, xpos, ypos);
		} else {
			remove_tile_rect(anchorx_, anchory_, xpos, ypos);
		}

		tmp_undo_.reset(new executable_command(undo_.back()));
		undo_.pop_back();
		lvl_->unfreeze_rebuild_tiles_in_background();
	}
	g_rect_drawing = new_rect;
}

void editor::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	if(event.button == SDL_BUTTON_WHEELUP || event.button == SDL_BUTTON_WHEELDOWN) {
		return;
	}
	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	const bool shift_pressed = (SDL_GetModState()&(KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
	const bool alt_pressed = (SDL_GetModState()&KMOD_ALT) != 0;
	int mousex, mousey;
	const unsigned int buttons = get_mouse_state(mousex, mousey);

	anchorx_ = xpos_ + mousex*zoom_;
	anchory_ = ypos_ + mousey*zoom_;
	if(event.button == SDL_BUTTON_MIDDLE && !alt_pressed) {
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

	if(adding_points_.empty() == false) {
		if(event.button == SDL_BUTTON_LEFT && property_dialog_ && property_dialog_->get_entity()) {
			const int xpos = anchorx_;
			const int ypos = anchory_;
			std::cerr << "ADD POINT: " << xpos << ", " << ypos << "\n";

			entity_ptr c = property_dialog_->get_entity();

			game_logic::formula_callable* obj_vars = c->query_value("vars").mutable_callable();
			variant current_value = obj_vars->query_value(adding_points_);
			std::vector<variant> new_value;
			if(current_value.is_list()) {
				new_value = current_value.as_list();
			}

			std::vector<variant> point;
			point.push_back(variant(xpos));
			point.push_back(variant(ypos));
			new_value.push_back(variant(&point));

			std::vector<boost::function<void()> > redo, undo;
			generate_mutate_commands(c, adding_points_, variant(&new_value), undo, redo);

			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));


			start_adding_points(adding_points_);

		} else {
			start_adding_points("");
		}
	} else if(tool() == TOOL_EDIT_SEGMENTS) {
		if(point_in_rect(point(anchorx_, anchory_), lvl_->boundaries())) {
			const int xpos = anchorx_ - lvl_->boundaries().x();
			const int ypos = anchory_ - lvl_->boundaries().y();
			const int segment = lvl_->segment_width() ? xpos/lvl_->segment_width() : ypos/lvl_->segment_height();

			if(selected_segment_ == -1) {
				selected_segment_ = segment;
				segment_dialog_->set_segment(segment);
			} else if(buttons&SDL_BUTTON_RIGHT) {
				if(segment != selected_segment_ && selected_segment_ >= 0) {
					variant next = lvl_->get_var(formatter() << "segments_after_" << selected_segment_);
					std::vector<variant> v;
					if(next.is_list()) {
						for(int n = 0; n != next.num_elements(); ++n) {
							v.push_back(next[n]);
						}
					}

					std::vector<variant>::iterator i = std::find(v.begin(), v.end(), variant(segment));
					if(i != v.end()) {
						v.erase(i);
					} else {
						v.push_back(variant(segment));
					}

					lvl_->set_var(formatter() << "segments_after_" << selected_segment_, variant(&v));
				}
			}
		} else {
			selected_segment_ = -1;
			segment_dialog_->set_segment(selected_segment_);
		}
	} else if(tool() == TOOL_PICKER) {
		if(lvl_->editor_highlight()) {
			change_tool(TOOL_ADD_OBJECT);

			variant node = lvl_->editor_highlight()->write();
			const std::string type = node["type"].as_string();
			for(int n = 0; n != all_characters().size(); ++n) {
				const enemy_type& c = all_characters()[n];
				if(c.node["type"].as_string() == type) {
					character_dialog_->select_category(c.category);
					character_dialog_->set_character(n);
					return;
				}
			}
			return;
		} else {
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
						//if we're in adding objects mode then switch to adding tiles mode.
						if(tool_ == TOOL_ADD_OBJECT) {
							change_tool(TOOL_ADD_RECT);
						}
						return;
					}
				}
			}
		}
	} else if(editing_tiles() && !tile_selection_.empty() &&
	   std::binary_search(tile_selection_.tiles.begin(), tile_selection_.tiles.end(), point(round_tile_size(anchorx_)/TileSize, round_tile_size(anchory_)/TileSize))) {
		//we are beginning to drag our selection
		dragging_ = true;
	} else if(tool() == TOOL_ADD_RECT || tool() == TOOL_SELECT_RECT) {
		tmp_undo_.reset();
		drawing_rect_ = true;
		g_rect_drawing = rect();
	} else if(tool() == TOOL_MAGIC_WAND) {
		drawing_rect_ = false;
	} else if(tool() == TOOL_PENCIL) {
		drawing_rect_ = false;
		dragging_ = true;
		point p(anchorx_, anchory_);
		if(buttons&SDL_BUTTON_LEFT) {
			add_tile_rect(p.x, p.y, p.x, p.y);
		} else {
			remove_tile_rect(p.x, p.y, p.x, p.y);
		}
		g_current_draw_tiles.clear();
		g_current_draw_tiles.push_back(p);
	} else if(property_dialog_ && variable_info_selected(property_dialog_->get_entity(), anchorx_, anchory_, zoom_)) {
		g_variable_editing = variable_info_selected(property_dialog_->get_entity(), anchorx_, anchory_, zoom_, &g_variable_editing_index);
		g_variable_editing_original_value = property_dialog_->get_entity()->query_value(g_variable_editing->variable_name());

		if(g_variable_editing->type() == editor_variable_info::TYPE_POINTS && event.button == SDL_BUTTON_RIGHT) {
			std::vector<variant> points = g_variable_editing_original_value.as_list();
			ASSERT_LOG(g_variable_editing_index >= 0 && g_variable_editing_index < points.size(), "INVALID VALUE WHEN EDITING POINTS: " << g_variable_editing_index << " / " << points.size());

			points.erase(points.begin() + g_variable_editing_index);

			variant new_value(&points);

			std::vector<boost::function<void()> > undo, redo;
			generate_mutate_commands(property_dialog_->get_entity(), g_variable_editing->variable_name(), new_value, undo, redo);
			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));

			g_variable_editing = NULL;
			g_variable_editing_original_value = variant();
			g_variable_editing_index = -1;
		}

		//If we select a variable to edit, return here so we don't select
		//another object instead, swallowing the event.
		return;
		
	} else if(tool() == TOOL_SELECT_OBJECT && !lvl_->editor_highlight()) {
		//dragging a rectangle to select objects
		drawing_rect_ = true;
	} else if(property_dialog_) {
		property_dialog_->set_entity(lvl_->editor_highlight());

		set_code_file();
	}

	if(lvl_->editor_highlight()) {
		if(std::count(lvl_->editor_selection().begin(),
		              lvl_->editor_selection().end(), lvl_->editor_highlight()) == 0) {
			//set the object as selected in the editor.
			if(!shift_pressed) {
				lvl_->editor_clear_selection();
			}

			lvl_->editor_select_object(lvl_->editor_highlight());

			property_dialog_->set_entity_group(lvl_->editor_selection());

			if(!lvl_->editor_selection().empty() && tool() == TOOL_ADD_OBJECT) {
				//we are in add objects mode and we clicked on an object,
				//so change to select mode.
				change_tool(TOOL_SELECT_OBJECT);
			}

			current_dialog_ = property_dialog_.get();
		} else if(shift_pressed) {
			lvl_->editor_deselect_object(lvl_->editor_highlight());
		}

		//start dragging the object
		selected_entity_startx_ = lvl_->editor_highlight()->x();
		selected_entity_starty_ = lvl_->editor_highlight()->y();

		g_started_dragging_object = false;

	} else {
		//clear any selection in the editor
		lvl_->editor_clear_selection();
	}

	if(tool() == TOOL_ADD_OBJECT && event.button == SDL_BUTTON_LEFT && !lvl_->editor_highlight()) {
		variant_builder node;
		node.merge_object(enemy_types[cur_object_].node);
		node.set("x", (ctrl_pressed ? anchorx_ : round_tile_size(anchorx_)));
		node.set("y", (ctrl_pressed ? anchory_ : round_tile_size(anchory_)));
		node.set("face_right", face_right_);

		node.set("upside_down", upside_down_);

		entity_ptr c(entity::build(node.build()));

		//any vars that require formula initialization are calculated here.
		std::map<std::string, variant> vars;
		foreach(const editor_variable_info& info, c->editor_info()->vars()) {
			if(info.formula()) {
				vars[info.variable_name()] = info.formula()->execute(*c);
			}
		}
		
		//if we have parallax, offset the object so it's placed at the same position it's graphically visible at
		c->set_x( c->x() +  + ((1000 - (c->parallax_scale_millis_x()))* xpos_ )/1000);
		c->set_y( c->y() +  + ((1000 - (c->parallax_scale_millis_y()))* ypos_ )/1000);
		

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
			if(!shift_pressed) {
				begin_command_group();
				foreach(level_ptr lvl, levels_) {
					entity_ptr obj(c->backup());
					execute_command(
					  boost::bind(&editor::add_object_to_level, this, lvl, obj),
					  boost::bind(&editor::add_object_to_level, this, lvl, &lvl->player()->get_entity()));
				}
				end_command_group();
			} else {
				begin_command_group();
				foreach(level_ptr lvl, levels_) {
					entity_ptr obj(c->backup());
					execute_command(
					  boost::bind(&editor::add_multi_object_to_level, this, lvl, obj),
					  boost::bind(&editor::add_object_to_level, this, lvl, &lvl->player()->get_entity()));
				}
				end_command_group();
			}

		} else {
			begin_command_group();
			foreach(level_ptr lvl, levels_) {
				entity_ptr obj(c->backup());
				execute_command(
				  boost::bind(&editor::add_object_to_level, this, lvl, obj),
				  boost::bind(&editor::remove_object_from_level, this, lvl, obj));
				std::cerr << "ADD OBJECT: " << obj->x() << "," << obj->y() << "\n";
			}
			end_command_group();
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

	if((event.button == SDL_BUTTON_WHEELUP || event.button == SDL_BUTTON_WHEELDOWN) 
		&& xpos < editor_x_resolution-sidebar_width() ) {
		if(event.button == SDL_BUTTON_WHEELUP) {
			zoom_in();
		} else {
			zoom_out();
		}
		return;
	}

	if(g_variable_editing) {
		if(property_dialog_ && property_dialog_->get_entity()) {
			entity_ptr e = property_dialog_->get_entity();
			const std::string& var = g_variable_editing->variable_name();

			begin_command_group();
			foreach(level_ptr lvl, levels_) {
				entity_ptr obj = lvl->get_entity_by_label(e->label());
				if(obj) {
					execute_command(
					  boost::bind(&editor::mutate_object_value, this, lvl, obj.get(), var, e->query_value(var)),
					  boost::bind(&editor::mutate_object_value, this, lvl, obj.get(), var, g_variable_editing_original_value));
				}
			}
			end_command_group();
			property_dialog_->init();
		}
		g_variable_editing = NULL;
		return;
	}

	if(resizing_left_level_edge || resizing_right_level_edge ||resizing_top_level_edge || resizing_bottom_level_edge) {
		rect boundaries = modify_selected_rect(*this, lvl_->boundaries(), xpos, ypos);

		resizing_left_level_edge = resizing_right_level_edge = resizing_top_level_edge = resizing_bottom_level_edge = false;

		if(boundaries != lvl_->boundaries()) {
			begin_command_group();
			foreach(level_ptr lvl, levels_) {
				execute_command(
				  boost::bind(&level::set_boundaries, lvl.get(), boundaries),
				  boost::bind(&level::set_boundaries, lvl.get(), lvl->boundaries()));
			}
			end_command_group();
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

			foreach(level_ptr lvl, levels_) {
				foreach(const point& p, tile_selection_.tiles) {
					const int x = (p.x+diffx)*TileSize;
					const int y = (p.y+diffy)*TileSize;
					undo.push_back(boost::bind(&level::clear_tile_rect,lvl.get(), x, y, x, y));
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
					lvl->get_all_tiles_rect(x, y, x, y, old_tiles);
					for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
						undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x, y, x, y, i->second));
						redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x, y, x, y, std::vector<std::string>(1,"")));
					}

					old_tiles.clear();
	
					x += diffx*TileSize;
					y += diffy*TileSize;

					min_x = std::min(x, min_x);
					max_x = std::max(x, max_x);
					min_y = std::min(y, min_y);
					max_y = std::max(y, max_y);

					lvl->get_all_tiles_rect(x, y, x, y, old_tiles);
					for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
						undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x, y, x, y, i->second));
						redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x, y, x, y, std::vector<std::string>(1,"")));
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
					lvl->get_all_tiles_rect(x, y, x, y, old_tiles);
					for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
						redo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x + diffx*TileSize, y + diffy*TileSize, x + diffx*TileSize, y + diffy*TileSize, i->second));
					}
				}

				if(!tile_selection_.tiles.empty()) {
					undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
					redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
				}
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

				lvl_->freeze_rebuild_tiles_in_background();
				if(tmp_undo_.get()) {
					//if we have a temporary change that was made while dragging
					//to preview the change, undo that now.
					tmp_undo_->undo_command();
					tmp_undo_.reset();
				}

				add_tile_rect(anchorx_, anchory_, xpos, ypos);
				lvl_->unfreeze_rebuild_tiles_in_background();
			} else if(tool() == TOOL_SELECT_RECT) {
				select_tile_rect(anchorx_, anchory_, xpos, ypos);
			}
			  
		} else if(event.button == SDL_BUTTON_RIGHT) {
			lvl_->freeze_rebuild_tiles_in_background();
			if(tmp_undo_.get()) {
				//if we have a temporary change that was made while dragging
				//to preview the change, undo that now.
				tmp_undo_->undo_command();
				tmp_undo_.reset();
			}
			remove_tile_rect(anchorx_, anchory_, xpos, ypos);
			lvl_->unfreeze_rebuild_tiles_in_background();
		}
	} else {
		//some kind of object editing
		if(event.button == SDL_BUTTON_RIGHT) {
			std::vector<boost::function<void()> > undo, redo;
			const rect rect_selected(rect::from_coordinates(anchorx_, anchory_, xpos, ypos));
			std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect_selected, xpos_, ypos_);

			//Delete all the objects in the rect.
			foreach(const entity_ptr& c, chars) {
				if(c->spawned_by().empty() == false) {
					continue;
				}
				std::cerr << "REMOVING RECT CHAR: " << c->debug_description() << "\n";
				foreach(level_ptr lvl, levels_) {
					entity_ptr obj = lvl->get_entity_by_label(c->label());
					generate_remove_commands(obj, undo, redo);
				}
			}

			if(property_dialog_ && property_dialog_.get() == current_dialog_ && property_dialog_->get_entity() && property_dialog_->get_entity()->editor_info()) {
				//As well as removing objects, we will remove any vertices
				//that we see.
				foreach(const editor_variable_info& var, property_dialog_->get_entity()->editor_info()->vars()) {
					const std::string& name = var.variable_name();
					const editor_variable_info::VARIABLE_TYPE type = var.type();
					if(type != editor_variable_info::TYPE_POINTS) {
						continue;
					}

					variant value = property_dialog_->get_entity()->query_value(name);
					if(!value.is_list()) {
						continue;
					}

					std::vector<point> points;
					foreach(const variant& v, value.as_list()) {
						points.push_back(point(v));
					}

					bool modified = false;
					for(std::vector<point>::iterator i = points.begin(); i != points.end(); ) {
						if(point_in_rect(*i, rect_selected)) {
							modified = true;
							i = points.erase(i);
						} else {
							++i;
						}
					}

					if(modified) {
						std::vector<variant> points_var;
						foreach(const point& p, points) {
							points_var.push_back(p.write());
						}

						generate_mutate_commands(property_dialog_->get_entity(), name, variant(&points_var), undo, redo);
					}
				}
			}

			execute_command(
			  boost::bind(execute_functions, redo),
			  boost::bind(execute_functions, undo));
		} else if(tool() == TOOL_SELECT_OBJECT && drawing_rect_) {
			std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect::from_coordinates(anchorx_, anchory_, xpos, ypos), xpos_, ypos_);
			if(chars.empty()) {
				//no chars is just a no-op.
				drawing_rect_ = dragging_ = false;
				return;
			}

			foreach(const entity_ptr& c, chars) {
				lvl_->editor_select_object(c);
			}

			property_dialog_->set_entity_group(lvl_->editor_selection());

			if(lvl_->editor_selection().size() == 1) {
				current_dialog_ = property_dialog_.get();
				property_dialog_->set_entity(lvl_->editor_selection().front());

				set_code_file();
			} else {
				current_dialog_ = property_dialog_.get();
			}
		}
	}

	drawing_rect_ = dragging_ = false;
}


void editor::load_stats()
{
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

	std::vector<boost::function<void()> > undo, redo;

	foreach(level_ptr lvl, levels_) {
		std::vector<std::string> old_rect;
		lvl->get_tile_rect(zorder, x1, y1, x2, y2, old_rect);

		redo.push_back(boost::bind(&level::add_tile_rect, lvl.get(), zorder, x1, y1, x2, y2, tile_id));
		undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), zorder, x1, y1, x2, y2, old_rect));

		std::vector<int> layers;
		layers.push_back(zorder);
		undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), layers));
		redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), layers));
	}

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
	foreach(level_ptr lvl, levels_) {
		lvl->set_tile_layer_speed(tilesets[cur_tileset_].zorder,
		                          tilesets[cur_tileset_].x_speed,
								  tilesets[cur_tileset_].y_speed);
	}
}

void editor::remove_tile_rect(int x1, int y1, int x2, int y2)
{
	x1 += ((100 - tilesets[cur_tileset_].x_speed)*xpos_)/100;
	x2 += ((100 - tilesets[cur_tileset_].x_speed)*xpos_)/100;
	y1 += ((100 - tilesets[cur_tileset_].y_speed)*ypos_)/100;
	y2 += ((100 - tilesets[cur_tileset_].y_speed)*ypos_)/100;

	if(x2 < x1) {
		std::swap(x1, x2);
	}

	if(y2 < y1) {
		std::swap(y1, y2);
	}

	std::vector<boost::function<void()> > redo, undo;
	foreach(level_ptr lvl, levels_) {

		std::map<int, std::vector<std::string> > old_tiles;
		lvl->get_all_tiles_rect(x1, y1, x2, y2, old_tiles);
		for(std::map<int, std::vector<std::string> >::const_iterator i = old_tiles.begin(); i != old_tiles.end(); ++i) {
			undo.push_back(boost::bind(&level::add_tile_rect_vector, lvl.get(), i->first, x1, y1, x2, y2, i->second));
		}

		redo.push_back(boost::bind(&level::clear_tile_rect, lvl.get(), x1, y1, x2, y2));
		undo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
		redo.push_back(boost::bind(&level::start_rebuild_tiles_in_background, lvl.get(), std::vector<int>()));
	}

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

void editor::move_object(level_ptr lvl, entity_ptr e, int new_x, int new_y)
{
	current_level_scope scope(lvl.get());
	lvl->relocate_object(e, new_x, new_y);
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

	foreach(level_ptr lvl, levels_) {
		lvl->set_tile_layer_speed(tilesets[cur_tileset_].zorder,
		                          tilesets[cur_tileset_].x_speed,
								  tilesets[cur_tileset_].y_speed);
	}
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
		case TOOL_ADD_OBJECT:
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
	selected_segment_ = -1;

	std::cerr << "CHANGE TOOL: " << (int)tool << "\n";

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
	case TOOL_EDIT_SEGMENTS: {

		if(!segment_dialog_) {
			segment_dialog_.reset(new editor_dialogs::segment_editor_dialog(*this));
		}
	
		current_dialog_ = segment_dialog_.get();
		segment_dialog_->set_segment(selected_segment_);
		break;
	}
	}

	if(editor_mode_dialog_) {
		editor_mode_dialog_->init();
	}

	reset_dialog_positions();
}

void editor::save_level_as(const std::string& fname)
{
	const std::string id = module::make_module_id(fname);
	all_editors.erase(filename_);
	all_editors[id] = this;

	std::string path = module::get_id(fname);
	std::string modname = module::get_module_id(fname);
	sys::write_file(module::get_module_path(modname) + preferences::level_path() + path, "");
	loadlevel::load_level_paths();
	filename_ = id;
	save_level();
	g_last_edited_level() = id;
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

bool editor::confirm_quit(bool allow_cancel)
{
	if(!level_changed_) {
		return true;
	}

	const int center_x = graphics::screen_width()/2;
	const int center_y = graphics::screen_height()/2;
	using namespace gui;
	dialog d(center_x - 140, center_y - 100, center_x + 140, center_y + 100);

	d.add_widget(widget_ptr(new label("Do you want to save the level?", graphics::color_white())), dialog::MOVE_DOWN);

	gui::grid* grid = new gui::grid(allow_cancel ? 3 : 2);

	int result = 0;
	grid->add_col(widget_ptr(
	  new button(widget_ptr(new label("Yes", graphics::color_white())),
	             boost::bind(quit_editor_result, &d, &result, 0))));
	grid->add_col(widget_ptr(
	  new button(widget_ptr(new label("No", graphics::color_white())),
	             boost::bind(quit_editor_result, &d, &result, 1))));
	if(allow_cancel) {
		grid->add_col(widget_ptr(
		  new button(widget_ptr(new label("Cancel", graphics::color_white())),
		             boost::bind(quit_editor_result, &d, &result, 2))));
	}
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
	controls::control_backup_scope ctrl_backup;

	toggle_active_level();

	lvl_->set_id(filename_);

	level_changed_ = 0;

	remove_ghost_objects();
	ghost_objects_.clear();

	std::string data;
	variant lvl_node = lvl_->write();
	std::map<variant,variant> attr = lvl_node.as_map();
	attr.erase(variant("cycle"));  //levels saved in the editor should never
	                               //have a cycle attached to them so that
								   //all levels start at cycle 0.
	lvl_node = variant(&attr);
	std::cerr << "GET LEVEL FILENAME: " << filename_ << "\n";
	if(preferences::is_level_path_set()) {
		sys::write_file(preferences::level_path() + filename_, lvl_node.write_json(true));
	} else {
		sys::write_file(loadlevel::get_level_path(filename_), lvl_node.write_json(true));
	}

	//see if we should write the next/previous levels also
	//based on them having changed.
	if(lvl_->previous_level().empty() == false) {
		try {
			level_ptr prev(new level(lvl_->previous_level()));
			prev->finish_loading();
			if(prev->next_level() != lvl_->id()) {
				prev->set_next_level(lvl_->id());
				if(preferences::is_level_path_set()) {
					sys::write_file(preferences::level_path() + prev->id(), prev->write().write_json(true));
				} else {
					sys::write_file(module::map_file(prev->id()), prev->write().write_json(true));
				}
			}
		} catch(...) {
		}
	}

	if(lvl_->next_level().empty() == false) {
		try {
			level_ptr next(new level(lvl_->next_level()));
			next->finish_loading();
			if(next->previous_level() != lvl_->id()) {
				next->set_previous_level(lvl_->id());
				if(preferences::is_level_path_set()) {
					sys::write_file(preferences::level_path() + next->id(), next->write().write_json(true));
				} else {
					sys::write_file(module::map_file(next->id()), next->write().write_json(true));
				}
			}
		} catch(...) {
		}
	}

	toggle_active_level();
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
	graphics::prepare_raster();
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(zoom_ == 1) {
		//backgrounds only draw nicely at the regular zoom level for now.
		lvl_->draw_background(xpos_, ypos_, 0);
	}

	lvl_->draw(xpos_, ypos_, graphics::screen_width()*zoom_, graphics::screen_height()*zoom_);
	
	draw_gui();

	debug_console::draw();

	SDL_GL_SwapBuffers();
}

void editor::draw_gui() const
{
	glPushMatrix();
	glScalef(1.0/zoom_, 1.0/zoom_, 0);
	glTranslatef(-xpos_,-ypos_,0);

	const bool ctrl_pressed = (SDL_GetModState()&(KMOD_LCTRL|KMOD_RCTRL)) != 0;
	int mousex, mousey;
	get_mouse_state(mousex, mousey);
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
			all_characters()[cur_object_].preview_frame->draw(e.x(), e.y(), face_right_, upside_down_);
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
	if(property_dialog_ && property_dialog_.get() == current_dialog_ &&
	   property_dialog_->get_entity() &&
	   property_dialog_->get_entity()->editor_info() &&
	   std::count(lvl_->get_chars().begin(), lvl_->get_chars().end(),
	              property_dialog_->get_entity())) {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		//number of variables seen of each type, used to
		//cycle through colors for each variable type.
		std::map<editor_variable_info::VARIABLE_TYPE, int> nseen_variables;

		int selected_index = -1;
		const editor_variable_info* selected_var = variable_info_selected(property_dialog_->get_entity(), xpos_ + mousex*zoom_, ypos_ + mousey*zoom_, zoom_, &selected_index);
		foreach(const editor_variable_info& var, property_dialog_->get_entity()->editor_info()->vars()) {
			const std::string& name = var.variable_name();
			const editor_variable_info::VARIABLE_TYPE type = var.type();
			const int color_index = nseen_variables[type]++;
			variant value = property_dialog_->get_entity()->query_value(name);
			graphics::color color;
			switch(color_index) {
				case 0: color = graphics::color(255, 0, 0, 255); break;
				case 1: color = graphics::color(0, 255, 0, 255); break;
				case 2: color = graphics::color(0, 0, 255, 255); break;
				case 3: color = graphics::color(255, 255, 0, 255); break;
				default:color = graphics::color(255, 0, 255, 255); break;
			}

			if(&var == selected_var) {
				glColor4ub(255, 255, 0, 255);
			} else {
				glColor4ub(color.r(), color.g(), color.b(), color.a());
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
				case editor_variable_info::TYPE_POINTS:
					if(value.is_list()) {
						std::vector<variant> items = value.as_list();

						int index = 0;
						foreach(const variant& item, items) {
							point p(item);
							graphics::color col = color;
							if(&var == selected_var && index == selected_index) {
								col = graphics::color(255, 255, 0, 255);
							}

							graphics::draw_rect(rect(p.x, p.y-10, 1, 20), col);
							graphics::draw_rect(rect(p.x-10, p.y, 20, 1), col);

							graphics::blit_texture(font::render_text(formatter() << (index+1), col.as_sdl_color(), 12), p.x+4, p.y-14);
							++index;
						}
					}
					break;
				default:
					break;
			}

			if(!varray.empty()) {
				glVertexPointer(2, GL_FLOAT, 0, &varray.front());
				glDrawArrays(GL_LINES, 0, varray.size()/2);
			}
		}
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
	}

	if(g_draw_stats) {
//		stats::draw_stats(stats_);
	}

	glPopMatrix();

	//draw the difficulties of segments.
	if(lvl_->segment_width() > 0 || lvl_->segment_height() > 0) {
		const int seg_width = lvl_->segment_width() ? lvl_->segment_width() : lvl_->boundaries().w();
		const int seg_height = lvl_->segment_height() ? lvl_->segment_height() : lvl_->boundaries().h();
		rect boundaries = modify_selected_rect(*this, lvl_->boundaries(), selectx, selecty);
		int seg = 0;
		for(int ypos = boundaries.y(); ypos < boundaries.y2(); ypos += seg_height) {
			const int y1 = ypos/zoom_;
			for(int xpos = boundaries.x(); xpos < boundaries.x2(); xpos += seg_width) {
				const int difficulty = lvl_->get_var(formatter() << "segment_difficulty_start_" << seg).as_int();
//				if(difficulty) {
					graphics::blit_texture(font::render_text(formatter() << "Difficulty: " << difficulty, graphics::color_white(), 14), (xpos - xpos_)/zoom_, y1 - 20 - ypos_/zoom_);
//				}
			
				++seg;
			}
		}
	}

	//draw grid
	if(g_draw_grid){
	   glDisable(GL_TEXTURE_2D);
	   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	   varray.clear();
	   glColor4ub(255, 255, 255, 64);
	   for(int x = -TileSize - (xpos_/zoom_)%TileSize; x < graphics::screen_width(); x += 32/zoom_) {
		   varray.push_back(x); varray.push_back(0);
		   varray.push_back(x); varray.push_back(graphics::screen_height());
	   }
	}   

	for(int y = -TileSize - (ypos_/zoom_)%TileSize; y < graphics::screen_height(); y += 32/zoom_) {
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
		rect boundaries = modify_selected_rect(*this, lvl_->boundaries(), selectx, selecty);
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

		if(lvl_->segment_width() > 0) {
			for(int xpos = boundaries.x() + lvl_->segment_width(); xpos < boundaries.x2(); xpos += lvl_->segment_width()) {
				varray.push_back((xpos - xpos_)/zoom_);
				varray.push_back(y1 - ypos_/zoom_);
				varray.push_back((xpos - xpos_)/zoom_);
				varray.push_back(y2 - ypos_/zoom_);
				normal_color.add_to_vector(&carray);
				normal_color.add_to_vector(&carray);
			}
		}

		if(lvl_->segment_height() > 0) {
			for(int ypos = boundaries.y() + lvl_->segment_height(); ypos < boundaries.y2(); ypos += lvl_->segment_height()) {
				varray.push_back(x1 - xpos_/zoom_);
				varray.push_back((ypos - ypos_)/zoom_);
				varray.push_back(x2 - xpos_/zoom_);
				varray.push_back((ypos - ypos_)/zoom_);
				normal_color.add_to_vector(&carray);
				normal_color.add_to_vector(&carray);
			}
		}
		
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

	if(tool() == TOOL_EDIT_SEGMENTS && selected_segment_ >= 0) {
		rect area = rect(lvl_->boundaries().x() + selected_segment_*lvl_->segment_width(), lvl_->boundaries().y() + selected_segment_*lvl_->segment_height(),
		lvl_->segment_width() ? lvl_->segment_width() : lvl_->boundaries().w(),
		lvl_->segment_height() ? lvl_->segment_height() : lvl_->boundaries().h());
		area = rect((area.x() - xpos_)/zoom_, (area.y() - ypos_)/zoom_,
		            area.w()/zoom_, area.h()/zoom_);
		graphics::draw_rect(area, graphics::color(255, 255, 0, 64));

		variant next = lvl_->get_var(formatter() << "segments_after_" << selected_segment_);
		if(next.is_list()) {
			for(int n = 0; n != next.num_elements(); ++n) {
				const int segment = next[n].as_int();
				rect area = rect(lvl_->boundaries().x() + segment*lvl_->segment_width(), lvl_->boundaries().y() + segment*lvl_->segment_height(),
				lvl_->segment_width() ? lvl_->segment_width() : lvl_->boundaries().w(),
				lvl_->segment_height() ? lvl_->segment_height() : lvl_->boundaries().h());
				area = rect((area.x() - xpos_)/zoom_, (area.y() - ypos_)/zoom_,
				            area.w()/zoom_, area.h()/zoom_);
				graphics::draw_rect(area, graphics::color(255, 0, 0, 64));
			}
		}
	}
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);

	ASSERT_INDEX_INTO_VECTOR(cur_object_, enemy_types);

	//the location of the mouse cursor in the map
	char loc_buf[256];
	sprintf(loc_buf, "%d,%d", xpos_ + mousex*zoom_, ypos_ + mousey*zoom_);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	graphics::texture xtex = font::render_text(formatter() << (xpos_ + mousex*zoom_) << ",", graphics::color_white(), 14);
	graphics::texture ytex = font::render_text(formatter() << (ypos_ + mousey*zoom_), graphics::color_white(), 14);

	graphics::blit_texture(xtex, 10, 80);
	graphics::blit_texture(ytex, 10 + xtex.width(), 80);
	if(!code_dialog_ && current_dialog_) {
		current_dialog_->draw();
	}

	if(!code_dialog_ && layers_dialog_) {
		layers_dialog_->draw();
	}

	editor_menu_dialog_->draw();

	if(!code_dialog_) {
		editor_mode_dialog_->draw();
	}

	if(code_dialog_) {
		code_dialog_->draw();
	}

	gui::draw_tooltip();
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
#ifndef SDL_VIDEO_OPENGL_ES
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1, stipple_mask);
#endif
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
#ifndef SDL_VIDEO_OPENGL_ES
	glDisable(GL_LINE_STIPPLE);
	glLineStipple(1, 0xFFFF);
#endif
}

void editor::run_script(const std::string& id)
{
	editor_script::execute(id, *this);
}

void editor::execute_command(boost::function<void()> command, boost::function<void()> undo, EXECUTABLE_COMMAND_TYPE type)
{
	level_changed_++;

	command();

	executable_command cmd;
	cmd.redo_command = command;
	cmd.undo_command = undo;
	cmd.type = type;
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

void show_object_editor_dialog(const std::string& obj_type);

void launch_object_editor(const std::vector<std::string>& args);

void editor::edit_level_properties()
{
	editor_dialogs::editor_level_properties_dialog prop_dialog(*this);
	prop_dialog.show_modal();
}

void editor::create_new_module()
{
	editor_dialogs::editor_module_properties_dialog prop_dialog(*this);
	prop_dialog.show_modal();
	if(prop_dialog.cancelled() == false) {
		prop_dialog.on_exit();
		close();
		g_last_edited_level() = prop_dialog.on_exit();
		SDL_WM_SetCaption(module::get_module_pretty_name().c_str(), module::get_module_pretty_name().c_str());
	}
}

void editor::edit_module_properties()
{
	editor_dialogs::editor_module_properties_dialog prop_dialog(*this, module::get_module_name());
	prop_dialog.show_modal();
	if(prop_dialog.cancelled() == false) {
		prop_dialog.on_exit();
		SDL_WM_SetCaption(module::get_module_pretty_name().c_str(), module::get_module_pretty_name().c_str());
	}
}

namespace {
void do_draw_scene() {
	draw_scene(level::current(), last_draw_position());
}
}

void editor::create_new_object()
{
	editor_dialogs::custom_object_dialog object_dialog(*this, 
		preferences::virtual_screen_width()*0.05, 
		preferences::virtual_screen_height()*0.05, 
		preferences::virtual_screen_width()*0.9, 
		preferences::virtual_screen_height()*0.9);
	object_dialog.set_background_frame("empty_window");
	object_dialog.set_draw_background_fn(gui::dialog::draw_last_scene);
	object_dialog.show_modal();
	if(object_dialog.cancelled() == false) {
		customobjecttype::reload_file_paths();
		lvl_->editor_clear_selection();
		change_tool(TOOL_ADD_OBJECT);
		const std::string type = object_dialog.get_object()["id"].as_string();
		const_custom_object_type_ptr obj = custom_object_type::get(type);

		if(obj->editor_info()) {
			enemy_types.push_back(editor::enemy_type(*obj));
			current_dialog_ = character_dialog_.get();

			for(int n = 0; n != all_characters().size(); ++n) {
				const enemy_type& c = all_characters()[n];
				if(c.node["type"].as_string() == type) {
					character_dialog_->select_category(c.category);
					character_dialog_->set_character(n);
				}
			}
		}
	}
}


void editor::add_multi_object_to_level(level_ptr lvl, entity_ptr e)
{
	current_level_scope scope(lvl.get());
	lvl->add_multi_player(e);
	e->handle_event("editor_added");
}

void editor::add_object_to_level(level_ptr lvl, entity_ptr e)
{
	current_level_scope scope(lvl.get());
	lvl->add_character(e);
	e->handle_event("editor_added");
}

void editor::remove_object_from_level(level_ptr lvl, entity_ptr e)
{
	current_level_scope scope(lvl.get());
	e->handle_event("editor_removed");
	lvl->remove_character(e);
	lvl->set_active_chars();
}

void editor::mutate_object_value(level_ptr lvl, entity_ptr e, const std::string& value, variant new_value)
{
	current_level_scope scope(lvl.get());
	e->handle_event("editor_changing_variable");
	e->mutate_value(value, new_value);
	e->handle_event("editor_changed_variable");
}

void editor::generate_mutate_commands(entity_ptr c, const std::string& attr, variant new_value, std::vector<boost::function<void()> >& undo, std::vector<boost::function<void()> >& redo)
{
	if(!c || c->spawned_by().empty() == false) {
		return;
	}

	foreach(level_ptr lvl, levels_) {
		entity_ptr obj = lvl->get_entity_by_label(c->label());
		if(!obj) {
			continue;
		}
		const game_logic::formula_callable* obj_vars = obj->query_value("vars").as_callable();
		variant current_value = obj_vars->query_value(attr);

		redo.push_back(boost::bind(&editor::mutate_object_value, this, lvl, obj, attr, new_value));
		undo.push_back(boost::bind(&editor::mutate_object_value, this, lvl, obj, attr, current_value));
	}
}

void editor::generate_remove_commands(entity_ptr c, std::vector<boost::function<void()> >& undo, std::vector<boost::function<void()> >& redo)
{
	if(!c || c->spawned_by().empty() == false) {
		return;
	}
	
	foreach(level_ptr lvl, levels_) {
		entity_ptr obj = lvl->get_entity_by_label(c->label());
		if(!obj) {
			continue;
		}

		redo.push_back(boost::bind(&editor::remove_object_from_level, this, lvl, obj));
		undo.push_back(boost::bind(&editor::add_object_to_level, this, lvl, obj));
		if(obj->label().empty() == false) {
			foreach(entity_ptr child, lvl->get_chars()) {
				if(child->spawned_by() == obj->label()) {
		std::cerr << "REMOVING CHILD OBJECT: " << child->debug_description() << " " << child->label() << "\n";
					redo.push_back(boost::bind(&editor::remove_object_from_level, this, lvl, child));
					undo.push_back(boost::bind(&editor::add_object_to_level, this, lvl, child));
				}
			}
		}
	}
}

bool editor::has_keyboard_focus() const
{
	if(code_dialog_ && code_dialog_->has_keyboard_focus()) {
		return true;
	}

	if(current_dialog_ && current_dialog_->has_focus()) {
		return true;
	}

	return false;
}

void editor::toggle_code()
{
	if(external_code_editor_ && external_code_editor_->replace_in_game_editor()) {

		std::string type;
		if(lvl_->editor_selection().empty() == false) {
			type = lvl_->editor_selection().back()->query_value("type").as_string();
		}

		if(type.empty()) {
			std::cerr << "no object selected to open code for\n";
		} else {
			//if this is a nested type, convert it to their parent type.
			std::string::iterator dot_itor = std::find(type.begin(), type.end(), '.');
			type.erase(dot_itor, type.end());

			const std::string* path = custom_object_type::get_object_path(type + ".cfg");
			ASSERT_LOG(path, "Could not find path for object " << type);
			std::cerr << "Loading file in external editor: " << *path << "\n";
			external_code_editor_->load_file(*path);
		}

		return;
	}

	if(code_dialog_) {
		code_dialog_.reset();
	} else {
		code_dialog_.reset(new code_editor_dialog(rect(graphics::screen_width() - 620, 30, 620, graphics::screen_height() - 30)));
		set_code_file();
	}
}

void editor::set_code_file()
{
	if(tool_ == TOOL_ADD_RECT || tool_ == TOOL_SELECT_RECT || tool_ == TOOL_MAGIC_WAND || tool_ == TOOL_PENCIL) {
		std::cerr << "SET TILESET..\n";
		if(cur_tileset_ >= 0 && cur_tileset_ < tilesets.size()) {
			const std::vector<std::string>& files = tile_map::get_files(tilesets[cur_tileset_].type);
			std::cerr << "TILESET: " << files.size() << " FOR " << tilesets[cur_tileset_].type << "\n";
			foreach(const std::string& file, files) {
				std::map<std::string, std::string> fnames;
				module::get_unique_filenames_under_dir("data/tiles", &fnames);
				const std::map<std::string, std::string>::const_iterator itor =
				  module::find(fnames, file);
				if(itor != fnames.end() && code_dialog_) {
					std::cerr << "TILESET FNAME: " << itor->second << "\n";
					code_dialog_->load_file(itor->second);
				}
			}
		}

		return;
	}
	
	std::string type;
	if(lvl_->editor_selection().empty() == false) {
		type = lvl_->editor_selection().back()->query_value("type").as_string();
	} else if(lvl_->player()) {
		type = lvl_->player()->get_entity().query_value("type").as_string();
	}

	if(type.empty()) {
		return;
	}

	if(std::count(type.begin(), type.end(), '.')) {
		//it's a subtype, so find the parent.
		type = std::string(type.begin(), std::find(type.begin(), type.end(), '.'));
	}

	const std::string* path = custom_object_type::get_object_path(type + ".cfg");
	if(path && code_dialog_) {
		code_dialog_->load_file(*path);
	}
}

void editor::start_adding_points(const std::string& field_name)
{
	adding_points_ = field_name;

	if(property_dialog_) {
		property_dialog_->init();
	}
}
#endif // !NO_EDITOR

