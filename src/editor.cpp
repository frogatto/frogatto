#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "button.hpp"
#include "character.hpp"
#include "character_editor_dialog.hpp"
#include "character_type.hpp"
#include "draw_tile.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "frame.hpp"
#include "grid_widget.hpp"
#include "item.hpp"
#include "key.hpp"
#include "label.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "load_level.hpp"
#include "prop.hpp"
#include "property_editor_dialog.hpp"
#include "prop_editor_dialog.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "tile_map.hpp"
#include "tileset_editor_dialog.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
const char* ModeStrings[] = {"Tiles", "Objects", "Items", "Groups", "Properties", "Variations", "Props"};
}

class editor_mode_dialog : public gui::dialog
{
	editor& editor_;
	gui::widget_ptr context_menu_;

	void select_mode(int mode)
	{
		if(mode >= 0 && mode < editor::NUM_MODES) {
			editor_.change_mode(mode);
		}

		remove_widget(context_menu_);
		context_menu_.reset();
		init();
	}

	void show_menu()
	{
		using namespace gui;
		gui::grid* grid = new gui::grid(1);
		grid->set_show_background(true);
		grid->allow_selection();
		grid->register_selection_callback(boost::bind(&editor_mode_dialog::select_mode, this, _1));
		for(int n = 0; n != editor::NUM_MODES; ++n) {
			grid->add_col(widget_ptr(new label(ModeStrings[n], graphics::color_white())));
		}

		int mousex, mousey;
		SDL_GetMouseState(&mousex, &mousey);

		mousex -= x();
		mousey -= y();

		remove_widget(context_menu_);
		context_menu_.reset(grid);
		add_widget(context_menu_, mousex, mousey);
	}

	bool handle_event(const SDL_Event& event, bool claimed)
	{
		if(!claimed) {
			switch(event.type) {
			case SDL_KEYDOWN: {
				editor::EDIT_MODE mode = editor::NUM_MODES;
				switch(event.key.keysym.sym) {
				case SDLK_t:
					mode = editor::EDIT_TILES;
					break;
				case SDLK_o:
					mode = editor::EDIT_PROPS;
					break;
				case SDLK_v:
					mode = editor::EDIT_VARIATIONS;
					break;
				case SDLK_c:
					mode = editor::EDIT_CHARS;
					break;
				}

				if(mode < editor::NUM_MODES) {
					editor_.change_mode(mode);
					init();
					claimed = true;
				}
				break;
			}
			}
		}

		return claimed || dialog::handle_event(event, claimed);
	}
public:
	explicit editor_mode_dialog(editor& e)
	  : gui::dialog(640, 0, 160, 40), editor_(e)
	{
		init();
	}

	void init()
	{
		clear();
		using namespace gui;
		button* b = new button(widget_ptr(new label(ModeStrings[editor_.mode()], graphics::color_white())), boost::bind(&editor_mode_dialog::show_menu, this));
		add_widget(widget_ptr(b), 5, 5);
	}
};

namespace {

const int TileSize = 32;

std::vector<editor::tileset> tilesets;

std::vector<editor::enemy_type> enemy_types;

struct placeable_item {
	static void init(wml::const_node_ptr node);
	explicit placeable_item(wml::const_node_ptr node);
	wml::const_node_ptr node;
	const_item_type_ptr type;
};

std::vector<placeable_item> placeable_items;

void placeable_item::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("item");
	wml::node::const_child_iterator i2 = node->end_child("item");
	for(; i1 != i2; ++i1) {
		placeable_items.push_back(placeable_item(i1->second));
	}

	std::cerr << "ITEMS: " << placeable_items.size() << "\n";
}

placeable_item::placeable_item(wml::const_node_ptr node)
  : node(node), type(item_type::get((*node)["type"]))
{}

entity_ptr selected_entity;
int selected_property = 0;

std::vector<const_prop_ptr> all_props;

bool foreground_mode = false;
int foreground_zorder_change() {
	return foreground_mode ? 1500 : 0;
}

}

void editor::enemy_type::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("character");
	wml::node::const_child_iterator i2 = node->end_child("character");
	for(; i1 != i2; ++i1) {
		enemy_types.push_back(editor::enemy_type(i1->second));
	}
}

editor::enemy_type::enemy_type(wml::const_node_ptr node)
  : node(node), category(node->attr("category")),
    preview_frame(&entity::build(node)->current_frame())
{}

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
    zorder(wml::get_int(node, "zorder")), sloped(wml::get_bool(node, "sloped"))
{
	if(node->get_child("preview")) {
		preview.reset(new tile_map(node->get_child("preview")));
	}
}

