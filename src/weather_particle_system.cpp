#include "weather_particle_system.hpp"
#include <cstdio>

weather_particle_system_factory::weather_particle_system_factory (wml::const_node_ptr node)
{
	
}

particle_system_ptr weather_particle_system_factory::create(const entity& e) const
{
	return particle_system_ptr(new weather_particle_system(e, *this));
}


weather_particle_system::weather_particle_system(const entity& e, const weather_particle_system_factory& factory)
 : factory_(factory), cycle_(0)
{
	
}

void weather_particle_system::process(const entity& e)
{
	++cycle_;
	
	for (int i = 0; i < 10; i++)
	{
		particle new_p;
		new_p.pos[0] = e.x()+(rand()%1000);
		new_p.pos[1] = e.y();
		new_p.velocity[0] = 0;
		new_p.velocity[1] = 3+(rand()%5);
		particles_.push_back(new_p);
	}
	
	foreach(particle& p, particles_)
	{
		p.pos[0] += p.velocity[0];
		p.pos[1] += p.velocity[1];
	}
	
	if (particles_.size() > 300) particles_.pop_front();
}

void weather_particle_system::draw(const rect& area, const entity& e) const
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4f(0.75, 0.75, 1.0, 0.9);
	foreach(const particle& p, particles_)
	{
		//printf("Drawing a particle at %f:%f\n", p.pos[0], p.pos[1]);
		glVertex3f(p.pos[0], p.pos[1], 0.0);
		glVertex3f(p.pos[0], p.pos[1]+8.0, 0.0);
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
}
