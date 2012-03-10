#include <algorithm>
#include <iostream>

#include "graphics.hpp"

#include "controls.hpp"
#include "draw_scene.hpp"
#include "entity.hpp"
#include "gui_section.hpp"
#include "inventory.hpp"
#include "powerup.hpp"
#include "raster.hpp"
#include "texture.hpp"

void show_inventory(const level& lvl, entity& c)
{
	const_gui_section_ptr selector = gui_section::get("powerup_selector");
	if(!selector) {
		std::cerr << "ERROR: selector frame not found\n";
		return;
	}

	const_powerup_ptr empty_powerup = powerup::get("empty");
	const int NumPowerupSlots = 6;
	int selected_powerup = 0;
	for(;;) {

		std::vector<const_powerup_ptr> powerups = c.powerups();
		powerups.erase(std::unique(powerups.begin(), powerups.end()), powerups.end());
		std::reverse(powerups.begin(), powerups.end());
		while(empty_powerup && powerups.size() < NumPowerupSlots) {
			powerups.push_back(empty_powerup);
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_SPACE) {
					return;
				}
				if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_UP)) {
					selected_powerup = (selected_powerup + 9)%NumPowerupSlots;
					break;
				}
				if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_DOWN)) {
					selected_powerup = (selected_powerup + 3)%NumPowerupSlots;
					break;
				}
				if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_LEFT)) {
					selected_powerup--;
					if(selected_powerup == 2) {
						selected_powerup = 5;
					} else if(selected_powerup == -1) {
						selected_powerup = 2;
					}
					break;
				}
				if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_RIGHT)) {
					selected_powerup++;
					if(selected_powerup == 3) {
						selected_powerup = 0;
					} else if(selected_powerup == 6) {
						selected_powerup = 3;
					}
					break;
				}
				if(event.key.keysym.sym == get_sdlkey(controls::CONTROL_JUMP)) {
					//move the selected powerup to the front.
					assert(selected_powerup >= 0 && selected_powerup < powerups.size());
					const_powerup_ptr powerup = powerups[selected_powerup];
					if(powerup != empty_powerup) {
						const int count = c.remove_powerup(powerup);
						for(int n = 0; n != count; ++n) {
							c.get_powerup(powerup);
						}

						selected_powerup = 0;
					}
					break;
				}
				break;
			}
		}

		graphics::prepare_raster();

		graphics::texture inventory(graphics::texture::get("gui/inventory.png"));

		const int InventoryWidth = 400;
		const int InventoryHeight = 240;
		graphics::blit_texture(inventory, 0, 0,
		                       InventoryWidth*2, InventoryHeight*2, 0, 0, 0,
		                       GLfloat(InventoryWidth)/inventory.width(),
		                       GLfloat(InventoryHeight)/inventory.height());

		for(int n = 0; n != powerups.size(); ++n) {
			const int xpos = 208 + (n%3)*46;
			const int ypos = 36 + (n/3)*66;

			if(selected_powerup == n) {
				selector->blit(xpos - 2, ypos - 1);
			}

			powerups[n]->icon().draw(xpos, ypos);

			//draw the number of items we have. This is probably temporary
			//code that'll be superceded later when we have a nice text
			//drawing system.
			const int count = std::count(c.powerups().begin(), c.powerups().end(), powerups[n]);
			if(!count) {
				continue;
			}

			const int width = 9;
			const int height = 9;
			const int x1 = 173 + width*count;
			const int x2 = x1 + width;
			const int y1 = 313;
			const int y2 = y1 + height;
			graphics::blit_texture(inventory, xpos + 18, ypos + 38, width*2, height*2, 0.0,
			                       GLfloat(x1)/inventory.width(),
			                       GLfloat(y1)/inventory.height(),
			                       GLfloat(x2)/inventory.width(),
			                       GLfloat(y2)/inventory.height());
		}

		std::vector<const_powerup_ptr> abilities = c.abilities();
		for(int n = 0; n != abilities.size(); ++n) {
			const int xpos = 200 + n*46;
			const int ypos = 183;
			abilities[n]->icon().draw(xpos, ypos);
		}
		
		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}
}
