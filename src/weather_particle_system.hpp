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
#ifndef WEATHER_PARTICLE_SYSTEM_HPP_INCLUDED
#define WEATHER_PARTICLE_SYSTEM_HPP_INCLUDED

#include "graphics.hpp"

#include <deque>

#include "particle_system.hpp"
#include "foreach.hpp"
#include "entity.hpp"
#include "color_utils.hpp"
#include "variant.hpp"

struct weather_particle_system_info {
	weather_particle_system_info(variant node)
	: number_of_particles(node["number_of_particles"].as_int(1500)),
	repeat_period(node["repeat_period"].as_int(1000)),
	velocity_x(node["velocity_x"].as_int()),
	velocity_y(node["velocity_y"].as_int(5)),
	velocity_rand(node["velocity_rand"].as_int(3)),
	line_width(node["line_width"].as_int(1)),
	line_length(node["line_length"].as_int(8))
	{
		irgba = graphics::color(node["color"].as_string()).value();
	}
	int number_of_particles;
	int repeat_period;
	int velocity_x, velocity_y;
	int velocity_rand;
	int line_width, line_length;
	
	union {
		uint8_t rgba[4];
		uint32_t irgba;
	};
};

class weather_particle_system_factory : public particle_system_factory {
public:
	explicit weather_particle_system_factory(variant node);
	~weather_particle_system_factory() {}
	
	particle_system_ptr create(const entity& e) const;
	weather_particle_system_info info;
};

class weather_particle_system : public particle_system
{
public:
	weather_particle_system(const entity& e, const weather_particle_system_factory& factory);
	
	bool is_destroyed() const { return false; }
	void process(const entity& e);
	void draw(const rect& area, const entity& e) const;
	
private:
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);
	
	const weather_particle_system_factory& factory_;
	const weather_particle_system_info& info_;
	
	int cycle_;
	
	struct particle {
		GLfloat pos[2];
		GLfloat velocity;
	};
	
	GLfloat direction[2];
	GLfloat base_velocity;
	
	std::vector<particle> particles_;
};

#endif
