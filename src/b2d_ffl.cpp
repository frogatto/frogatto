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
#include "asserts.hpp"
#include "b2d_ffl.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "variant_utils.hpp"

#ifdef USE_BOX2D

namespace box2d
{
	class joint_factory
	{
	public:
		joint_factory(const variant& j);
		virtual ~joint_factory();
		b2JointDef* get_joint_definition() const { return joint_def_.get(); }
		variant write();
	protected:
	private:
		boost::shared_ptr<b2JointDef> joint_def_;
		std::string id_;

		variant joint_variant_def_;
	};

	namespace 
	{
		b2World *current_world = NULL;
		world_ptr this_world;

		typedef std::map<std::string, boost::shared_ptr<joint_factory> > joint_factory_map;
		typedef std::pair<std::string, boost::shared_ptr<joint_factory> > joint_factory_pair;
		joint_factory_map& get_joint_defs()
		{
			static joint_factory_map res;
			return res;
		}
	}

	struct body_destructor
	{
		void operator()(b2Body* b) const 
		{
			// if the world has destructed the body will already have been destroyed.
			if(current_world != NULL) {
				std::cerr << "body_destructor: " << b << std::endl;
				current_world->DestroyBody(b);
			}
		}
	};

	joint_ptr world::find_joint_by_id(const std::string& key) const
	{
		for(b2Joint* j = current_world->GetJointList(); j != NULL; j = j->GetNext()) {
			std::string* s = (std::string*)j->GetUserData();
			if(*s == key) {
				return joint_ptr(new joint(j));
			}
		}
		return joint_ptr();
	}

	class joints_command : public game_logic::formula_callable
	{
	public:
		explicit joints_command()
		{}
		virtual variant get_value(const std::string& key) const
		{
			return variant(this_world->find_joint_by_id(key).get());
		}
		void set_value(const std::string& key, const variant& value)
		{
		}
	};

	manager::manager()	
	{
		try {
			variant w = json::parse_from_file("data/world.cfg");
			this_world = new world(w);
			this_world->finish_loading();
		} catch(json::parse_error&) {
			std::cerr << "WORLD NOT FOUND/NOT VALID. NOT LOADING WORLD. WORLD IS SAD." << std::endl;
		}
	}

	manager::~manager()
	{
		this_world.reset();
	}

	world::world(const variant& w)
		: world_(b2Vec2(0.0f, -10.0f)), velocity_iterations_(8), position_iterations_(3),
		world_x1_(0.0f), world_y1_(0.0f),
		world_x2_(10.0f), world_y2_(10.0f),
		pixel_scale_(w["scale"].as_int(10))
	{
		if(w.has_key("gravity") && w["gravity"].is_list() && w["gravity"].num_elements() == 2) {
			b2Vec2 gravity;
			gravity.x = float(w["gravity"][0].as_decimal().as_float());
			gravity.y = float(w["gravity"][1].as_decimal().as_float());
			world_.SetGravity(gravity);
		}
		world_.SetAllowSleeping(w["allow_sleeping"].as_bool(true));
		if(w.has_key("iterations")) {
			velocity_iterations_ = w["iterations"]["velocity"].as_int(8);
			position_iterations_ = w["iterations"]["position"].as_int(3);
		}
		if(w.has_key("viewport")) {
			ASSERT_LOG(w["viewport"].is_list() && w["viewport"].num_elements() == 4, 
				"viewport must be a list of four elements.");
			world_x1_ = float(w["viewport"][0].as_decimal().as_float());
			world_y1_ = float(w["viewport"][1].as_decimal().as_float());
			world_x2_ = float(w["viewport"][2].as_decimal().as_float());
			world_y2_ = float(w["viewport"][3].as_decimal().as_float());
		}
		if(w.has_key("joints")) {
			if(w["joints"].is_list()) {
				for(size_t n = 0; n < w["joints"].num_elements(); ++n) {
					boost::shared_ptr<joint_factory> p = boost::shared_ptr<joint_factory>(new joint_factory(w["joints"][n]));
					get_joint_defs()[w["joints"][n]["id"].as_string()] = p;
				}
			} else if(w["joints"].is_map()) {
				boost::shared_ptr<joint_factory> p = boost::shared_ptr<joint_factory>(new joint_factory(w["joints"]));
				get_joint_defs()[w["joints"]["id"].as_string()] = p;
			}
		}
	}

	world::~world()
	{
		std::cerr << "DESTRUCTING WORLD" << std::endl;
		clear_current_world();
	}

	const world& world::our_world()
	{
		return *this_world;
	}

	world_ptr world::our_world_ptr()
	{
		return this_world;
	}

	void world::set_dt(float time_step)
	{
		last_dt_ = time_step;
		last_inv_dt_ = 1.0f / time_step;
	}

	void world::step(float time_step)
	{
		set_dt(time_step);
		get_world().Step(time_step, velocity_iterations_, position_iterations_);
	}

