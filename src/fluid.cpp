#include <iostream>

#include "fluid.hpp"
#include "geometry.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
const int FluidTileSize = 32;
}

class fluid_tile;

class fluid_pixel {
public:
	fluid_pixel() : volume_(0), velocity_x_(0), velocity_y_(0),
	                processed_(false)
	{}

	unsigned char volume() const { return volume_; }

	void set_volume(unsigned char v) { volume_ = v; }

	void new_frame() { processed_ = false; }
	void process(int x, int y, level& lvl, fluid& f, fluid_tile* t);
private:
	unsigned char volume_;
	char velocity_x_, velocity_y_;
	bool processed_;
};

class fluid_tile {
public:
	fluid_tile(int base_x, int base_y)
	  : base_x_(base_x), base_y_(base_y), water_volume_(0)
	{}

	fluid_pixel& get_pixel(int x, int y) {
		x -= base_x_;
		y -= base_y_;
		return pixels_[y*FluidTileSize + x];
	}

	const fluid_pixel& get_pixel(int x, int y) const {
		x -= base_x_;
		y -= base_y_;
		return pixels_[y*FluidTileSize + x];
	}

	int base_x() const { return base_x_; }
	int base_y() const { return base_y_; }

	int volume() const { return water_volume_; }
	void change_volume(int amount) { water_volume_ += amount; }

	void draw();
private:
	fluid_pixel pixels_[FluidTileSize*FluidTileSize];
	int base_x_, base_y_;
	int water_volume_;
};

namespace {
int vacant_at(const level& lvl, fluid& f, int x, int y) {
	if(lvl.solid(x, y)) {
		return 0;
	}
	fluid_pixel* p = f.get_pixel(x, y);
	if(p == NULL) {
		return fluid::MaxPerPixel;
	}

	return fluid::MaxPerPixel - p->volume();
}
}

void fluid_pixel::process(int x, int y, level& lvl, fluid& f, fluid_tile* my_tile)
{
	if(processed_) {
		return;
	}

	processed_ = true;

	if(volume_ == 0) {
		return;
	}

	if(vacant_at(lvl, f, x, y + 1)) {
		++velocity_y_;
	}

	if(velocity_x_ == 0 && velocity_y_ == 0) {
		return;
	}


	int target_x = x + velocity_x_;
	int target_y = y + velocity_y_;
	while(volume_) {
		while(vacant_at(lvl, f, target_x, target_y) == 0) {
			if(target_x < x) {
				++target_x;
			} else if(target_x > x) {
				--target_x;
			}

			if(target_y < y) {
				++target_y;
			} else if(target_y > y) {
				--target_y;
			}

			if(target_x == x && target_y == y) {
				break;
			}
		}

		if(target_x == x && target_y == y) {
			break;
		}

		fluid_tile* target_tile = NULL;
		fluid_pixel* target = f.create_pixel(target_x, target_y, &target_tile);
		if(target == NULL) {
			break;
		}

		int transfer = fluid::MaxPerPixel - target->volume();
		if(transfer > volume_) {
			transfer = volume_;
		}

		if(target->volume_ == 0) {
			target->processed_ = true;
		}

		if(my_tile != target_tile) {
			my_tile->change_volume(-transfer);
			target_tile->change_volume(transfer);
		}
		target->volume_ += transfer;
		target->velocity_x_ += velocity_x_;
		target->velocity_y_ += velocity_y_;
		volume_ -= transfer;
	}

	velocity_x_ = 0;
	velocity_y_ = 0;
}

void fluid_tile::draw()
{
	if(water_volume_ == fluid::MaxPerPixel*FluidTileSize*FluidTileSize) {
		glBegin(GL_QUADS);
		glVertex3i(base_x_, base_y_, 0);
		glVertex3i(base_x_ + FluidTileSize, base_y_, 0);
		glVertex3i(base_x_ + FluidTileSize, base_y_ + FluidTileSize, 0);
		glVertex3i(base_x_, base_y_ + FluidTileSize, 0);
		glEnd();
		return;
	}

	glBegin(GL_POINTS);
	for(int y = 0; y != FluidTileSize; ++y) {
		for(int x = 0; x != FluidTileSize; ++x) {
			if(pixels_[y*FluidTileSize + x].volume()) {
				glVertex3i(base_x_ + x, base_y_ + y, 0);
			}
		}
	}
	glEnd();
}

