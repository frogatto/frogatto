#include <iostream>
#include <deque>
#include <inttypes.h>
#include <math.h>

#include "asserts.hpp"
#include "color_utils.hpp"
#include "entity.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "particle_system.hpp"
#include "texture.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {

class particle_animation {
public:
	explicit particle_animation(wml::const_node_ptr node) :
	  id_(node->attr("id")),
	  texture_(graphics::texture::get(node->attr("image"))),
	  duration_(wml::get_int(node, "duration"))
	{
		rect base_area(node->has_attr("rect") ? rect(node->attr("rect")) :
	           rect(wml::get_int(node, "x"),
	                wml::get_int(node, "y"),
	                wml::get_int(node, "w", texture_.width()),
	                wml::get_int(node, "h", texture_.height())));
		width_ = base_area.w();
		height_ = base_area.h();
		int nframes = wml::get_int(node, "frames", 1);
		if(nframes < 1) {
			nframes = 1;
		}

		const int nframes_per_row = wml::get_int(node, "frames_per_row", -1);
		const int pad = wml::get_int(node, "pad");

		int row = 0, col = 0;
		for(int n = 0; n != nframes; ++n) {
			rect area(base_area.x() + col*(base_area.w()+pad),
			          base_area.y() + row*(base_area.h()+pad),
					  base_area.w(), base_area.h());

			frame_area a;
			a.u1 = GLfloat(area.x())/GLfloat(texture_.width());
			a.u2 = GLfloat(area.x2())/GLfloat(texture_.width());
			a.v1 = GLfloat(area.y())/GLfloat(texture_.height());
			a.v2 = GLfloat(area.y2())/GLfloat(texture_.height());

			frames_.push_back(a);

			++col;
			if(col == nframes_per_row) {
				col = 0;
				++row;
			}
		}
	}

	struct frame_area {
		GLfloat u1, v1, u2, v2;
	};

	const frame_area& get_frame(int t) const {
		int index = t/duration_;
		if(index < 0) {
			index = 0;
		} else if(index >= frames_.size()) {
			index = frames_.size() - 1;
		}

		return frames_[index];
	}

	void set_texture() const {
		texture_.set_as_current_texture();
	}

	int width() const { return width_; }
	int height() const { return height_; }
private:
	std::string id_;
	graphics::texture texture_;

	std::vector<frame_area> frames_;
	int duration_;
	int width_, height_;
};

struct simple_particle_system_info {
	simple_particle_system_info(wml::const_node_ptr node)
	  : spawn_rate_(wml::get_int(node, "spawn_rate", 1)),
	    spawn_rate_random_(wml::get_int(node, "spawn_rate_random")),
	    system_time_to_live_(wml::get_int(node, "system_time_to_live", -1)),
	    time_to_live_(wml::get_int(node, "time_to_live", 50)),
	    min_x_(wml::get_int(node, "min_x", 0)),
	    max_x_(wml::get_int(node, "max_x", 0)),
	    min_y_(wml::get_int(node, "min_y", 0)),
	    max_y_(wml::get_int(node, "max_y", 0)),
		velocity_x_(wml::get_int(node, "velocity_x", 0)),
		velocity_y_(wml::get_int(node, "velocity_y", 0)),
		velocity_x_rand_(wml::get_int(node, "velocity_x_random", 0)),
		velocity_y_rand_(wml::get_int(node, "velocity_y_random", 0)),
		velocity_magnitude_(wml::get_int(node, "velocity_magnitude", 0)),
		velocity_magnitude_rand_(wml::get_int(node, "velocity_magnitude_random", 0)),
		velocity_rotate_(wml::get_int(node, "velocity_rotate", 0)),
		velocity_rotate_rand_(wml::get_int(node, "velocity_rotate_random", 0)),
		accel_x_(wml::get_int(node, "accel_x", 0)),
		accel_y_(wml::get_int(node, "accel_y", 0)),
		delta_r_(wml::get_int(node, "delta_r", 0)),
		delta_g_(wml::get_int(node, "delta_g", 0)),
		delta_b_(wml::get_int(node, "delta_b", 0)),
		delta_a_(wml::get_int(node, "delta_a", 0))
	{
		irgba_ = graphics::color(node->attr("color")).value();
	}
	int spawn_rate_, spawn_rate_random_;
	int system_time_to_live_;
	int time_to_live_;
	int min_x_, max_x_, min_y_, max_y_;
	int velocity_x_, velocity_y_;
	int velocity_x_rand_, velocity_y_rand_;
	int velocity_magnitude_, velocity_magnitude_rand_;
	int velocity_rotate_, velocity_rotate_rand_;
	int accel_x_, accel_y_;

