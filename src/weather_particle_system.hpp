#ifndef WEATHER_PARTICLE_SYSTEM_H
#define WEATHER_PARTICLE_SYSTEM_H

#include <GL/gl.h>

#include <deque>

#include "particle_system.hpp"
#include "foreach.hpp"
#include "entity.hpp"

class weather_particle_system_factory : public particle_system_factory {
public:
	explicit weather_particle_system_factory(wml::const_node_ptr node);
	~weather_particle_system_factory() {}
	
	particle_system_ptr create(const entity& e) const;
};

class weather_particle_system : public particle_system
{
public:
	weather_particle_system(const entity& e, const weather_particle_system_factory& factory);
	
	bool is_destroyed() const { return false; }
	void process(const entity& e);
	void draw(const rect& area, const entity& e) const;
	
private:
	static const int repeat_period = 1000;
	variant get_value(const std::string& key) const { return variant(); }
	
	const weather_particle_system_factory& factory_;
	
	int cycle_;
	
	struct particle {
		GLfloat pos[2];
		GLfloat velocity[2];
	};
	
	std::deque<particle> particles_;
};

#endif