fluid::fluid(int width, int height)
  : width_(width/FluidTileSize + ((width%FluidTileSize) ? 1 : 0)),
    height_(height/FluidTileSize + ((height%FluidTileSize) ? 1 : 0))
{
	tiles_.resize(width_*height_);
}

int fluid::fluids_at(int x, int y) const
{
	const int index = (y/FluidTileSize)*width_ + x/FluidTileSize;
	if(index < 0 || index >= tiles_.size() || tiles_[index] == NULL) {
		return 0;
	}

	return tiles_[index]->get_pixel(x, y).volume();
}

fluid_pixel* fluid::get_pixel(int x, int y)
{
	if(x < 0 || y < 0) {
		return NULL;
	}

	const int index = (y/FluidTileSize)*width_ + x/FluidTileSize;
	if(index < 0 || index >= tiles_.size() || tiles_[index] == NULL) {
		return NULL;
	}

	return &tiles_[index]->get_pixel(x, y);
}

fluid_pixel* fluid::create_pixel(int x, int y, fluid_tile** t)
{
	const int index = (y/FluidTileSize)*width_ + x/FluidTileSize;
	if(index < 0 || index >= tiles_.size()) {
		return NULL;
	}
	
	if(tiles_[index] == NULL) {
		std::cerr << "create fluid tile at " << index << "\n";
		tiles_[index] = new fluid_tile(x - x%FluidTileSize, y - y%FluidTileSize);
	}

	*t = tiles_[index];
	return &tiles_[index]->get_pixel(x, y);
}

void fluid::process(level& lvl)
{
	for(std::vector<fluid_tile*>::iterator i = tiles_.begin(); i != tiles_.end(); ++i) {
		if(*i == NULL || (*i)->volume() == 0) {
			continue;
		}

		for(int y = 0; y != FluidTileSize; ++y) {
			for(int x = 0; x != FluidTileSize; ++x) {
				(*i)->get_pixel(x, y).new_frame();
			}
		}
	}

	for(std::vector<fluid_tile*>::iterator i = tiles_.begin(); i != tiles_.end(); ++i) {
		if(*i == NULL || (*i)->volume() == 0) {
			continue;
		}

		const int x1 = (*i)->base_x();
		const int y1 = (*i)->base_y();
		const int x2 = x1 + FluidTileSize;
		const int y2 = y1 + FluidTileSize;
		for(int y = y1; y != y2; ++y) {
			for(int x = x1; x != x2; ++x) {
				(*i)->get_pixel(x, y).process(x, y, lvl, *this, *i);
			}
		}
	}
}

void fluid::draw(int x, int y, int w, int h)
{
	glColor4f(0.0, 0.0, 1.0, 0.5);
	for(int ypos = y; ypos < y + h; ypos += FluidTileSize) {
		for(int xpos = x; xpos < x + w; xpos += FluidTileSize) {
			const int index = (ypos/FluidTileSize)*width_ + xpos/FluidTileSize;
			if(index < 0 || index >= tiles_.size() || tiles_[index] == NULL || tiles_[index]->volume() == 0) {
				continue;
			}

			tiles_[index]->draw();
		}
	}
	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void fluid::read(wml::const_node_ptr node)
{
	wml::node::const_child_iterator r1 = node->begin_child("rect");
	wml::node::const_child_iterator r2 = node->end_child("rect");
	for(; r1 != r2; ++r1) {
		rect r(r1->second->attr("rect"));
		for(int y = r.y(); y != r.y2(); ++y) {
			for(int x = r.x(); x != r.x2(); ++x) {
				fluid_tile* t = NULL;
				fluid_pixel* p = create_pixel(x, y, &t);
				if(p) {
					p->set_volume(MaxPerPixel);
					t->change_volume(MaxPerPixel);
				}
			}
		}
	}
}

wml::node_ptr fluid::write() const
{
	wml::node_ptr node(new wml::node("fluid"));
	return node;
}