	union {
		uint8_t rgba_[4];
		uint32_t irgba_;
	};

	int delta_r_, delta_g_, delta_b_, delta_a_;
};

class simple_particle_system_factory : public particle_system_factory {
public:
	explicit simple_particle_system_factory(wml::const_node_ptr node);
	~simple_particle_system_factory() {}

	particle_system_ptr create(const entity& e) const;

	std::vector<particle_animation> frames_;

	simple_particle_system_info info_;
};

simple_particle_system_factory::simple_particle_system_factory(wml::const_node_ptr node)
  : info_(node)
{
	FOREACH_WML_CHILD(frame_node, node, "animation") {
		frames_.push_back(particle_animation(frame_node));
	}
}

class simple_particle_system : public particle_system
{
public:
	simple_particle_system(const entity& e, const simple_particle_system_factory& factory);
	~simple_particle_system() {}

	bool is_destroyed() const { return info_.system_time_to_live_ == 0; }
	void process(const entity& e);
	void draw(const rect& area, const entity& e) const;

private:
	variant get_value(const std::string& key) const { return variant(); }

	const simple_particle_system_factory& factory_;
	simple_particle_system_info info_;

	int cycle_;

	struct particle {
		GLfloat pos[2];
		const particle_animation* anim;
		GLfloat velocity[2];
	};

	struct generation {
		int members;
		int created_at;

		GLfloat rgba[4];
	};

	std::deque<particle> particles_;
	std::deque<generation> generations_;

	int spawn_buildup_;
};

simple_particle_system::simple_particle_system(const entity& e, const simple_particle_system_factory& factory)
  : factory_(factory), info_(factory.info_), cycle_(0), spawn_buildup_(0)
{
}

