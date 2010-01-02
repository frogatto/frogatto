#include <cstdio>
#include <math.h>
#include <vector>

#include "preferences.hpp"
#include "water_particle_system.hpp"
#include "wml_utils.hpp"

water_particle_system_info::water_particle_system_info(wml::const_node_ptr node)
: number_of_particles(wml::get_int(node, "number_of_particles", 1500)),
repeat_period(wml::get_int(node, "repeat_period", 1000)),
velocity_x(wml::get_int(node, "velocity_x")),
velocity_y(wml::get_int(node, "velocity_y", -5)),
velocity_rand(wml::get_int(node, "velocity_rand", 3)),
dot_size(wml::get_int(node, "dot_size", 1))
{
	irgba = graphics::color(node->attr("color")).value();

	if(dot_size > 1 && preferences::xypos_draw_mask) {
		//if we are clipping our drawing granularity, then we have a small
		//enough screen that we want to shrink the particles.
		dot_size /= 2;
	}
}

water_particle_system_factory::water_particle_system_factory (wml::const_node_ptr node)
 : info(node)
{
	
}

particle_system_ptr water_particle_system_factory::create(const entity& e) const
{
	return particle_system_ptr(new water_particle_system(e, *this));
}


water_particle_system::water_particle_system(const entity& e, const water_particle_system_factory& factory)
 : factory_(factory), info_(factory.info), cycle_(0)
{
	area_ = rect("0,0,1,1");
	base_velocity = sqrtf(info_.velocity_x*info_.velocity_x + info_.velocity_y*info_.velocity_y);
	direction[0] = info_.velocity_x / base_velocity;
	direction[1] = info_.velocity_y / base_velocity;
	particles_.reserve(info_.number_of_particles);
	for (int i = 0; i < info_.number_of_particles; i++)
	{
		particle new_p;
		new_p.pos[0] = rand()%info_.repeat_period;
		new_p.pos[1] = rand()%info_.repeat_period;
		new_p.velocity = base_velocity + (info_.velocity_rand ? (rand() % info_.velocity_rand) : 0);
		particles_.push_back(new_p);
	}
}

void water_particle_system::process(const level& lvl, const entity& e)
{
	++cycle_;
	
	foreach(particle& p, particles_)
	{
		p.pos[0] = static_cast<int>(p.pos[0]+direction[0] * p.velocity) % info_.repeat_period;
		p.pos[1] = static_cast<int>(p.pos[1]+direction[1] * p.velocity) % info_.repeat_period;
	}
	
	
	//while (particles_.size() > 1500) particles_.pop_front();
}

void water_particle_system::draw(const rect& screen_area, const entity& e) const
{
	const rect area = intersection_rect(screen_area, area_);
	if(area.w() == 0 || area.h() == 0) {
		return;
	}
	
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPointSize(info_.dot_size);
	glColor4f(info_.rgba[0]/255.0, info_.rgba[1]/255.0, info_.rgba[2]/255.0, info_.rgba[3]/255.0);
	int offset_x = area.x() - area.x()%info_.repeat_period;
	if (area.x() < 0) offset_x -= info_.repeat_period;
	int offset_y = area.y() - area.y()%info_.repeat_period;
	if (area.y() < 0) offset_y -= info_.repeat_period;
	static std::vector<GLshort> vertices;
	vertices.clear();
	foreach(const particle& p, particles_)
	{
		GLshort my_y = p.pos[1]+offset_y;
		GLshort xpos = p.pos[0]+offset_x;
		while(my_y < area_.y()) {
			my_y += info_.repeat_period;
		}

		while(xpos < area_.x()) {
			xpos += info_.repeat_period;
		}

		if(my_y > area_.y2() || xpos > area_.x2()) {
			continue;
		}

		while(my_y <= area.y2()) {
			GLshort my_x = xpos;

			while (my_x <= area.x2()) {
				vertices.push_back(my_x);
				vertices.push_back(my_y);
				my_x += info_.repeat_period;
			}
			my_y += info_.repeat_period;
		}
	}

	if(vertices.empty()) {
		return;
	}

	glVertexPointer(2, GL_SHORT, 0, &vertices.front());
	glDrawArrays(GL_POINTS, 0, vertices.size()/2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
}


void water_particle_system::set_value(const std::string& key, const variant& value)
{
	if(key == "area") {
		if(value.is_string()) {
			area_ = rect(value.as_string());
		} else if(value.is_list() && value.num_elements() == 4) {
			area_ = rect::from_coordinates(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int());
		}		
	}
}
