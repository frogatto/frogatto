#include <cstdio>
#include <math.h>
#include <vector>

#include "preferences.hpp"
#include "variant.hpp"
#include "water_particle_system.hpp"

water_particle_system_info::water_particle_system_info(variant node)
: number_of_particles(node["number_of_particles"].as_int(1500)),
repeat_period(node["repeat_period"].as_int(1000)),
velocity_x(node["velocity_x"].as_int()),
velocity_y(node["velocity_y"].as_int(-5)),
velocity_rand(node["velocity_rand"].as_int(3)),
dot_size(node["dot_size"].as_int(1)*(preferences::double_scale() ? 2 : 1))
{
	irgba = graphics::color(node["color"]).value();

	if(dot_size > 1 && preferences::xypos_draw_mask) {
		//if we are clipping our drawing granularity, then we have a small
		//enough screen that we want to shrink the particles.
		dot_size /= 2;
	}
}

water_particle_system_factory::water_particle_system_factory(variant node)
 : info(node)
{
	
}

particle_system_ptr water_particle_system_factory::create(const entity& e) const
{
	return particle_system_ptr(new water_particle_system(e, *this));
}


water_particle_system::water_particle_system(const entity& e, const water_particle_system_factory& factory)
 : factory_(factory), info_(factory.info), velocity_x_(factory.info.velocity_x), velocity_y_(factory.info.velocity_y), cycle_(0)
{
	area_ = rect("0,0,1,1");
	base_velocity = sqrtf(info_.velocity_x*info_.velocity_x + info_.velocity_y*info_.velocity_y);
	direction[0] = velocity_x_ / base_velocity;
	direction[1] = velocity_y_ / base_velocity;
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

void water_particle_system::process(const entity& e)
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

	glColor4f(info_.rgba[0]/255.0, info_.rgba[1]/255.0, info_.rgba[2]/255.0, info_.rgba[3]/255.0);
#if defined(USE_GLES2)
	gles2::manager gles2_manager;
	glUniform1f(gles2_manager.pt_size, info_.dot_size);
	glVertexAttribPointer(gles2_manager.vtx_coord, 2, GL_SHORT, GL_FALSE, 0, &vertices.front());
	glEnableVertexAttribArray(gles2_manager.vtx_coord);
	glDrawArrays(GL_POINTS, 0, vertices.size()/2);
#else
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPointSize(info_.dot_size);

	glVertexPointer(2, GL_SHORT, 0, &vertices.front());
	glDrawArrays(GL_POINTS, 0, vertices.size()/2);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
#endif
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
	} else if(key=="velocity_x") {
		velocity_x_ = value.as_int();
		direction[0] = velocity_x_ / base_velocity;
		direction[1] = velocity_y_ / base_velocity;

	} else if(key=="velocity_y") {
		velocity_y_ = value.as_int();
		direction[0] = velocity_x_ / base_velocity;
		direction[1] = velocity_y_ / base_velocity;

	}
}