void simple_particle_system::process(const entity& e)
{
	--info_.system_time_to_live_;
	++cycle_;

	while(!generations_.empty() && cycle_ - generations_.front().created_at == info_.time_to_live_) {
		particles_.erase(particles_.begin(), particles_.begin() + generations_.front().members);
		generations_.pop_front();
	}

	std::deque<particle>::iterator p = particles_.begin();
	foreach(generation& gen, generations_) {
		for(int n = 0; n != gen.members; ++n) {
			p->pos[0] += p->velocity[0];
			p->pos[1] += p->velocity[1];
			p->velocity[0] += info_.accel_x_/1000.0;
			p->velocity[1] += info_.accel_y_/1000.0;
			++p;
		}

		gen.rgba[0] += info_.delta_r_/1000.0;
		gen.rgba[1] += info_.delta_g_/1000.0;
		gen.rgba[2] += info_.delta_b_/1000.0;
		gen.rgba[3] += info_.delta_a_/1000.0;
	}

	int nspawn = info_.spawn_rate_;
	if(info_.spawn_rate_random_ > 0) {
		nspawn += rand()%info_.spawn_rate_random_;
	}

	nspawn += spawn_buildup_;
	spawn_buildup_ = nspawn%1000;
	nspawn /= 1000;

	generation new_gen;
	new_gen.members = nspawn;
	new_gen.created_at = cycle_;
	for(int n = 0; n != 4; ++n) {
		new_gen.rgba[n] = info_.rgba_[n]/255.0;
	}

	generations_.push_back(new_gen);

	while(nspawn-- > 0) {
		particle p;
		p.pos[0] = e.x() + info_.min_x_;
		p.pos[1] = e.y() + info_.min_y_;
		p.velocity[0] = info_.velocity_x_/1000.0;
		p.velocity[1] = info_.velocity_y_/1000.0;

		if(info_.velocity_x_rand_ > 0) {
			p.velocity[0] += (rand()%info_.velocity_x_rand_)/1000.0;
		}

		if(info_.velocity_y_rand_ > 0) {
			p.velocity[1] += (rand()%info_.velocity_y_rand_)/1000.0;
		}

		int velocity_magnitude = info_.velocity_magnitude_;
		if(info_.velocity_magnitude_rand_ > 0) {
			velocity_magnitude += rand()%info_.velocity_magnitude_rand_;
		}

		if(velocity_magnitude) {
			int rotate_velocity = info_.velocity_rotate_;
			if(info_.velocity_rotate_rand_) {
				rotate_velocity += rand()%info_.velocity_rotate_rand_;
			}

			const GLfloat rotate_radians = (GLfloat(rotate_velocity)/360.0)*3.14*2.0;
			const GLfloat magnitude = velocity_magnitude/1000.0;
			p.velocity[0] += sin(rotate_radians)*magnitude;
			p.velocity[1] += cos(rotate_radians)*magnitude;
		}

		ASSERT_GT(factory_.frames_.size(), 0);
		p.anim = &factory_.frames_[rand()%factory_.frames_.size()];

		const int diff_x = info_.max_x_ - info_.min_x_;
		if(diff_x > 0) {
			p.pos[0] += (rand()%(diff_x*1000))/1000.0;
		}

		const int diff_y = info_.max_y_ - info_.min_y_;
		if(diff_y > 0) {
			p.pos[1] += (rand()%(diff_y*1000))/1000.0;
		}

		particles_.push_back(p);
	}
}

void simple_particle_system::draw(const rect& area, const entity& e) const
{
	if(particles_.empty()) {
		return;
	}

	std::deque<particle>::const_iterator p = particles_.begin();

	//all particles must have the same texture, so just set it once.
	p->anim->set_texture();
	glBegin(GL_QUADS);
	foreach(const generation& gen, generations_) {
		glColor4fv(gen.rgba);
		for(int n = 0; n != gen.members; ++n) {
			const particle_animation* anim = p->anim;
			const particle_animation::frame_area& f = anim->get_frame(cycle_ - gen.created_at);
			graphics::texture::set_coord(f.u1, f.v1);
			glVertex3f(p->pos[0], p->pos[1], 0.0);
			graphics::texture::set_coord(f.u2, f.v1);
			glVertex3f(p->pos[0] + anim->width(), p->pos[1], 0.0);
			graphics::texture::set_coord(f.u2, f.v2);
			glVertex3f(p->pos[0] + anim->width(), p->pos[1] + anim->height(), 0.0);
			graphics::texture::set_coord(f.u1, f.v2);
			glVertex3f(p->pos[0], p->pos[1] + anim->height(), 0.0);
			++p;
		}
	}
	glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

particle_system_ptr simple_particle_system_factory::create(const entity& e) const
{
	return particle_system_ptr(new simple_particle_system(e, *this));
}

}

const_particle_system_factory_ptr particle_system_factory::create_factory(wml::const_node_ptr node)
{
	const std::string& type = node->attr("type");
	if(type == "simple") {
		return const_particle_system_factory_ptr(new simple_particle_system_factory(node));
	}

	ASSERT_LOG(false, "Unrecognized particle system type: " << node->attr("type"));
}

particle_system_factory::~particle_system_factory()
{
}

particle_system::~particle_system()
{
}