	void world::finish_loading()
	{
		set_as_current_world();
		debug_draw_.SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_aabbBit | b2Draw::e_pairBit | b2Draw::e_centerOfMassBit);
		world_.SetDebugDraw(&debug_draw_);
		world_.SetDestructionListener(&destruction_listener_);
	}

	b2World& world::current()
	{
		ASSERT_LOG(current_world != NULL, "Access to world::current() denied. world is NULL");
		return *current_world;
	}

	b2World* world::current_ptr()
	{
		return current_world;
	}

	void world::set_as_current_world()
	{
		current_world = &this->world_;
	}

	void world::clear_current_world()
	{
		current_world = NULL;
	}

	variant world::get_value(const std::string& key) const
	{
		if(key == "gravity") {
			std::vector<variant> v;
			v.push_back(variant(get_world().GetGravity().x));
			v.push_back(variant(get_world().GetGravity().y));
			return variant(&v);
		} else if(key == "allow_sleeping") {
			return variant::from_bool(get_world().GetAllowSleeping());
		} else if(key == "iterations") {
			std::map<variant, variant> m;
			m[variant("velocity")] = variant(velocity_iterations_);
			m[variant("position")] = variant(position_iterations_);
			return variant(&m);
		} else if(key == "viewport") {
			std::vector<variant> v;
			v.push_back(variant(x1()));
			v.push_back(variant(y1()));
			v.push_back(variant(x2()));
			v.push_back(variant(y2()));
			return variant(&v);
		} else if(key == "scale") {
			return variant(scale());
		} else if(key == "debug_draw") {
			return variant::from_bool(draw_debug_data());
		} else if(key == "joints") {
			return variant(new joints_command);
		}
		return variant();
	}

	void world::set_value(const std::string& key, const variant& value)
	{
		if(key == "gravity") {
			ASSERT_LOG(value.is_list() && value.num_elements() == 2, 
				"gravity must be a list of two elements");
			b2Vec2 gravity;
			gravity.x = float(value[0].as_decimal().as_float());
			gravity.y = float(value[1].as_decimal().as_float());
			get_world().SetGravity(gravity);
		} else if(key == "allow_sleeping") {
			get_world().SetAllowSleeping(value.as_bool(true));
		} else if(key == "iterations") {
			velocity_iterations_ = value["velocity"].as_int();
			position_iterations_ = value["position"].as_int();
		} else if(key == "viewport") {
			ASSERT_LOG(value.is_list() && value.num_elements() == 4, 
				"viewport must be a list of four elements.");
			world_x1_ = float(value[0].as_decimal().as_float());
			world_y1_ = float(value[1].as_decimal().as_float());
			world_x2_ = float(value[2].as_decimal().as_float());
			world_y2_ = float(value[3].as_decimal().as_float());
		} else if(key == "scale") {
			set_scale(value.as_int());
		} else if(key == "debug_draw") {
			enable_draw_debug_data(value.as_bool());
		} else if(key == "joints") {
			joint_factory j(value);
		}
	}

	b2Body* world::create_body(body* b)
	{
		b2Body* bp = current().CreateBody(b->get_body_definition());
		std::cerr << "create_body: " << std::hex << bp << " " << b << std::dec << std::endl;
		return bp;
	}

	variant world::write()
	{
		variant_builder res;
		res.add("gravity", get_value("gravity"));
		res.add("allow_sleeping", get_value("allow_sleeping"));
		res.add("iterations", get_value("iterations"));
		res.add("viewport", get_value("viewport"));
		foreach(const joint_factory_pair& j, get_joint_defs()) {
			res.add("joints", j.second->write());
		}
		return res.build();
	}

	destruction_listener::destruction_listener()
	{
	}

	void destruction_listener::SayGoodbye(b2Joint* j)
	{
		std::cerr << "joint being destructed: " << std::hex << intptr_t(j) << std::dec << std::endl;
		delete (std::string*)j->GetUserData();
	}

	void destruction_listener::SayGoodbye(b2Fixture* fix)
	{
		// do nothing.
		//std::cerr << "fixture being destructed: " << std::hex << intptr_t(fix) << std::dec << std::endl;
	}

	boost::shared_ptr<b2FixtureDef> body::create_fixture(const variant& fix)
	{
		b2FixtureDef* fix_def = new b2FixtureDef();
		fix_def->friction = float(fix["friction"].as_decimal().as_float());
		fix_def->restitution = float(fix["restitution"].as_decimal().as_float());
		fix_def->density = float(fix["density"].as_decimal().as_float());
		fix_def->isSensor = fix["is_sensor"].as_bool(false);
		if(fix.has_key("shape")) {
			const variant& shape = fix["shape"];
			ASSERT_LOG(shape.is_map() && shape.has_key("type"), "shape in a fixture must be a map with a 'type' field.");
			const std::string type = shape["type"].as_string();
			if(type == "polygon") {
				ASSERT_LOG(shape.has_key("box") && shape["box"].num_elements() > 1, 
					"polygon shape must specify a 'box' parameter of at least two elements");
				b2PolygonShape* poly_shape = new b2PolygonShape;
				if(shape["box"][0].is_list()) {
					// If the first element is a list, assume this is a list of vertices (i.e. a list
					// of two element lists).
					int num_elements = shape["box"].num_elements();
					//ASSERT_LOG(shape["box"].num_elements() % 2 == 0, "Must have an even number of vertices in the list");
					std::vector<b2Vec2> v;
					v.reserve(num_elements);
					for(int n = 0; n != num_elements; ++n) {
						ASSERT_LOG(shape["box"][n].is_list() && shape["box"][n].num_elements() > 2, 
							"Inner elements must be lists of at least two elements.");
						v.push_back(b2Vec2(float(shape["box"][n][0].as_decimal().as_float()),
							float(shape["box"][n][1].as_decimal().as_float())));
					}
					poly_shape->Set(&v[0], num_elements);
				} else if(shape["box"].num_elements() == 2) {
					// half-width, half-height
					poly_shape->SetAsBox(float(shape["box"][0].as_decimal().as_float()),
						float(shape["box"][1].as_decimal().as_float()));
				} else if(shape["box"].num_elements() == 5) {
					// half-width, half-height, center-x, center-y, angle
					poly_shape->SetAsBox(float(shape["box"][0].as_decimal().as_float()),
						float(shape["box"][1].as_decimal().as_float()),
						b2Vec2(float(shape["box"][2].as_decimal().as_float()),float(shape["box"][3].as_decimal().as_float())),
						float(shape["box"][4].as_decimal().as_float()));
				} else if(shape["box"].num_elements() >= 6) {
					int num_elements = shape["box"].num_elements() / 2;
					ASSERT_LOG(shape["box"].num_elements() % 2 == 0, "Must have an even number of vertices in the list");
					std::vector<b2Vec2> v;
					v.reserve(num_elements);
					for(int n = 0; n != num_elements; n += 2) {
						v.push_back(b2Vec2(float(shape["box"][n+0].as_decimal().as_float()),
							float(shape["box"][n+1].as_decimal().as_float())));
					}
					poly_shape->Set(&v[0], num_elements);
				}
				fix_def->shape = poly_shape;
				shape_list_.push_back(boost::shared_ptr<b2Shape>(poly_shape));
			} else if(type == "circle") {
				b2CircleShape* circle_shape = new b2CircleShape;
				circle_shape->m_radius = float(shape["radius"].as_decimal().as_float());
				if(shape.has_key("position")) {
					ASSERT_LOG(shape["position"].is_list() && shape["position"].num_elements() == 2, 
						"Position must be a list of two elements.");
					circle_shape->m_p.Set(float(shape["position"][0].as_decimal().as_float()),
						float(shape["position"][1].as_decimal().as_float()));
				}
				fix_def->shape = circle_shape;
				shape_list_.push_back(boost::shared_ptr<b2Shape>(circle_shape));
			} else if(type == "edge") {
				b2EdgeShape* edge_shape = new b2EdgeShape;
				ASSERT_LOG(shape.has_key("vertex1") && shape["vertex1"].is_list() && shape["vertex1"].num_elements() == 2,
					"Must have vertex1 attribute, being a list of two elements (x,y).");
				ASSERT_LOG(shape.has_key("vertex2") && shape["vertex2"].is_list() && shape["vertex2"].num_elements() == 2,
					"Must have vertex2 attribute, being a list of two elements (x,y).");
				edge_shape->m_vertex1.Set(float32(shape["vertex1"][0].as_decimal().as_float()), float32(shape["vertex1"][1].as_decimal().as_float()));
				edge_shape->m_vertex2.Set(float32(shape["vertex2"][0].as_decimal().as_float()), float32(shape["vertex2"][1].as_decimal().as_float()));
				if(shape.has_key("vertex0")) {
					ASSERT_LOG(shape["vertex0"].is_list() && shape["vertex0"].num_elements() == 2,
						"vertex0 attribute must be a list of two elements (x,y).");
					edge_shape->m_vertex0.Set(float32(shape["vertex0"][0].as_decimal().as_float()), float32(shape["vertex0"][1].as_decimal().as_float()));
					edge_shape->m_hasVertex0 = true;
				}
				if(shape.has_key("vertex3")) {
					ASSERT_LOG(shape["vertex3"].is_list() && shape["vertex3"].num_elements() == 2,
						"vertex3 attribute must be a list of two elements (x,y).");
					edge_shape->m_vertex3.Set(float32(shape["vertex3"][0].as_decimal().as_float()), float32(shape["vertex3"][1].as_decimal().as_float()));
					edge_shape->m_hasVertex3 = true;
				}
				fix_def->shape = edge_shape;
				shape_list_.push_back(boost::shared_ptr<b2Shape>(edge_shape));
			} else if(type == "chain") {
				b2ChainShape* chain_shape = new b2ChainShape;
				ASSERT_LOG(shape.has_key("vertices") && shape["vertices"].is_list(), "verticies must be a list");
				bool loop = shape["loop"].as_bool(false);
				std::vector<b2Vec2> vertices;
				for(size_t n = 0; n < shape["vertices"].num_elements(); ++n) {
					ASSERT_LOG(shape["vertices"][n].is_list() && shape["vertices"][n].num_elements() > 2, 
						"Inner items on vertices must be lists of length > 2.");
					vertices.push_back(b2Vec2(float32(shape["vertices"][n][0].as_decimal().as_float()), float32(shape["vertex3"][n][1].as_decimal().as_float())));
				}
				if(loop) {
					chain_shape->CreateLoop(&vertices[0], vertices.size());
				} else {
					chain_shape->CreateChain(&vertices[0], vertices.size());
				}
				if(shape.has_key("previous_vertex")) {
					chain_shape->SetPrevVertex(b2Vec2(float32(shape["previous_vertex"][0].as_decimal().as_float()), float32(shape["previous_vertex"][1].as_decimal().as_float())));
				}
				if(shape.has_key("next_vertex")) {
					chain_shape->SetNextVertex(b2Vec2(float32(shape["next_vertex"][0].as_decimal().as_float()), float32(shape["next_vertex"][1].as_decimal().as_float())));
				}
				fix_def->shape = chain_shape;
				shape_list_.push_back(boost::shared_ptr<b2Shape>(chain_shape));
			} else {
				ASSERT_LOG(false, "Unrecognised shape type: " << type);
			}
		}
		return boost::shared_ptr<b2FixtureDef>(fix_def);
	}

	body::body(const variant& value) 
	{
		// value["id"]...
		if(value.has_key("position")) {
			ASSERT_LOG(value["position"].is_list() && value["position"].num_elements() == 2, 
				"Position must be a list of two elements.");
			body_def_.position.Set(float(value["position"][0].as_decimal().as_float()),
				float(value["position"][1].as_decimal().as_float()));
		} else {
			body_def_.position.SetZero();
		}
		if(value.has_key("type")) {
			const std::string type = value["type"].as_string();
			if(type == "static") {
				body_def_.type = b2_staticBody;
			} else if(type == "dynamic") {
				body_def_.type = b2_dynamicBody;
			} else if(type == "kinematic") {
				body_def_.type = b2_kinematicBody;
			} else {
				ASSERT_LOG(false, "Unrecognised body type parameter:" << type);
			}
		}
		if(value.has_key("active")) {
			body_def_.active = value["active"].as_bool(true);
		}
		if(value.has_key("angle")) {
			body_def_.angle = float(value["angle"].as_decimal().as_float());
		}
		if(value.has_key("angular_velocity")) {
			body_def_.angularVelocity = float(value["angular_velocity"].as_decimal().as_float());
		}
		if(value.has_key("angular_damping")) {
			body_def_.angularDamping = float(value["angular_damping"].as_decimal().as_float());
		}
		if(value.has_key("allow_sleeping")) {
			body_def_.allowSleep = value["allow_sleeping"].as_bool(true);
		}
		if(value.has_key("awake")) {
			body_def_.awake = value["awake"].as_bool(true);
		}
		if(value.has_key("bullet")) {
			body_def_.bullet = value["bullet"].as_bool(false);
		}
		if(value.has_key("fixed_rotation")) {
			body_def_.fixedRotation = value["fixed_rotation"].as_bool(false);
		}
		if(value.has_key("gravity_scale")) {
			body_def_.gravityScale = float(value["gravity_scale"].as_decimal().as_float());
		}
		if(value.has_key("linear_damping")) {
			body_def_.linearDamping = float(value["linear_damping"].as_decimal().as_float());
		}
		if(value.has_key("linear_velocity")) {
			ASSERT_LOG(value["linear_velocity"].is_list() && value["linear_velocity"].num_elements() == 2, 
				"linear_velocity must be a list of two elements.");
			body_def_.linearVelocity.Set(float(value["linear_velocity"][0].as_decimal().as_float()),
				float(value["linear_velocity"][1].as_decimal().as_float()));
		}

		if(value.has_key("fixtures")) {
			ASSERT_LOG(value["fixtures"].is_list(), "fixtures must be a list.");
			for(int n = 0; n != value["fixtures"].num_elements(); ++n) {
				fix_defs_.push_back(create_fixture(value["fixtures"][n]));
			}
		}
	}

	body::~body()
	{
	}

	
	void body::finish_loading(entity_ptr e)
	{
		world_ptr wp = world::our_world_ptr();
		if(e != NULL) {
			if(body_def_.position.x == 0 && body_def_.position.y == 0) {
				body_def_.position.x = float(e->x());
				body_def_.position.y = float(e->y());
			}
			body_def_.userData = e.get();
		} else {
			body_def_.userData = NULL;
		}
		body_def_.position.x /= wp->scale();
		body_def_.position.y /= wp->scale();

		body_ = boost::shared_ptr<b2Body>(wp->create_body(this), body_destructor());
		foreach(const boost::shared_ptr<b2FixtureDef> fix_def, fix_defs_) {
			body_->CreateFixture(fix_def.get());
		}
		body_->ResetMassData();
	}

	bool body::active() const
	{
		ASSERT_LOG(body_ != NULL, "body_ is NULL in active()");
		return body_->IsActive();
	}

	void body::set_active(bool actv)
	{
		ASSERT_LOG(body_ != NULL, "body_ is NULL in set_active()");
		body_->SetActive(actv);
	}


	variant body::get_value(const std::string& key) const
	{
		ASSERT_LOG(body_ != NULL, "Can't set parameters on this body. body_ == NULL");
		if(key == "active") {
			return variant::from_bool(body_->IsActive());
		} else if(key == "angle") {
			return variant(body_->GetAngle());
		} else if(key == "angular_velocity") {
			return variant(body_->GetAngularVelocity());
		} else if(key == "angular_damping") {
			return variant(body_->GetAngularDamping());
		} else if(key == "allow_sleeping") {
			return variant::from_bool(body_->IsSleepingAllowed());
		} else if(key == "awake") {
			return variant::from_bool(body_->IsAwake());
		} else if(key == "bullet") {
			return variant::from_bool(body_->IsBullet());
		} else if(key == "fixed_rotation") {
			return variant::from_bool(body_->IsFixedRotation());
		} else if(key == "gravity_scale") {
			return variant(body_->GetGravityScale());
		} else if(key == "linear_damping") {
			return variant(body_->GetLinearDamping());
		} else if(key == "linear_velocity") {
			b2Vec2 lv = body_->GetLinearVelocity();
			std::vector<variant> v;
			v.push_back(variant(lv.x));
			v.push_back(variant(lv.y));
			return variant(&v);
		} else if(key == "fixtures") {
			ASSERT_LOG(false, "todo: fix fixtures");
		} else if(key == "position") {
			b2Vec2 pos = body_->GetPosition();
			std::vector<variant> v;
			v.push_back(variant(pos.x));
			v.push_back(variant(pos.y));
			return variant(&v);
		} else if(key == "type") {
			b2BodyType type = body_->GetType();
			if(type == b2_staticBody) {
				return variant("static");
			} else if(type == b2_kinematicBody) {
				return variant("kinematic");
			} else if(type == b2_dynamicBody) {
				return variant("dynamic");
			} else {
				ASSERT_LOG(false, "unimplemented internal type: " << type);
			}
		} else if(key == "mass") {
			return variant(body_->GetMass());
		} else if(key == "inertia") {
			return variant(body_->GetInertia());
		} else if(key == "joints") {
			ASSERT_LOG(false, "todo: joints");
		}
		return variant();
	}

	void body::set_value(const std::string& key, const variant& value)
	{
		ASSERT_LOG(body_ != NULL, "Can't set parameters on this body. body_ == NULL");
		if(key == "active") {
			body_->SetActive(value.as_bool());
		} else if(key == "angular_velocity") {
			body_->SetAngularVelocity(float(value.as_decimal().as_float()));
		} else if(key == "angular_damping") {
			body_->SetAngularDamping(float(value.as_decimal().as_float()));
		} else if(key == "allow_sleeping") {
			body_->SetSleepingAllowed(value.as_bool());
		} else if(key == "awake") {
			body_->SetAwake(value.as_bool());
		} else if(key == "bullet") {
			body_->SetBullet(value.as_bool());
		} else if(key == "fixed_rotation") {
			body_->SetFixedRotation(value.as_bool());
		} else if(key == "gravity_scale") {
			body_->SetGravityScale(float(value.as_decimal().as_float()));
		} else if(key == "linear_damping") {
			body_->SetLinearDamping(float(value.as_decimal().as_float()));
		} else if(key == "linear_velocity") {
			body_->SetLinearVelocity(b2Vec2(float(value[0].as_decimal().as_float()), 
				float(value[1].as_decimal().as_float())));
		} else if(key == "fixtures") {
			ASSERT_LOG(false, "todo: fix fixtures");
		} else if(key == "transform") {
			if(value.num_elements() == 2 && value[0].is_list() && value[1].is_decimal()) {
				body_->SetTransform(
					b2Vec2(float(value[0][0].as_decimal().as_float()), float(value[0][1].as_decimal().as_float())), 
					float(value[1].as_decimal().as_float()));
			} else if(value.num_elements() == 3) {
				body_->SetTransform(
					b2Vec2(float(value[0].as_decimal().as_float()), float(value[1].as_decimal().as_float())), 
					float(value[2].as_decimal().as_float()));
			} else {
				ASSERT_LOG(false, "Unrecognised position and angle format for transform command");
			}
		} else if(key == "type") {
			const std::string type = value.as_string();
			if(type == "static") {
				body_->SetType(b2_staticBody);
			} else if(type == "dynamic") {
				body_->SetType(b2_dynamicBody);
			} else if(type == "kinematic") {
				body_->SetType(b2_kinematicBody);
			} else {
				ASSERT_LOG(false, "Unrecognised body type parameter:" << type);
			}
		} else if(key == "linear_impulse") {
			b2Vec2 impluse;
			b2Vec2 point;
			ASSERT_LOG(value.is_list(), "linear_impulse must be a list type.");
			if(value.num_elements() == 4) {
				impluse.Set(float32(value[0].as_decimal().as_float()), float32(value[1].as_decimal().as_float()));
				point.Set(float32(value[2].as_decimal().as_float()), float32(value[3].as_decimal().as_float()));
			} else if(value.num_elements() == 2) {
				ASSERT_LOG(value[0].is_list() && value[1].is_list(), 
					"Syntax for linear impulse is [[impx,impy],[pointx,pointy]] or [impx,impy,pointx,pointy]");
				impluse.Set(float32(value[0][0].as_decimal().as_float()), float32(value[0][1].as_decimal().as_float()));
				point.Set(float32(value[1][0].as_decimal().as_float()), float32(value[1][1].as_decimal().as_float()));
			} else {
				ASSERT_LOG(false, 
					"Syntax for linear impulse is [[impx,impy],[pointx,pointy]] or [impx,impy,pointx,pointy]");
			}
			body_->ApplyLinearImpulse(impluse, point);
		} else if(key == "angular_impulse") {
			body_->ApplyAngularImpulse(float32(value.as_decimal().as_float()));
		} else if(key == "torque") {
			body_->ApplyTorque(float32(value.as_decimal().as_float()));
		} else if(key == "force") {
			b2Vec2 f, p;
			ASSERT_LOG(value.is_list(), "force must be a list type.");
			if(value.num_elements() == 4) {
				f.Set(float32(value[0].as_decimal().as_float()), float32(value[1].as_decimal().as_float()));
				p.Set(float32(value[2].as_decimal().as_float()), float32(value[3].as_decimal().as_float()));
			} else if(value.num_elements() == 2) {
				ASSERT_LOG(value[0].is_list() && value[1].is_list(), 
					"Syntax for force impulse is [[forcex,forcey],[pointx,pointy]] or [forcex,forcey,pointx,pointy]");
				f.Set(float32(value[0][0].as_decimal().as_float()), float32(value[0][1].as_decimal().as_float()));
				p.Set(float32(value[1][0].as_decimal().as_float()), float32(value[1][1].as_decimal().as_float()));
			} else {
				ASSERT_LOG(false, 
					"Syntax for force impulse is [[forcex,forcey],[pointx,pointy]] or [forcex,forcey,pointx,pointy]");
			}
			body_->ApplyForce(f, p);		
		} else if(key == "force_to_center" || key == "force_to_centre") {
			b2Vec2 f;
			ASSERT_LOG(value.is_list() && value.num_elements() == 2,
				"force to apply is a list of two elements representing the force vector in Newtons");
			f.Set(float32(value[0].as_decimal().as_float()), float32(value[1].as_decimal().as_float()));
			body_->ApplyForceToCenter(f);
		}
	}

	variant body::fix_write()
	{
		variant_builder res;
		std::vector<boost::shared_ptr<b2FixtureDef> >::const_iterator it = fix_defs_.begin();
		while(it != fix_defs_.end()) {
			variant_builder fix;
			fix.add("friction", variant((*it)->friction));
			fix.add("restitution", variant((*it)->restitution));
			fix.add("density", variant((*it)->density));
			fix.add("is_sensor", variant((*it)->isSensor));

			variant_builder filter;
			filter.add("category_bits", (*it)->filter.categoryBits);
			filter.add("group_index", (*it)->filter.groupIndex);
			filter.add("mask_bits", (*it)->filter.maskBits);
			fix.add("filter", filter.build());
			fix.add("shape", shape_write((*it)->shape));
			
			res.add("fixtures", fix.build());
			++it;
		}
		return res.build();
	}

	variant body::shape_write(const b2Shape* shape)
	{
		variant_builder res;
		if(shape->GetType() == b2Shape::e_circle) {
			b2CircleShape* circle = (b2CircleShape*)shape;
			res.add("type", "circle");
			std::vector<variant> v;
			v.push_back(variant(circle->m_p.x));
			v.push_back(variant(circle->m_p.y));
			res.add("position", variant(&v));
			res.add("radius", circle->m_radius);
		} else if(shape->GetType() == b2Shape::e_edge) {
			b2EdgeShape* edge = (b2EdgeShape*)shape;
			res.add("type", "edge");
			std::vector<variant> v1;
			v1.push_back(variant(edge->m_vertex1.x));
			v1.push_back(variant(edge->m_vertex1.y));
			res.add("vertex1", variant(&v1));
			std::vector<variant> v2;
			v2.push_back(variant(edge->m_vertex2.x));
			v2.push_back(variant(edge->m_vertex2.y));
			res.add("vertex2", variant(&v2));
			if(edge->m_hasVertex0) {
				std::vector<variant> v0;
				v0.push_back(variant(edge->m_vertex0.x));
				v0.push_back(variant(edge->m_vertex0.y));
				res.add("vertex0", variant(&v0));
			}
			if(edge->m_hasVertex3) {
				std::vector<variant> v3;
				v3.push_back(variant(edge->m_vertex3.x));
				v3.push_back(variant(edge->m_vertex3.y));
				res.add("vertex3", variant(&v3));
			}
		} else if(shape->GetType() == b2Shape::e_polygon) {
			b2PolygonShape* poly = (b2PolygonShape*)shape;
			res.add("type", "polygon");
			std::vector<variant> vertices;
			for(int n = 0; n < poly->m_vertexCount; ++n) {
				std::vector<variant> vertex;
				vertex.push_back(variant(poly->m_vertices[n].x));
				vertex.push_back(variant(poly->m_vertices[n].y));
				vertices.push_back(variant(&vertex));
			}
			res.add("box",variant(&vertices)); 
		} else if(shape->GetType() == b2Shape::e_chain) {
			b2ChainShape* chain = (b2ChainShape*)shape;
			res.add("type", "chain");
			std::vector<variant> vertices;
			for(int n = 0; n < chain->m_count; ++n) {
				std::vector<variant> vertex;
				vertex.push_back(variant(chain->m_vertices[n].x));
				vertex.push_back(variant(chain->m_vertices[n].y));
				vertices.push_back(variant(&vertex));
			}
			res.add("vertices", variant(&vertices));
			if(chain->m_hasPrevVertex) {
				std::vector<variant> pvertex;
				pvertex.push_back(variant(chain->m_prevVertex.x));
				pvertex.push_back(variant(chain->m_prevVertex.y));
				res.add("previous_vertex", variant(&pvertex));
			}
			if(chain->m_hasNextVertex) {
				std::vector<variant> nvertex;
				nvertex.push_back(variant(chain->m_nextVertex.x));
				nvertex.push_back(variant(chain->m_nextVertex.y));
				res.add("next_vertex", variant(&nvertex));
			}
			//res.add("loop", chain->
		}
		return res.build();
	}

	variant body::write()
	{
		variant_builder res;
		if(body_def_.type == b2_staticBody) {
			res.add("type", variant("static"));
		} else if(body_def_.type == b2_dynamicBody) {
			res.add("type", variant("dynamic"));
		} else if(body_def_.type == b2_kinematicBody) {
			res.add("type", variant("kinematic"));
		}
		res.add("active", variant::from_bool(body_def_.active));
		res.add("allow_sleeping", variant::from_bool(body_def_.allowSleep));
		res.add("bullet", variant::from_bool(body_def_.bullet));
		res.add("fixed_rotation", variant::from_bool(body_def_.fixedRotation));
		std::vector<variant> v;
		v.push_back(variant(body_def_.position.x));
		v.push_back(variant(body_def_.position.y));
		res.add("position", variant(&v));
		res.add("angle", variant(body_def_.angle));
		res.add("angular_velocity", variant(body_def_.angularVelocity));
		res.add("angular_damping", variant(body_def_.angularDamping));
		res.add("gravity_scale", variant(body_def_.gravityScale));
		res.add("linear_damping", variant(body_def_.linearDamping));
		v.clear();
		v.push_back(variant(body_def_.linearVelocity.x));
		v.push_back(variant(body_def_.linearVelocity.y));
		res.add("linear_velocity", variant(&v));

		res.add("fixtures", fix_write());
		return res.build();
	}

	joint_factory::joint_factory(const variant& value) 
		: joint_variant_def_(value)
	{
		ASSERT_LOG(value.has_key("type"), "Joints must specify a 'type' field.");
		ASSERT_LOG(value.has_key("id"), "Joints must specify an 'id' field.");
		ASSERT_LOG(value.has_key("a") && value.has_key("b"), "Joints must have bodies 'a' and 'b' fields.");
		const std::string type = value["type"].as_string();
		body_ptr body_a_ = value["a"].try_convert<body>();
		body_ptr body_b_ = value["b"].try_convert<body>();
		bool collide_connected = value["collide_connected"].as_bool(false);

		if(type == "revolute") {
			boost::shared_ptr<b2RevoluteJointDef> revolute = boost::shared_ptr<b2RevoluteJointDef>(new b2RevoluteJointDef);
			b2Vec2 anchor = body_a_->get_raw_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor")) {
				ASSERT_LOG(value["anchor"].is_list() && value["anchor"].num_elements() == 2,
					"'anchor' must be a list of two elements.");
				anchor.Set(float32(value["anchor"][0].as_decimal().as_float()), float32(value["anchor"][1].as_decimal().as_float()));
			} 
			revolute->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor);
			if(value.has_key("lower_angle")) {
				revolute->lowerAngle = float(value["lower_angle"].as_decimal().as_float()) * b2_pi / 180.0f;
			}
			if(value.has_key("upper_angle")) {
				revolute->lowerAngle = float(value["upper_angle"].as_decimal().as_float()) * b2_pi / 180.0f;
			}
			if(value.has_key("reference_angle")) {
				revolute->referenceAngle = float(value["reference_angle"].as_decimal().as_float()) * b2_pi / 180.0f;
			}
			if(value.has_key("enable_limit")) {
				revolute->enableLimit = value["enable_limit"].as_bool();
			}
			if(value.has_key("max_motor_torque")) {
				revolute->maxMotorTorque = float(value["max_motor_torque"].as_decimal().as_float());
			}
			if(value.has_key("motor_speed")) {
				revolute->motorSpeed = float(value["motor_speed"].as_decimal().as_float());
			}
			if(value.has_key("enable_motor")) {
				revolute->enableMotor = value["enable_motor"].as_bool();
			}
			joint_def_ = revolute;
		} else if(type == "distance") {
			boost::shared_ptr<b2DistanceJointDef> distance = boost::shared_ptr<b2DistanceJointDef>(new b2DistanceJointDef);
			b2Vec2 anchor_a = body_a_->get_body_ptr()->GetWorldCenter();
			b2Vec2 anchor_b = body_b_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor_a")) {
				ASSERT_LOG(value["anchor_a"].is_list() && value["anchor_a"].num_elements() == 2,
					"'anchor_a' must be a list of two elements.");
				anchor_a.Set(float32(value["anchor_a"][0].as_decimal().as_float()), float32(value["anchor_a"][1].as_decimal().as_float()));
			}
			if(value.has_key("anchor_b")) {
				ASSERT_LOG(value["anchor_b"].is_list() && value["anchor_b"].num_elements() == 2,
					"'anchor_b' must be a list of two elements.");
				anchor_b.Set(float32(value["anchor_b"][0].as_decimal().as_float()), float32(value["anchor_b"][1].as_decimal().as_float()));
			}
			distance->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor_a, anchor_b);
			if(value.has_key("frequency")) {
				distance->frequencyHz = float(value["frequency"].as_decimal().as_float());
			}
			if(value.has_key("damping_ratio")) {
				distance->dampingRatio = float(value["damping_ratio"].as_decimal().as_float());
			}
			joint_def_ = distance;
		} else if(type == "prismatic") {
			boost::shared_ptr<b2PrismaticJointDef> prismatic = boost::shared_ptr<b2PrismaticJointDef>(new b2PrismaticJointDef);
			b2Vec2 anchor = body_a_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor")) {
				ASSERT_LOG(value["anchor"].is_list() && value["anchor"].num_elements() == 2,
					"'anchor' must be a list of two elements.");
				anchor.Set(float32(value["anchor"][0].as_decimal().as_float()), float32(value["anchor"][1].as_decimal().as_float()));
			} 
			b2Vec2 axis(1.0f, 0.0f);
			if(value.has_key("axis")) {
				ASSERT_LOG(value["axis"].is_list() && value["axis"].num_elements() == 2,
					"'axis' must be a list of two elements.");
				anchor.Set(float32(value["axis"][0].as_decimal().as_float()), float32(value["axis"][1].as_decimal().as_float()));
			}
			prismatic->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor, axis);
			if(value.has_key("lower_translation")) {
				prismatic->lowerTranslation = float(value["lower_translation"].as_decimal().as_float());
			}
			if(value.has_key("upper_translation")) {
				prismatic->upperTranslation = float(value["upper_translation"].as_decimal().as_float());
			}
			if(value.has_key("reference_angle")) {
				prismatic->referenceAngle = float(value["reference_angle"].as_decimal().as_float()) * b2_pi / 180.0f;
			}
			if(value.has_key("enable_limit")) {
				prismatic->enableLimit = value["enable_limit"].as_bool();
			}
			if(value.has_key("max_motor_force")) {
				prismatic->maxMotorForce = float(value["max_motor_force"].as_decimal().as_float());
			}
			if(value.has_key("motor_speed")) {
				prismatic->motorSpeed = float(value["motor_speed"].as_decimal().as_float());
			}
			if(value.has_key("enable_motor")) {
				prismatic->enableMotor = value["enable_motor"].as_bool();
			}
			joint_def_ = prismatic;
		} else if(type == "pulley") {
			boost::shared_ptr<b2PulleyJointDef> pulley = boost::shared_ptr<b2PulleyJointDef>(new b2PulleyJointDef);
			b2Vec2 anchor_a = body_a_->get_body_ptr()->GetWorldCenter();
			b2Vec2 anchor_b = body_b_->get_body_ptr()->GetWorldCenter();
			b2Vec2 ground_anchor_a;
			b2Vec2 ground_anchor_b;
			if(value.has_key("anchor_a")) {
				ASSERT_LOG(value["anchor_a"].is_list() && value["anchor_a"].num_elements() == 2,
					"'anchor_a' must be a list of two elements.");
				anchor_a.Set(float32(value["anchor_a"][0].as_decimal().as_float()), float32(value["anchor_a"][1].as_decimal().as_float()));
			}
			if(value.has_key("anchor_b")) {
				ASSERT_LOG(value["anchor_b"].is_list() && value["anchor_b"].num_elements() == 2,
					"'anchor_b' must be a list of two elements.");
				anchor_b.Set(float32(value["anchor_b"][0].as_decimal().as_float()), float32(value["anchor_b"][1].as_decimal().as_float()));
			}
			ASSERT_LOG(value["ground_anchor_a"].is_list() && value["ground_anchor_a"].num_elements() == 2,
				"'ground_anchor_a' must be a list of two elements.");
			ground_anchor_a.Set(float32(value["ground_anchor_a"][0].as_decimal().as_float()), float32(value["ground_anchor_a"][1].as_decimal().as_float()));
			ASSERT_LOG(value["ground_anchor_b"].is_list() && value["ground_anchor_b"].num_elements() == 2,
				"'ground_anchor_b' must be a list of two elements.");
			ground_anchor_b.Set(float32(value["ground_anchor_b"][0].as_decimal().as_float()), float32(value["ground_anchor_b"][1].as_decimal().as_float()));
			float32 ratio = float(value["ratio"].as_decimal().as_float());
			pulley->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), ground_anchor_a, ground_anchor_b, anchor_a, anchor_b, ratio);
			joint_def_ = pulley;
		} else if(type == "gear") {
			boost::shared_ptr<b2GearJointDef> gear = boost::shared_ptr<b2GearJointDef>(new b2GearJointDef);
			gear->ratio = float(value["ratio"].as_decimal(decimal(1.0)).as_float());
			gear->bodyA = body_a_->get_raw_body_ptr();
			gear->bodyB = body_b_->get_raw_body_ptr();
			ASSERT_LOG(value.has_key("get_raw_body_ptr") && value["joints"].is_list() && value["joints"].num_elements() == 2,
				"Must supply a list of two joints.");
			ASSERT_LOG(false, "Gear joint to fix");
			joint_ptr j1 = this_world->find_joint_by_id(value["joints"][0].as_string());
			joint_ptr j2 = this_world->find_joint_by_id(value["joints"][1].as_string());
			gear->joint1 = j1->get_b2Joint();
			gear->joint2 = j1->get_b2Joint();
			joint_def_ = gear;
		} else if(type == "mouse") {
			boost::shared_ptr<b2MouseJointDef> mouse = boost::shared_ptr<b2MouseJointDef>(new b2MouseJointDef);
			b2Vec2 target = body_a_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("target")) {
				ASSERT_LOG(value["target"].is_list() && value["target"].num_elements() == 2,
					"'target' must be a list of two elements.");
				target.Set(float(value["target"][0].as_decimal().as_float()), float(value["target"][1].as_decimal().as_float()));
			}
			mouse->target = target;
			if(value.has_key("max_force")) {
				mouse->maxForce = float(value["max_force"].as_decimal().as_float());
			}
			if(value.has_key("frequency")) {
				mouse->frequencyHz = float(value["frequency"].as_decimal().as_float());
			}
			if(value.has_key("damping_ratio")) {
				mouse->dampingRatio = float(value["damping_ratio"].as_decimal().as_float());
			}
			joint_def_ = mouse;
		} else if(type == "wheel") {
			boost::shared_ptr<b2WheelJointDef> wheel = boost::shared_ptr<b2WheelJointDef>(new b2WheelJointDef);
			b2Vec2 anchor = body_a_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor")) {
				ASSERT_LOG(value["anchor"].is_list() && value["anchor"].num_elements() == 2,
					"'anchor' must be a list of two elements.");
				anchor.Set(float32(value["anchor"][0].as_decimal().as_float()), float32(value["anchor"][1].as_decimal().as_float()));
			} 
			b2Vec2 axis(1.0f, 0.0f);
			if(value.has_key("axis")) {
				ASSERT_LOG(value["axis"].is_list() && value["axis"].num_elements() == 2,
					"'axis' must be a list of two elements.");
				anchor.Set(float32(value["axis"][0].as_decimal().as_float()), float32(value["axis"][1].as_decimal().as_float()));
			}
			wheel->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor, axis);
			wheel->maxMotorTorque = float(value["max_motor_torque"].as_decimal(decimal(0.0)).as_float());
			wheel->motorSpeed = float(value["motor_speed"].as_decimal(decimal(0.0)).as_float());
			wheel->enableMotor = value["enable_motor"].as_bool(false);
			wheel->frequencyHz = float(value["frequency"].as_decimal(decimal(2.0)).as_float());
			wheel->dampingRatio = float(value["damping_ratio"].as_decimal(decimal(0.7)).as_float());
			joint_def_ = wheel;
		} else if(type == "weld") {
			boost::shared_ptr<b2WeldJointDef> weld = boost::shared_ptr<b2WeldJointDef>(new b2WeldJointDef);
			b2Vec2 anchor = body_a_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor")) {
				ASSERT_LOG(value["anchor"].is_list() && value["anchor"].num_elements() == 2,
					"'anchor' must be a list of two elements.");
				anchor.Set(float32(value["anchor"][0].as_decimal().as_float()), float32(value["anchor"][1].as_decimal().as_float()));
			}
			weld->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor);
			weld->referenceAngle = float(value["reference_angle"].as_decimal().as_float()) * b2_pi / 180.0f;
			weld->frequencyHz = float(value["frequency"].as_decimal(decimal(0.0)).as_float());
			weld->dampingRatio = float(value["damping_ratio"].as_decimal(decimal(0.0)).as_float());
			joint_def_ = weld;
		} else if(type == "rope") {
			boost::shared_ptr<b2RopeJointDef> rope = boost::shared_ptr<b2RopeJointDef>(new b2RopeJointDef);
			rope->bodyA = body_a_->get_raw_body_ptr();
			rope->bodyB = body_b_->get_raw_body_ptr();
			b2Vec2 local_anchor_a(-1.0f, 0.0f);
			b2Vec2 local_anchor_b(1.0f, 0.0f);
			if(value.has_key("local_anchor_a")) {
				ASSERT_LOG(value["local_anchor_a"].is_list() && value["local_anchor_a"].num_elements() == 2,
					"'local_anchor_a' must be a list of two elements");
				local_anchor_a.Set(float(value["local_anchor_a"][0].as_decimal().as_float()), float(value["local_anchor_a"][1].as_decimal().as_float()));
			}
			if(value.has_key("local_anchor_b")) {
				ASSERT_LOG(value["local_anchor_b"].is_list() && value["local_anchor_b"].num_elements() == 2,
					"'local_anchor_b' must be a list of two elements");
				local_anchor_b.Set(float(value["local_anchor_b"][0].as_decimal().as_float()), float(value["local_anchor_b"][1].as_decimal().as_float()));
			}
			rope->localAnchorA = local_anchor_a;
			rope->localAnchorB = local_anchor_b;
			rope->maxLength = float(value["max_length"].as_decimal(decimal(0.0)).as_float());
			joint_def_ = rope;
		} else if(type == "friction") {
			boost::shared_ptr<b2FrictionJointDef> friction = boost::shared_ptr<b2FrictionJointDef>(new b2FrictionJointDef);
			b2Vec2 anchor = body_a_->get_body_ptr()->GetWorldCenter();
			if(value.has_key("anchor")) {
				ASSERT_LOG(value["anchor"].is_list() && value["anchor"].num_elements() == 2,
					"'anchor' must be a list of two elements.");
				anchor.Set(float32(value["anchor"][0].as_decimal().as_float()), float32(value["anchor"][1].as_decimal().as_float()));
			}
			b2Vec2 local_anchor_a(0.0f, 0.0f);
			b2Vec2 local_anchor_b(0.0f, 0.0f);
			if(value.has_key("local_anchor_a")) {
				ASSERT_LOG(value["local_anchor_a"].is_list() && value["local_anchor_a"].num_elements() == 2,
					"'local_anchor_a' must be a list of two elements");
				local_anchor_a.Set(float(value["local_anchor_a"][0].as_decimal().as_float()), float(value["local_anchor_a"][1].as_decimal().as_float()));
			}
			if(value.has_key("local_anchor_b")) {
				ASSERT_LOG(value["local_anchor_b"].is_list() && value["local_anchor_b"].num_elements() == 2,
					"'local_anchor_b' must be a list of two elements");
				local_anchor_b.Set(float(value["local_anchor_b"][0].as_decimal().as_float()), float(value["local_anchor_b"][1].as_decimal().as_float()));
			}
			friction->Initialize(body_a_->get_raw_body_ptr(), body_b_->get_raw_body_ptr(), anchor);
			friction->localAnchorA = local_anchor_a;
			friction->localAnchorB = local_anchor_b;
			friction->maxForce = float(value["max_force"].as_decimal(decimal(0.0)).as_float());
			friction->maxTorque = float(value["max_torque"].as_decimal(decimal(0.0)).as_float());
			joint_def_ = friction;
		} else {
			ASSERT_LOG(false, "Unrecognised joint type '" << type << "'");
		}
		joint_def_->collideConnected = collide_connected;
		joint_def_->userData = new std::string(value["id"].as_string());
		current_world->CreateJoint(joint_def_.get());
	}

	joint_factory::~joint_factory()
	{
	}

	variant joint_factory::write()
	{
		variant_builder res;
		ASSERT_LOG(joint_def_ != NULL, "No joint definition found.");
		variant keys = joint_variant_def_.get_keys();
		for(int n = 0; n != keys.num_elements(); ++n) {
			res.add(keys[n].as_string(), joint_variant_def_[keys[n]]);
		}
		return res.build();

	}

	joint::joint(b2Joint* j)
		: joint_(j)
	{
	}

	variant joint::get_value(const std::string& key) const
	{
		ASSERT_LOG(joint_ != NULL, "Internal joint has been destroyed.");
		if(key == "a") {
			return variant((body*)joint_->GetBodyA()->GetUserData());
		} else if(key == "b") {
			return variant((body*)joint_->GetBodyB()->GetUserData());
		} else if(key == "collide_connected") {
			return variant::from_bool(joint_->GetCollideConnected());
		} else if(key == "anchor_a") {
			// world co-ordinates
			std::vector<variant> v;
			const b2Vec2 &anchor = joint_->GetAnchorA();
			v.push_back(variant(anchor.x));
			v.push_back(variant(anchor.y));
			return variant(&v);
		} else if(key == "anchor_b") {
			// world co-ordinates
			std::vector<variant> v;
			const b2Vec2 &anchor = joint_->GetAnchorB();
			v.push_back(variant(anchor.x));
			v.push_back(variant(anchor.y));
			return variant(&v);
		} else if(key == "active") {
			return variant::from_bool(joint_->IsActive());
		} else if(key == "type") {
			if(joint_->GetType() == e_revoluteJoint) {
				return variant("revolute");
			} else if(joint_->GetType() == e_prismaticJoint) {
				return variant("prismatic");
			} else if(joint_->GetType() == e_distanceJoint) {
				return variant("distance");
			} else if(joint_->GetType() == e_pulleyJoint) {
				return variant("pulley");
			} else if(joint_->GetType() == e_mouseJoint) {
				return variant("mouse");
			} else if(joint_->GetType() == e_gearJoint) {
				return variant("gear");
			} else if(joint_->GetType() == e_wheelJoint) {
				return variant("wheel");
			} else if(joint_->GetType() == e_weldJoint) {
				return variant("weld");
			} else if(joint_->GetType() == e_frictionJoint) {
				return variant("friction");
			} else if(joint_->GetType() == e_ropeJoint) {
				return variant("rope");
			} else {
				ASSERT_LOG(false, "Joint type unrecognised: " << joint_->GetType());
			}
		} else if(key == "get_reaction_force") {
			b2Vec2 rf = joint_->GetReactionForce(this_world->last_inv_dt());
			std::vector<variant> v;
			v.push_back(variant(rf.x));
			v.push_back(variant(rf.y));
			return variant(&v);
		} else if(key == "get_reaction_torque") {
			return variant(joint_->GetReactionTorque(this_world->last_inv_dt()));
		}
		if(joint_->GetType() == e_revoluteJoint) {
			b2RevoluteJoint* revolute = (b2RevoluteJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = revolute->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = revolute->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "reference_angle") {
				return variant(revolute->GetReferenceAngle() * 180.0 / b2_pi);
			} else if(key == "angle") {
				return variant(revolute->GetJointAngle() * 180.0 / b2_pi);
			} else if(key == "speed") {
				return variant(revolute->GetJointSpeed());
			} else if(key == "limit_enabled") {
				return variant::from_bool(revolute->IsLimitEnabled());
			} else if(key == "lower_limit") {
				return variant(revolute->GetLowerLimit());
			} else if(key == "upper_limit") {
				return variant(revolute->GetUpperLimit());
			} else if(key == "motor_enabled") {
				return variant::from_bool(revolute->IsMotorEnabled());
			} else if(key == "motor_speed") {
				return variant(revolute->GetMotorSpeed());
			} else if(key == "max_motor_torque") {
				return variant(revolute->GetMaxMotorTorque());
			} else if(key == "get_motor_torque") {
				return variant(revolute->GetMotorTorque(this_world->last_inv_dt()));
			}
		} else if(joint_->GetType() == e_prismaticJoint) {
			b2PrismaticJoint* prismatic = (b2PrismaticJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = prismatic->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = prismatic->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "axis") {
				std::vector<variant> v;
				const b2Vec2 &axis = prismatic->GetLocalAxisA();
				v.push_back(variant(axis.x));
				v.push_back(variant(axis.y));
				return variant(&v);
			} else if(key == "translation") {
				return variant(prismatic->GetJointTranslation());
			} else if(key == "reference_angle") {
				return variant(prismatic->GetReferenceAngle() * 180.0 / b2_pi);
			} else if(key == "speed") {
				return variant(prismatic->GetJointSpeed());
			} else if(key == "limit_enabled") {
				return variant::from_bool(prismatic->IsLimitEnabled());
			} else if(key == "lower_limit") {
				return variant(prismatic->GetLowerLimit());
			} else if(key == "upper_limit") {
				return variant(prismatic->GetUpperLimit());
			} else if(key == "motor_enabled") {
				return variant::from_bool(prismatic->IsMotorEnabled());
			} else if(key == "motor_speed") {
				return variant(prismatic->GetMotorSpeed());
			} else if(key == "max_motor_force") {
				return variant(prismatic->GetMaxMotorForce());
			}
		} else if(joint_->GetType() == e_distanceJoint) {
			b2DistanceJoint* distance = (b2DistanceJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = distance->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = distance->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "length") {
				return variant(distance->GetLength());
			} else if(key == "frequency") {
				return variant(distance->GetFrequency());
			} else if(key == "damping_ratio") {
				return variant(distance->GetDampingRatio());
			}
		} else if(joint_->GetType() == e_pulleyJoint) {
			b2PulleyJoint* pulley = (b2PulleyJoint*)joint_;
			if(key == "ground_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = pulley->GetGroundAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "ground_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = pulley->GetGroundAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "length_a") {
				return variant(pulley->GetLengthA());
			} else if(key == "length_b") {
				return variant(pulley->GetLengthB());
			} else if(key == "ratio") {
				return variant(pulley->GetRatio());
			}
		} else if(joint_->GetType() == e_mouseJoint) {
			b2MouseJoint* mouse = (b2MouseJoint*)joint_;
			if(key == "target") {
				std::vector<variant> v;
				const b2Vec2 &target = mouse->GetTarget();
				v.push_back(variant(target.x));
				v.push_back(variant(target.y));
				return variant(&v);
			} else if(key == "max_force") {
				return variant(mouse->GetMaxForce());
			} else if(key == "frequency") {
				return variant(mouse->GetFrequency());
			} else if(key == "damping_ratio") {
				return variant(mouse->GetDampingRatio());
			}
		} else if(joint_->GetType() == e_gearJoint) {
			b2GearJoint* gear = (b2GearJoint*)joint_;
			if(key == "joint1") {
				return variant(new joint((b2Joint*)gear->GetJoint1()));
			} else if(key == "joint2") {
				return variant(new joint((b2Joint*)gear->GetJoint2()));
			} else if(key == "ratio") {
				return variant(gear->GetRatio());
			}

		} else if(joint_->GetType() == e_wheelJoint) {
			b2WheelJoint* wheel = (b2WheelJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = wheel->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = wheel->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "axis") {
				std::vector<variant> v;
				const b2Vec2 &axis = wheel->GetLocalAxisA();
				v.push_back(variant(axis.x));
				v.push_back(variant(axis.y));
				return variant(&v);
			} else if(key == "translation") {
				return variant(wheel->GetJointTranslation());
			} else if(key == "speed") {
				return variant(wheel->GetJointSpeed());
			} else if(key == "motor_enabled") {
				return variant::from_bool(wheel->IsMotorEnabled());
			} else if(key == "motor_speed") {
				return variant(wheel->GetMotorSpeed());
			} else if(key == "max_motor_torque") {
				return variant(wheel->GetMaxMotorTorque());
			} else if(key == "spring_frequency") {
				return variant(wheel->GetSpringFrequencyHz());
			} else if(key == "spring_damping_ratio") {
				return variant(wheel->GetSpringDampingRatio());
			}
		} else if(joint_->GetType() == e_weldJoint) {
			b2WeldJoint* weld = (b2WeldJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = weld->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = weld->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "frequency") {
				return variant(weld->GetFrequency());
			} else if(key == "damping_ratio") {
				return variant(weld->GetDampingRatio());
			} else if(key == "reference_angle") {
				return variant(weld->GetReferenceAngle() * 180.0 / b2_pi);
			}
		} else if(joint_->GetType() == e_frictionJoint) {
			b2FrictionJoint* friction = (b2FrictionJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = friction->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = friction->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "max_force") {
				return variant(friction->GetMaxForce());
			} else if(key == "max_torque") {
				return variant(friction->GetMaxTorque());
			}
		} else if(joint_->GetType() == e_ropeJoint) {
			b2RopeJoint* rope = (b2RopeJoint*)joint_;
			if(key == "local_anchor_a") {
				std::vector<variant> v;
				const b2Vec2 &anchor = rope->GetLocalAnchorA();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "local_anchor_b") {
				std::vector<variant> v;
				const b2Vec2 &anchor = rope->GetLocalAnchorB();
				v.push_back(variant(anchor.x));
				v.push_back(variant(anchor.y));
				return variant(&v);
			} else if(key == "max_length") {
				return variant(rope->GetMaxLength());
			} else if(key == "limit_state") {
				b2LimitState ls = rope->GetLimitState();
				if(ls == e_inactiveLimit) {
					return variant("inactive");
				} else if(ls == e_atLowerLimit) {
					return variant("at_lower_limit");
				} else if(ls == e_atUpperLimit) {
					return variant("at_upper_limit");
				} else if(ls == e_equalLimits) {
					return variant("equal_limts");
				} else {
					ASSERT_LOG(false, "Bad rope limit state: " << ls);
				}
			}
		} else {
			ASSERT_LOG(false, "Joint type unrecognised: " << joint_->GetType());
		}
		return variant();
	}

	void joint::set_value(const std::string& key, const variant& value)
	{
		ASSERT_LOG(joint_ != NULL, "Internal joint has been destroyed.");
		if(joint_->GetType() == e_revoluteJoint) {
			b2RevoluteJoint* revolute = (b2RevoluteJoint*)joint_;
			if(key == "enable_limit") {
				revolute->EnableLimit(value.as_bool());
			} else if(key == "limits") {
				ASSERT_LOG(value.is_list() && value.num_elements() == 2,
					"Value must be list of two elements.");
				revolute->SetLimits(float(value[0].as_decimal().as_float()), float(value[1].as_decimal().as_float()));
			} else if(key == "enable_motor") {
				revolute->EnableMotor(value.as_bool());
			} else if(key == "motor_speed") {
				revolute->SetMotorSpeed(float(value.as_decimal().as_float()));
			} else if(key == "max_motor_torque") {
				revolute->SetMaxMotorTorque(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_prismaticJoint) {
			b2PrismaticJoint* prismatic = (b2PrismaticJoint*)joint_;
			if(key == "enable_limit") {
				prismatic->EnableLimit(value.as_bool());
			} else if(key == "limits") {
				ASSERT_LOG(value.is_list() && value.num_elements() == 2,
					"Value must be list of two elements.");
				prismatic->SetLimits(float(value[0].as_decimal().as_float()), float(value[1].as_decimal().as_float()));
			} else if(key == "enable_motor") {
				prismatic->EnableMotor(value.as_bool());
			} else if(key == "motor_speed") {
				prismatic->SetMotorSpeed(float(value.as_decimal().as_float()));
			} else if(key == "max_motor_force") {
				prismatic->SetMaxMotorForce(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_distanceJoint) {
			b2DistanceJoint* distance = (b2DistanceJoint*)joint_;
			if(key == "length") {
				distance->SetLength(float(value.as_decimal().as_float()));
			} else if(key == "frequency") {
				distance->SetFrequency(float(value.as_decimal().as_float()));
			} else if(key == "damping_ratio") {
				distance->SetDampingRatio(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_pulleyJoint) {
			// Nothing to set.
		} else if(joint_->GetType() == e_mouseJoint) {
			b2MouseJoint* mouse = (b2MouseJoint*)joint_;
			if(key == "target") {
				b2Vec2 target(float(value[0].as_decimal().as_float()), float(value[1].as_decimal().as_float()));
				mouse->SetTarget(target);
			} else if(key == "max_force") {
				mouse->SetMaxForce(float(value.as_decimal().as_float()));
			} else if(key == "frequency") {
				mouse->SetFrequency(float(value.as_decimal().as_float()));
			} else if(key == "damping_ratio") {
				mouse->SetDampingRatio(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_gearJoint) {
			if(key == "ratio") {
				((b2GearJoint*)joint_)->SetRatio(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_wheelJoint) {
			b2WheelJoint* wheel = (b2WheelJoint*)joint_;
			if(key == "enable_motor") {
				wheel->EnableMotor(value.as_bool());
			} else if(key == "motor_speed") {
				wheel->SetMotorSpeed(float(value.as_decimal().as_float()));
			} else if(key == "max_motor_torque") {
				wheel->SetMaxMotorTorque(float(value.as_decimal().as_float()));
			} else if(key == "spring_frequency") {
				wheel->SetSpringFrequencyHz(float(value.as_decimal().as_float()));
			} else if(key == "spring_damping_ratio") {
				wheel->SetSpringDampingRatio(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_weldJoint) {
			if(key == "frequency") {
				((b2WeldJoint*)joint_)->SetFrequency(float(value.as_decimal().as_float()));
			} else if(key == "damping_ratio") {
				((b2WeldJoint*)joint_)->SetDampingRatio(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_frictionJoint) {
			if(key == "max_force") {
				((b2FrictionJoint*)joint_)->SetMaxForce(float(value.as_decimal().as_float()));
			} else if(key == "max_torque") {
				((b2FrictionJoint*)joint_)->SetMaxTorque(float(value.as_decimal().as_float()));
			}
		} else if(joint_->GetType() == e_ropeJoint) {
			if(key == "max_length") {
				((b2RopeJoint*)joint_)->SetMaxLength(float(value.as_decimal().as_float()));
			}
		} else {
			ASSERT_LOG(false, "Joint type unrecognised: " << joint_->GetType());
		}
	}

	debug_draw::debug_draw()
	{
	}

	void debug_draw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		for(int n = 0; n != vertexCount; ++n) {
			varray.push_back(GLfloat(vertices[n].x * this_world->scale()));
			varray.push_back(GLfloat(vertices[n].y * this_world->scale()));
		}
#if defined(USE_GLES2)
		glColor4f(color.r, color.g, color.b, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		for(int n = 0; n != vertexCount; ++n) {
			varray.push_back(GLfloat(vertices[n].x * this_world->scale()));
			varray.push_back(GLfloat(vertices[n].y * this_world->scale()));
		}
#if defined(USE_GLES2)
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(color.r *0.5f, color.g*0.5f, color.b*0.5f, 0.5f);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_TRIANGLE_FAN, 0, varray.size()/2);
		glDisable(GL_BLEND);

		glColor4f(color.r, color.g, color.b, 1.0f);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r *0.5f, color.g*0.5f, color.b*0.5f, 0.5f);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_TRIANGLE_FAN, 0, varray.size()/2);
		glDisable(GL_BLEND);

		glColor4f(color.r, color.g, color.b, 1.0f);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
	{
		const float32 k_segments = 32;
		const float32 k_increment = 2.0f * b2_pi / k_segments;
		float32 theta = 0.0f;

		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		for(int n = 0; n < k_segments; ++n, theta += k_increment) {
			b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
			varray.push_back(GLfloat(v.x * this_world->scale()));
			varray.push_back(GLfloat(v.y * this_world->scale()));
		}
#if defined(USE_GLES2)
		glColor4f(color.r, color.g, color.b, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
	{
		const float32 k_segments = 32.0f;
		const float32 k_increment = 2.0f * b2_pi / k_segments;
		float32 theta = 0.0f;

		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		for(int n = 0; n < k_segments; ++n, theta += k_increment) {
			b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
			varray.push_back(GLfloat(v.x * this_world->scale()));
			varray.push_back(GLfloat(v.y * this_world->scale()));
		}
#if defined(USE_GLES2)
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(color.r *0.5f, color.g*0.5f, color.b*0.5f, 0.5f);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_TRIANGLE_FAN, 0, varray.size()/2);
		glDisable(GL_BLEND);

		glColor4f(color.r, color.g, color.b, 1.0f);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);

		varray.clear();
		b2Vec2 p = center + radius * axis;
		varray.push_back(center.x * this_world->scale());
		varray.push_back(center.y * this_world->scale());
		varray.push_back(p.x);
		varray.push_back(p.y);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);

		glColor4ub(255, 255, 255, 255);
#else
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r *0.5f, color.g*0.5f, color.b*0.5f, 0.5f);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_TRIANGLE_FAN, 0, varray.size()/2);
		glDisable(GL_BLEND);

		glColor4f(color.r, color.g, color.b, 1.0f);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);

		varray.clear();
		b2Vec2 p = center + radius * axis;
		varray.push_back(center.x * world_->scale());
		varray.push_back(center.y * world_->scale());
		varray.push_back(p.x);
		varray.push_back(p.y);
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		varray.push_back(GLfloat(p1.x * this_world->scale()));
		varray.push_back(GLfloat(p1.y * this_world->scale()));
		varray.push_back(GLfloat(p2.x * this_world->scale()));
		varray.push_back(GLfloat(p2.y * this_world->scale()));
#if defined(USE_GLES2)
		glColor4f(color.r, color.g, color.b, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawTransform(const b2Transform& xf)
	{
		b2Vec2 p1 = xf.p, p2;
		const float32 k_axisScale = 0.4f;
		p2 = p1 + k_axisScale * xf.q.GetXAxis();

		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		varray.push_back(GLfloat(p1.x * this_world->scale()));
		varray.push_back(GLfloat(p1.y * this_world->scale()));
		varray.push_back(GLfloat(p2.x * this_world->scale()));
		varray.push_back(GLfloat(p2.y * this_world->scale()));
#if defined(USE_GLES2)
		glColor4f(1.0f, 0.0f, 0.0f, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);

		p2 = p1 + k_axisScale * xf.q.GetYAxis();
		glColor4f(0.0f, 1.0f, 0.0f, 1.0);
		varray.clear();
		varray.push_back(GLfloat(p1.x * this_world->scale()));
		varray.push_back(GLfloat(p1.y * this_world->scale()));
		varray.push_back(GLfloat(p2.x * this_world->scale()));
		varray.push_back(GLfloat(p2.y * this_world->scale()));
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);

		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_LINES, 0, varray.size()/2);

		p2 = p1 + k_axisScale * xf.q.GetYAxis();
		glColor4f(0.0f, 1.0f, 0.0f, 1.0);
		varray.clear();
		varray.push_back(GLfloat(p1.x * this_world->scale()));
		varray.push_back(GLfloat(p1.y * this_world->scale()));
		varray.push_back(GLfloat(p2.x * this_world->scale()));
		varray.push_back(GLfloat(p2.y * this_world->scale()));
		glDrawArrays(GL_LINES, 0, varray.size()/2);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

	void debug_draw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
	{
		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		varray.push_back(GLfloat(p.x));
		varray.push_back(GLfloat(p.y));
		glPointSize(size);
#if defined(USE_GLES2)
		glColor4f(color.r, color.g, color.b, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_POINTS, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_POINTS, 0, varray.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
		glPointSize(1.0f);
	}

	void debug_draw::DrawString(int x, int y, const char *string, ...)
	{
		// XXX
	}

	void debug_draw::DrawAABB(b2AABB* aabb, const b2Color& color)
	{
		std::vector<GLfloat>& varray = graphics::global_vertex_array();
		varray.clear();
		varray.push_back(GLfloat(aabb->lowerBound.x * this_world->scale()));
		varray.push_back(GLfloat(aabb->lowerBound.y * this_world->scale()));
		varray.push_back(GLfloat(aabb->upperBound.x * this_world->scale()));
		varray.push_back(GLfloat(aabb->lowerBound.y * this_world->scale()));
		varray.push_back(GLfloat(aabb->upperBound.x * this_world->scale()));
		varray.push_back(GLfloat(aabb->upperBound.y * this_world->scale()));
		varray.push_back(GLfloat(aabb->lowerBound.y * this_world->scale()));
		varray.push_back(GLfloat(aabb->upperBound.y * this_world->scale()));
#if defined(USE_GLES2)
		glColor4f(color.r, color.g, color.b, 1.0);
		gles2::manager gles2_manager(gles2::get_simple_shader());
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glColor4ub(255, 255, 255, 255);
#else
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glLineWidth(1.0f);
		glColor4f(color.r, color.g, color.b, 1.0);
		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_LINE_LOOP, 0, varray.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
#endif
	}

}

#endif
