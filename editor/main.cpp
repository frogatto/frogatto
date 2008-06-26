#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <iostream>

#include <boost/shared_ptr.hpp>

#include "character_type.hpp"
#include "draw_tile.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "tile_map.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {

struct tileset {
	static void init(wml::const_node_ptr node);
	explicit tileset(wml::const_node_ptr node);
	std::string type;
	int zorder;
	boost::shared_ptr<tile_map> preview;
};

std::vector<tileset> tilesets;
int cur_tileset = 0;

void tileset::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("tileset");
	wml::node::const_child_iterator i2 = node->end_child("tileset");
	for(; i1 != i2; ++i1) {
		tilesets.push_back(tileset(i1->second));
	}
}

tileset::tileset(wml::const_node_ptr node) : type(node->attr("type")), zorder(wml::get_int(node, "zorder"))
{
	if(node->get_child("preview")) {
		preview.reset(new tile_map(node->get_child("preview")));
	}
}

}

int main(int argc, char** argv)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	if(SDL_SetVideoMode(1024,768,0,SDL_OPENGL) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	const SDL_Surface* fb = SDL_GetVideoSurface();
	if(fb == NULL) {
		return 0;
	}

	const char* level_cfg = "level.cfg";
	if(argc == 2) {
		level_cfg = argv[1];
	}

	graphics::texture::manager texture_manager;

	character_type::init(wml::parse_wml(sys::read_file("characters.cfg")));
	level_object::init(wml::parse_wml(sys::read_file("tiles.cfg")));
	tile_map::init(wml::parse_wml(sys::read_file("tiles.cfg")));
	tileset::init(wml::parse_wml(sys::read_file("editor.cfg")));
	assert(!tilesets.empty());
	level lvl(wml::parse_wml(sys::read_file(level_cfg)));

	glEnable(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const int TileSize = 32;

	std::vector<level_tile> tileset_preview;

	std::vector<const_level_object_ptr> tiles_list = level_object::all();
	int selected_tile = 0;
	bool face_right = true;

	CKey key;
	bool done = false;
	int xpos = 0, ypos = 0;
	int zorder = 0;
	int anchorx = 0, anchory = 0;
	while(!done) {
		if(tileset_preview.empty()) {
			assert(cur_tileset < tilesets.size());
			if(tilesets[cur_tileset].preview) {
				tilesets[cur_tileset].preview->build_tiles(&tileset_preview);
			}
		}

		int mousex, mousey;
		const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);

		const int selectx = (xpos + mousex)/TileSize;
		const int selecty = (ypos + mousey)/TileSize;

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				done = true;
				break;
			case SDL_KEYDOWN:
				std::cerr << "keydown " << (int)event.key.keysym.sym << " vs " << (int)SDLK_LEFT << "\n";
				if(event.key.keysym.sym == SDLK_LEFT) {
					xpos -= TileSize;
				}

				if(event.key.keysym.sym == SDLK_RIGHT) {
					xpos += TileSize;
				}

				if(event.key.keysym.sym == SDLK_UP) {
					ypos -= TileSize;
				}

				if(event.key.keysym.sym == SDLK_DOWN) {
					ypos += TileSize;
				}

				if(event.key.keysym.sym == SDLK_COMMA) {
					--selected_tile;
					selected_tile = selected_tile%tiles_list.size();
				}

				if(event.key.keysym.sym == SDLK_PERIOD) {
					++selected_tile;
					selected_tile = selected_tile%tiles_list.size();
				}

				if(event.key.keysym.sym == SDLK_s) {
					std::string data;
					wml::write(lvl.write(), data);
					sys::write_file(level_cfg, data);
				}

				if(event.key.keysym.sym == SDLK_f) {
					face_right = !face_right;
				}

				if(event.key.keysym.sym == SDLK_COMMA) {
					--cur_tileset;
					if(cur_tileset < 0) {
						cur_tileset = tilesets.size()-1;
					}
					tileset_preview.clear();
				}

				if(event.key.keysym.sym == SDLK_PERIOD) {
					++cur_tileset;
					if(cur_tileset == tilesets.size()) {
						cur_tileset = 0;
					}

					std::cerr << "change tileset to " << tilesets[cur_tileset].type << "\n";
					tileset_preview.clear();
				}

				if(event.key.keysym.sym == SDLK_g) {
					const level_tile* t = lvl.get_tile_at(xpos + mousex, ypos + mousey);
					if(t) {
						std::vector<const_level_object_ptr>::const_iterator i = std::find(tiles_list.begin(), tiles_list.end(), t->object);
						if(i != tiles_list.end()) {
							selected_tile = i - tiles_list.begin();
						}
					}
				}

				break;
			case SDL_MOUSEBUTTONDOWN:
				anchorx = xpos + mousex;
				anchory = ypos + mousey;
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.button == SDL_BUTTON_LEFT) {
					lvl.add_tile_rect(tilesets[cur_tileset].zorder, anchorx, anchory, xpos + mousex, ypos + mousey, tilesets[cur_tileset].type);
				} else if(event.button.button == SDL_BUTTON_RIGHT) {
					for(int zorder = -1; zorder <= 1; ++zorder) {
						lvl.add_tile_rect(zorder, anchorx, anchory, xpos + mousex, ypos + mousey, "");
					}
				}
				break;
			default:
				break;
			}
		}

		graphics::prepare_raster();
		lvl.draw_background(0,0);
		glPushMatrix();
		glTranslatef(-xpos,-ypos,0);

		if(zorder >= 0) {
			lvl.draw(xpos, ypos, graphics::screen_width(), graphics::screen_height());
		}

		if(selected_tile >= 0 && selected_tile < tiles_list.size()) {
			level_tile tmp_tile;
			tmp_tile.x = selectx*TileSize;
			tmp_tile.y = selecty*TileSize;
			tmp_tile.object = tiles_list[selected_tile];
			tmp_tile.zorder = zorder;
			tmp_tile.face_right = face_right;
			draw_tile(tmp_tile);
		}

		if(zorder < 0) {
			lvl.draw(xpos, ypos, graphics::screen_width(), graphics::screen_height());
		}

		if(buttons) {
			int x1 = anchorx;
			int x2 = xpos + mousex;
			if(x1 > x2) {
				std::swap(x1,x2);
			}

			int y1 = anchory;
			int y2 = ypos + mousey;
			if(y1 > y2) {
				std::swap(y1,y2);
			}

			const SDL_Rect rect = {x1, y1, x2 - x1, y2 - y1};
			const SDL_Color color = {255,255,255,255};
			graphics::draw_hollow_rect(rect, color);
		}
		glPopMatrix();

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glColor4ub(255, 255, 255, 64);
		for(int x = 0; x < graphics::screen_width(); x += 32) {
			glVertex3f(x, 0, 0);
			glVertex3f(x, graphics::screen_height(), 0);
		}

		for(int y = 0; y < graphics::screen_height(); y += 32) {
			glVertex3f(0, y, 0);
			glVertex3f(graphics::screen_width(), y, 0);
		}
		glColor4ub(255, 255, 255, 255);
		glEnd();
		glEnable(GL_TEXTURE_2D);

		foreach(const level_tile& t, tileset_preview) {
			level_object::draw(t);
		}

		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}

	SDL_Quit();
}
