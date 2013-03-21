/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef water_PARTICLE_SYSTEM_H
#define water_PARTICLE_SYSTEM_H

#include "graphics.hpp"

#include <deque>

#include "color_utils.hpp"
#include "foreach.hpp"
#include "geometry.hpp"
#include "entity.hpp"
#include "particle_system.hpp"
#include "variant.hpp"

struct water_particle_system_info {
	water_particle_system_info(variant node);

	int number_of_particles;
	int repeat_period;
	int velocity_x, velocity_y;
	int velocity_rand;
	int dot_size;
	
	union {
		uint8_t rgba[4];
		uint32_t irgba;
	};
};

class water_particle_system_factory : public particle_system_factory {
public:
	explicit water_particle_system_factory(variant node);
	~water_particle_system_factory() {}
	
	particle_system_ptr create(const entity& e) const;
	water_particle_system_info info;
};

class water_particle_system : public particle_system
{
public:
	water_particle_system(const entity& e, const water_particle_system_factory& factory);
	
	bool is_destroyed() const { return false; }
	void process(const entity& e);
	void draw(const rect& area, const entity& e) const;
	
private:
	variant get_value(const std::string& key) const { return variant(); }
	void set_value(const std::string& key, const variant& value);	
	
	const water_particle_system_factory& factory_;
	const water_particle_system_info& info_;
	
	int cycle_;
	
	rect area_;
	
	struct particle {
		GLfloat pos[2];
		GLfloat velocity;
	};
	
	GLfloat direction[2];
	GLfloat base_velocity;
	int velocity_x_, velocity_y_;
	
	
	std::vector<particle> particles_;
};

#endif
