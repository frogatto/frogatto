#include <cstdio>
#include <math.h>

#include "weather_particle_system.hpp"
#include "wml_utils.hpp"

weather_particle_system_factory::weather_particle_system_factory (wml::const_node_ptr node)
 : info(node)
{
	
}

particle_system_ptr weather_particle_system_factory::create(const entity& e) const
{
	return particle_system_ptr(new weather_particle_system(e, *this));
}


weather_particle_system::weather_particle_system(const entity& e, const weather_particle_system_factory& factory)
 : factory_(factory), info_(factory.info), cycle_(0)
{
	base_velocity = sqrtf(info_.velocity_x*info_.velocity_x + info_.velocity_y*info_.velocity_y);
	direction[0] = info_.velocity_x / base_velocity;
	direction[1] = info_.velocity_y / base_velocity;
	for (int i = 0; i < info_.number_of_particles; i++)
	{
		particle new_p;
		new_p.pos[0] = rand()%info_.repeat_period;
		new_p.pos[1] = rand()%info_.repeat_period;
		new_p.velocity = base_velocity + (info_.velocity_rand ? (rand() % info_.velocity_rand) : 0);
		particles_.push_back(new_p);
	}
}

void weather_particle_system::process(const level& lvl, const entity& e)
{
	++cycle_;
	
	foreach(particle& p, particles_)
	{
		p.pos[0] = static_cast<int>(p.pos[0]+direction[0] * p.velocity) % info_.repeat_period;
		p.pos[1] = static_cast<int>(p.pos[1]+direction[1] * p.velocity) % info_.repeat_period;
	}
	
	//while (particles_.size() > 1500) particles_.pop_front();
}

void weather_particle_system::draw(const rect& area, const entity& e) const
{
	glDisable(GL_TEXTURE_2D);
	glLineWidth(info_.line_width);
	glBegin(GL_LINES);
	glColor4f(info_.rgba[0]/255.0, info_.rgba[1]/255.0, info_.rgba[2]/255.0, info_.rgba[3]/255.0);
	int offset_x = area.x() - area.x()%info_.repeat_period;
	if (area.x() < 0) offset_x -= info_.repeat_period;
	int offset_y = area.y() - area.y()%info_.repeat_period;
	if (area.y() < 0) offset_y -= info_.repeat_period;
	foreach(const particle& p, particles_)
	{
		float my_y = p.pos[1]+offset_y;
		do
		{
			float my_x = p.pos[0]+offset_x;
			do
			{
				glVertex3f(my_x, my_y, 0.0);
				glVertex3f(my_x+direction[0]*info_.line_length, my_y+direction[1]*info_.line_length, 0.0);
				my_x += info_.repeat_period;
				//printf("my_x: %f, area.x: %i, area.w: %i\n", my_x, area.x(), area.w());
			} while (my_x < area.x()+area.w());
			my_y += info_.repeat_period;
		} while (my_y < area.y()+area.h());
	}
	glEnd();
	//glDisable(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
}
