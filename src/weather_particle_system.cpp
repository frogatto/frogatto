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
	for (int i = 0; i < 1500; i++)
	{
		particle new_p;
		new_p.pos[0] = rand()%repeat_period;
		new_p.pos[1] = rand()%repeat_period;
		new_p.velocity[0] = 0;
		new_p.velocity[1] = 5+(rand()%3);
		particles_.push_back(new_p);
	}
}

void weather_particle_system::process(const entity& e)
{
	++cycle_;
	
	foreach(particle& p, particles_)
	{
		p.pos[0] += p.velocity[0];
		p.pos[1] += p.velocity[1];
		p.pos[1] = static_cast<int>(p.pos[1]) % repeat_period;
	}
	
	//while (particles_.size() > 1500) particles_.pop_front();
}

void weather_particle_system::draw(const rect& area, const entity& e) const
{
	glDisable(GL_TEXTURE_2D);
	//glEnable(GL_SMOOTH);
	glLineWidth(1);
	glBegin(GL_LINES);
	glColor4f(0.75, 0.75, 1.0, 0.9);
	int offset_x = area.x()-(area.x()%repeat_period);
	int offset_y = area.y()-(area.y()%repeat_period);
	foreach(const particle& p, particles_)
	{
		float my_y = p.pos[1]+offset_y;
		do
		{
			float my_x = p.pos[0]+offset_x;
			do
			{
				glVertex3f(my_x, my_y, 0.0);
				glVertex3f(my_x, my_y+8.0, 0.0);
				my_x += repeat_period;
				//printf("my_x: %f, area.x: %i, area.w: %i\n", my_x, area.x(), area.w());
			} while (my_x < area.x()+area.w());
			my_y += repeat_period;
		} while (my_y < area.y()+area.h());
	}
	glEnd();
	//glDisable(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
}