editor::editor(const char* level_cfg)
  : xpos_(0), ypos_(0), anchorx_(0), anchory_(0),
    selected_entity_startx_(0), selected_entity_starty_(0),
    filename_(level_cfg), mode_(EDIT_TILES), done_(false), face_right_(true),
    cur_tileset_(0),
    cur_item_(0),
    current_dialog_(NULL),
	drawing_rect_(false)
{
	editor_mode_dialog_.reset(new editor_mode_dialog(*this));

	static bool first_time = true;
	if(first_time) {
		wml::const_node_ptr editor_cfg = wml::parse_wml(sys::read_file("editor.cfg"));
		tileset::init(editor_cfg);
		enemy_type::init(editor_cfg);
		placeable_item::init(editor_cfg);
		all_props = prop::all_props();
		first_time = false;
	}

	assert(!tilesets.empty());
	lvl_.reset(new level(level_cfg));
	lvl_->finish_loading();
	lvl_->set_editor();
}

editor::~editor()
{
}

void editor::edit_level()
{
	tileset_dialog_.reset(new editor_dialogs::tileset_editor_dialog(*this));
	current_dialog_ = tileset_dialog_.get();

	glEnable(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int selected_tile = 0;

	select_previous_level_ = false;
	select_next_level_ = false;
	while(!done_) {
		int mousex, mousey;
		const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);

		if(buttons == 0) {
			drawing_rect_ = false;
		}

		const int selectx = (xpos_ + mousex)/TileSize;
		const int selecty = (ypos_ + mousey)/TileSize;

		if(mode_ == EDIT_PROPERTIES && !buttons) {
			lvl_->set_editor_selection(lvl_->get_character_at_point(xpos_ + mousex, ypos_ + mousey));
		} else if(mode_ != EDIT_PROPERTIES) {
			lvl_->set_editor_selection(entity_ptr());
		} else if(lvl_->editor_selection()) {
			const int dx = xpos_ + mousex - anchorx_;
			const int dy = ypos_ + mousey - anchory_;
			const int xpos = selected_entity_startx_ + dx;
			const int ypos = selected_entity_starty_ + dy;

			lvl_->editor_selection()->set_pos(xpos - xpos%TileSize, ypos - ypos%TileSize);
		}

		const int ScrollSpeed = 8;

		const bool ctrl = key_[SDLK_LCTRL] || key_[SDLK_RCTRL];

		if(!ctrl) {
			if(key_[SDLK_LEFT]) {
				xpos_ -= ScrollSpeed;
			}

			if(key_[SDLK_RIGHT]) {
				xpos_ += ScrollSpeed;
			}

			if(key_[SDLK_UP]) {
				ypos_ -= ScrollSpeed;
			}

			if(key_[SDLK_DOWN]) {
				ypos_ += ScrollSpeed;
			}
		} else {
			const rect& bounds = lvl_->boundaries();
			if(key_[SDLK_LEFT]) {
				if(key_[SDLK_LEFT] && bounds.w() > TileSize) {
					lvl_->set_boundaries(rect(bounds.x(), bounds.y(), bounds.w() - TileSize, bounds.h()));
				}
			}

			if(key_[SDLK_RIGHT]) {
				lvl_->set_boundaries(rect(bounds.x(), bounds.y(), bounds.w() + TileSize, bounds.h()));
			}

			if(key_[SDLK_UP] && bounds.h() > TileSize) {
				lvl_->set_boundaries(rect(bounds.x(), bounds.y(), bounds.w(), bounds.h() - TileSize));
			}

			if(key_[SDLK_DOWN]) {
				lvl_->set_boundaries(rect(bounds.x(), bounds.y(), bounds.w(), bounds.h() + TileSize));
			}
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(editor_mode_dialog_->process_event(event, false)) {
				continue;
			}

			if(current_dialog_ && current_dialog_->process_event(event, false)) {
				continue;
			}
			
			switch(event.type) {
			case SDL_QUIT:
				done_ = true;
				break;
			case SDL_KEYDOWN:
				std::cerr << "keydown " << (int)event.key.keysym.sym << " vs " << (int)SDLK_LEFT << "\n";
				if(event.key.keysym.sym == SDLK_ESCAPE) {
					return;
				}

				if(mode_ == EDIT_PROPERTIES &&
				   selected_entity &&
				   event.key.keysym.sym >= SDLK_1 &&
				   event.key.keysym.sym <= SDLK_9) {
					const int num = event.key.keysym.sym - SDLK_1;
					game_logic::formula_callable* vars = selected_entity->vars();
					if(vars) {
						std::vector<game_logic::formula_input> inputs;
						vars->get_inputs(&inputs);
						if(num < inputs.size()) {
							selected_property = num;
						}
					}
				}

				if(mode_ == EDIT_PROPERTIES && selected_entity &&
				   (event.key.keysym.sym == SDLK_COMMA ||
				    event.key.keysym.sym == SDLK_PERIOD)) { 
					int increment = (event.key.keysym.sym == SDLK_COMMA ? -1 : 1) * ((event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1);

					game_logic::formula_callable* vars = selected_entity->vars();
					if(vars) {
						std::vector<game_logic::formula_input> inputs;
						vars->get_inputs(&inputs);
						if(selected_property < inputs.size()) {
							const std::string& key = inputs[selected_property].name;
							vars->mutate_value(key, vars->query_value(key) + variant(increment));
						}
					}
				}

				if(event.key.keysym.sym == SDLK_s) {
					const std::string path = "data/level/";
					std::string data;
					wml::write(lvl_->write(), data);
					sys::write_file(path + filename_, data);

					//see if we should write the next/previous levels also
					//based on them having changed.
					if(lvl_->previous_level().empty() == false) {
						level prev(lvl_->previous_level());
						if(prev.next_level() != lvl_->id()) {
							prev.set_next_level(lvl_->id());
							std::string data;
							wml::write(prev.write(), data);
							sys::write_file(path + prev.id(), data);
						}
					}

					if(lvl_->next_level().empty() == false) {
						level next(lvl_->next_level());
						if(next.previous_level() != lvl_->id()) {
							next.set_previous_level(lvl_->id());
							std::string data;
							wml::write(next.write(), data);
							sys::write_file(path + next.id(), data);
						}
					}
				}

				if(event.key.keysym.sym == SDLK_f &&
				   (event.key.keysym.mod&KMOD_CTRL)) {
					//set to edit foregrounds.
					foreground_mode = !foreground_mode;
				} else if(event.key.keysym.sym == SDLK_f) {
					face_right_ = !face_right_;
				}

				if(event.key.keysym.sym == SDLK_i &&
				   placeable_items.empty() == false) {
					change_mode(EDIT_ITEMS);
				}

				if(event.key.keysym.sym == SDLK_g) {
					change_mode(EDIT_GROUPS);
				}

				if(event.key.keysym.sym == SDLK_p) {
					change_mode(EDIT_PROPERTIES);
				}

				if(event.key.keysym.sym == SDLK_r &&
				   (event.key.keysym.mod&KMOD_CTRL)) {
					tile_map::init(wml::parse_wml(sys::read_file("tiles.cfg")));
					lvl_->rebuild_tiles();
				}

				if(event.key.keysym.sym == SDLK_COMMA) {
					switch(mode_) {
					case EDIT_TILES:
						set_tileset(cur_tileset_-1);
						break;
					case EDIT_CHARS:
						--cur_item_;
						if(cur_item_ < 0) {
							cur_item_ = enemy_types.size()-1;
						}
						break;
					case EDIT_ITEMS:
						--cur_item_;
						if(cur_item_ < 0) {
							cur_item_ = placeable_items.size()-1;
						}
						break;
					case EDIT_PROPS:
						--cur_item_;
						if(cur_item_ < 0) {
							cur_item_ = all_props.size()-1;
						}
						break;
					}
				}

				if(event.key.keysym.sym == SDLK_PERIOD) {
					switch(mode_) {
					case EDIT_TILES:
						set_tileset(cur_tileset_+1);
						break;
					case EDIT_CHARS:
						++cur_item_;
						if(cur_item_ == enemy_types.size()) {
							cur_item_ = 0;
						}
						break;
					case EDIT_ITEMS:
						++cur_item_;
						if(cur_item_ == placeable_items.size()) {
							cur_item_ = 0;
						}
						break;
					case EDIT_PROPS:
						++cur_item_;
						if(cur_item_ == all_props.size()) {
							cur_item_ = 0;
						}
						break;
					}
				}

				break;
			case SDL_MOUSEBUTTONDOWN:
				anchorx_ = xpos_ + mousex;
				anchory_ = ypos_ + mousey;

				if(mode_ != EDIT_PROPERTIES) {
					drawing_rect_ = true;
				} else if(property_dialog_) {
					property_dialog_->set_entity(lvl_->editor_selection());
				}

				if(lvl_->editor_selection()) {
					selected_entity_startx_ = lvl_->editor_selection()->x();
					selected_entity_starty_ = lvl_->editor_selection()->y();
				}

				if(select_previous_level_) {

					std::vector<std::string> levels = get_known_levels();
					assert(!levels.empty());
					levels.push_back("");
					int index = std::find(levels.begin(), levels.end(), lvl_->previous_level()) - levels.begin();
					index = (index + 1)%levels.size();
					if(levels[index] == lvl_->id()) {
						index = (index + 1)%levels.size();
					}

					lvl_->set_previous_level(levels[index]);

				} else if(select_next_level_) {

					std::vector<std::string> levels = get_known_levels();
					assert(!levels.empty());
					levels.push_back("");
					int index = std::find(levels.begin(), levels.end(), lvl_->next_level()) - levels.begin();
					index = (index + 1)%levels.size();
					if(levels[index] == lvl_->id()) {
						index = (index + 1)%levels.size();
					}

					lvl_->set_next_level(levels[index]);

				} else if(mode_ == EDIT_CHARS && event.button.button == SDL_BUTTON_LEFT) {
					wml::node_ptr node(wml::deep_copy(enemy_types[cur_item_].node));
					node->set_attr("x", formatter() << (anchorx_ - anchorx_%TileSize));
					node->set_attr("y", formatter() << (anchory_ - anchory_%TileSize));
					node->set_attr("face_right", face_right_ ? "yes" : "no");

					wml::node_ptr vars = node->get_child("vars");
					entity_ptr c(entity::build(node));
					if(vars) {
						std::map<std::string, std::string> attr;
						for(wml::node::const_attr_iterator i = vars->begin_attr();
						    i != vars->end_attr(); ++i) {
							game_logic::formula_ptr f = game_logic::formula::create_string_formula(i->second);
							if(f) {
								attr[i->first] = f->execute(*c).as_string();
							}
						}

						for(std::map<std::string, std::string>::const_iterator i = attr.begin(); i != attr.end(); ++i) {
							vars->set_attr(i->first, i->second);
						}

						c = entity::build(node);
					}
					lvl_->add_character(c);
				} else if(mode_ == EDIT_ITEMS && event.button.button == SDL_BUTTON_LEFT) {
					wml::node_ptr node(wml::deep_copy(placeable_items[cur_item_].node));
					node->set_attr("x", formatter() << (anchorx_ - anchorx_%TileSize));
					node->set_attr("y", formatter() << (anchory_ - anchory_%TileSize));
					item_ptr i(new item(node));
					lvl_->add_item(i);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if(mode_ == EDIT_TILES) {
					if(!drawing_rect_) {
						//wasn't drawing a rect; ignore.
					} else if(event.button.button == SDL_BUTTON_LEFT) {
						lvl_->add_tile_rect(tilesets[cur_tileset_].zorder + foreground_zorder_change(), anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey, tilesets[cur_tileset_].type);
					} else if(event.button.button == SDL_BUTTON_RIGHT) {
						lvl_->clear_tile_rect(anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey);
					}
				} else if(mode_ == EDIT_CHARS || mode_ == EDIT_ITEMS) {
					if(event.button.button == SDL_BUTTON_RIGHT) {
						lvl_->remove_characters_in_rect(anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey);
					}
				} else if(mode_ == EDIT_GROUPS && drawing_rect_) {
					std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect::from_coordinates(anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey));
					const int group = lvl_->add_group();
					foreach(entity_ptr c, chars) {
						lvl_->set_character_group(c, group);
					}
				} else if(mode_ == EDIT_PROPERTIES && drawing_rect_) {
					std::vector<entity_ptr> chars = lvl_->get_characters_in_rect(rect::from_coordinates(anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey));
					if(chars.empty() == false) {
						selected_entity = chars.front();
					}
				} else if(mode_ == EDIT_VARIATIONS) {
					const int xtile = (xpos_ + mousex)/TileSize;
					const int ytile = (ypos_ + mousey)/TileSize;
					lvl_->flip_variations(xtile, ytile);
				} else if(mode_ == EDIT_PROPS) {
					if(event.button.button == SDL_BUTTON_RIGHT && drawing_rect_) {
						lvl_->remove_props_in_rect(anchorx_, anchory_, xpos_ + mousex, ypos_ + mousey);
					} else if(event.button.button == SDL_BUTTON_LEFT) {
						const int xtile = (xpos_ + mousex)/TileSize;
						const int ytile = (ypos_ + mousey)/TileSize;
						prop_object obj(xtile*TileSize, ytile*TileSize, all_props[cur_item_]->id());
						obj.set_zorder(obj.zorder() + foreground_zorder_change());
						lvl_->add_prop(obj);
					}
				}

				drawing_rect_ = false;
				break;
			default:
				break;
			}
		}

		draw();
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

const std::vector<const_prop_ptr>& editor::get_props() const
{
	return all_props;
}

void editor::set_tileset(int index)
{
	cur_tileset_ = index;
	if(cur_tileset_ < 0) {
		cur_tileset_ = tilesets.size()-1;
	} else if(cur_tileset_ >= tilesets.size()) {
		cur_tileset_ = 0;
	}
}

void editor::set_item(int index)
{
	int max = 0;
	switch(mode_) {
	case EDIT_TILES:
		max = all_tilesets().size();
		break;
	case EDIT_CHARS:
		max = enemy_types.size();
		break;
	case EDIT_ITEMS:
		max = placeable_items.size();
		break;
	case EDIT_GROUPS:
		break;
	case EDIT_PROPERTIES:
		break;
	case EDIT_VARIATIONS:
		break;
	case EDIT_PROPS:
		max = all_props.size();
		break;
	}

	if(index < 0) {
		index = max - 1;
	} else if(index >= max) {
		index = 0;
	}

	cur_item_ = index;
}

void editor::change_mode(int nmode)
{
	cur_item_ = 0;
	mode_ = static_cast<EDIT_MODE>(nmode);
	switch(mode_) {
	case EDIT_TILES:
		tileset_dialog_.reset(new editor_dialogs::tileset_editor_dialog(*this));
		current_dialog_ = tileset_dialog_.get();
		break;
	case EDIT_CHARS:
		character_dialog_.reset(new editor_dialogs::character_editor_dialog(*this));
		current_dialog_ = character_dialog_.get();
		cur_item_ = 0;
		break;
	case EDIT_ITEMS:
		break;
	case EDIT_GROUPS:
		break;
	case EDIT_PROPERTIES:
		property_dialog_.reset(new editor_dialogs::property_editor_dialog(*this));
		current_dialog_ = property_dialog_.get();
		cur_item_ = 0;
		break;
	case EDIT_VARIATIONS:
		break;
	case EDIT_PROPS:
		prop_dialog_.reset(new editor_dialogs::prop_editor_dialog(*this));
		current_dialog_ = prop_dialog_.get();
		cur_item_ = 0;
		break;
	}
}

void editor::draw() const
{
	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	graphics::prepare_raster();
	lvl_->draw_background(0,0,0);
	glPushMatrix();
	glTranslatef(-xpos_,-ypos_,0);

	lvl_->draw(xpos_, ypos_, graphics::screen_width(), graphics::screen_height());


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
	int x = -t.width();
	int y = ypos_ + graphics::screen_height()/2;

	select_next_level_ = select_previous_level_ = false;

	const int selectx = xpos_ + mousex;
	const int selecty = ypos_ + mousey;
	if(selectx > x && selectx < 0 && selecty > y && selecty < y + t.height()) {
		t = font::render_text(previous_level, graphics::color_yellow(), 24);
		select_previous_level_ = true;
	}

	graphics::blit_texture(t, x, y);
	t = font::render_text(next_level, graphics::color_black(), 24);
	x = lvl_->boundaries().x2();
	if(selectx > x && selectx < x + t.width() && selecty > y && selecty < y + t.height()) {
		t = font::render_text(next_level, graphics::color_yellow(), 24);
		select_next_level_ = true;
	}
	graphics::blit_texture(t, x, y);
	}

	if(mode_ == EDIT_PROPS) {
		const int x = ((xpos_ + mousex)/TileSize)*TileSize;
		const int y = ((ypos_ + mousey)/TileSize)*TileSize;
		glColor4f(1.0, 1.0, 1.0, 0.5);
		all_props[cur_item_]->get_frame().draw(x, y, true);
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}

	if(drawing_rect_) {
		int x1 = anchorx_;
		int x2 = xpos_ + mousex;
		if(x1 > x2) {
			std::swap(x1,x2);
		}

		int y1 = anchory_;
		int y2 = ypos_ + mousey;
		if(y1 > y2) {
			std::swap(y1,y2);
		}

		const SDL_Rect rect = {x1, y1, x2 - x1, y2 - y1};
		const SDL_Color color = {255,255,255,255};
		graphics::draw_hollow_rect(rect, color);
	}
	glPopMatrix();

	//draw grid
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4ub(255, 255, 255, 64);
	for(int x = TileSize - xpos_%TileSize; x < graphics::screen_width(); x += 32) {
		glVertex3f(x, 0, 0);
		glVertex3f(x, graphics::screen_height(), 0);
	}

	for(int y = TileSize - ypos_%TileSize; y < graphics::screen_height(); y += 32) {
		glVertex3f(0, y, 0);
		glVertex3f(graphics::screen_width(), y, 0);
	}
	glColor4ub(255, 255, 255, 255);
	
	// draw level boundaries in clear white
	{
		const int x1 = lvl_->boundaries().x();
		const int x2 = lvl_->boundaries().x2();
		const int y1 = lvl_->boundaries().y();
		const int y2 = lvl_->boundaries().y2();
		glVertex3f(x1 - xpos_, y1 - ypos_, 0);
		glVertex3f(x2 - xpos_, y1 - ypos_, 0);

		glVertex3f(x1 - xpos_, y1 - ypos_, 0);
		glVertex3f(x1 - xpos_, y2 - ypos_, 0);

		glVertex3f(x2 - xpos_, y1 - ypos_, 0);
		glVertex3f(x2 - xpos_, y2 - ypos_, 0);

		glVertex3f(x1 - xpos_, y2 - ypos_, 0);
		glVertex3f(x2 - xpos_, y2 - ypos_, 0);
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);

	if(mode_ == EDIT_TILES) {
	} else if(mode_ == EDIT_CHARS) {
		ASSERT_INDEX_INTO_VECTOR(cur_item_, enemy_types);
	} else if(mode_ == EDIT_ITEMS) {

	} else if(mode_ == EDIT_PROPS) {

	} else if(mode_ == EDIT_PROPERTIES && selected_entity) {
		game_logic::formula_callable* vars = selected_entity->vars();
		if(vars) {
			std::vector<game_logic::formula_input> inputs;
			vars->get_inputs(&inputs);
			for(int n = 0; n != inputs.size(); ++n) {
				std::ostringstream s;
				s << "(" << (n+1) << ") " << inputs[n].name << ": " << vars->query_value(inputs[n].name).to_debug_string();
				glColor4ub(255, 255, 255, selected_property == n ? 255 : 160);
				graphics::blit_texture(font::render_text(s.str(), graphics::color_black(), 14), 600, 20 + n*20);
				glColor4ub(255, 255, 255, 255);

				if(inputs[n].name == "x_bound" || inputs[n].name == "x2_bound") {
					glDisable(GL_TEXTURE_2D);
					glBegin(GL_LINES);
					glColor4ub(255,0,0,128);
					const int value = vars->query_value(inputs[n].name).as_int();
					const int pos = value - xpos_;
					glVertex3f(pos, 0, 0);
					glVertex3f(pos, graphics::screen_height(), 0);
					glColor4ub(255,255,255,255);
					glEnd();
					glEnable(GL_TEXTURE_2D);
				}

				if(inputs[n].name == "y_bound" || inputs[n].name == "y2_bound") {
					glDisable(GL_TEXTURE_2D);
					glBegin(GL_LINES);
					glColor4ub(255,0,0,128);
					const int value = vars->query_value(inputs[n].name).as_int();
					const int pos = value - ypos_;
					glVertex3f(0, pos, 0);
					glVertex3f(graphics::screen_width(), pos, 0);
					glColor4ub(255,255,255,255);
					glEnd();
					glEnable(GL_TEXTURE_2D);
				}

			}
		}
	}

	//the location of the mouse cursor in the map
	char loc_buf[256];
	sprintf(loc_buf, "%d,%d", xpos_ + mousex, ypos_ + mousey);
	graphics::blit_texture(font::render_text(loc_buf, graphics::color_yellow(), 14), 10, 10);

	if(foreground_mode) {
		graphics::blit_texture(font::render_text("(Foreground mode)", graphics::color_yellow(), 14), 10, 26);
	}

	if(current_dialog_) {
		current_dialog_->draw();
	}

	editor_mode_dialog_->draw();

	SDL_GL_SwapBuffers();
	SDL_Delay(20);
}
